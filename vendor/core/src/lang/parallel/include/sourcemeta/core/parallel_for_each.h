#ifndef SOURCEMETA_CORE_PARALLEL_FOR_EACH_H_
#define SOURCEMETA_CORE_PARALLEL_FOR_EACH_H_

#include <algorithm> // std::max
#include <concepts>  // std::copyable, std::invocable
#include <exception> // std::exception_ptr, std::current_exception, std::rethrow_exception
#include <functional> // std::function
#include <iterator>   // std::input_iterator, std::iter_reference_t
#include <mutex>      // std::mutex, std::lock_guard
#include <queue>      // std::queue
#include <stdexcept>  // std::runtime_error
#include <thread>     // std::thread
#include <utility>    // std::forward
#include <vector>     // std::vector

#if defined(_WIN32)
#include <process.h> // _beginthreadex
#define NOMINMAX
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace sourcemeta::core {

#ifndef DOXYGEN
#if defined(_WIN32)
// See
// https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/beginthread-beginthreadex?view=msvc-170
inline unsigned __stdcall parallel_for_each_windows_thread_start(
    void *argument) {
  auto *function_ptr = static_cast<std::function<void()> *>(argument);
  (*function_ptr)();
  delete function_ptr;
  return 0;
}
#endif
#endif

/// @ingroup parallel
///
/// Process a collection in parallel. If the parallelism is set to zero, the
/// function will run using the available number of cores. If the stack size is
/// set to zero, the platform default applies. For example:
///
/// ```c++
/// #include <sourcemeta/core/parallel.h>
/// #include <vector>
/// #include <mutex>
/// #include <iostream>
///
/// std::vector<std::size_t> input;
/// for (std::size_t index = 0; index < 20; index++) {
///   input.push_back(index);
/// }
///
/// std::mutex mutex;
/// std::vector<std::size_t> result;
///
/// sourcemeta::core::parallel_for_each(
///     input.cbegin(), input.cend(),
///     [&mutex, &result](const auto value,
///                       const auto parallelism,
///                       const auto cursor) {
///       std::lock_guard<std::mutex> lock{mutex};
///       result.push_back(value);
///       std::cerr << "Processing " << cursor
///                 << " with parallelism " << parallelism << "\n";
///     });
/// ```
template <typename Iterator, typename Callback>
  requires std::input_iterator<Iterator> && std::copyable<Iterator> &&
           std::invocable<Callback, std::iter_reference_t<Iterator>,
                          std::size_t, std::size_t>
auto parallel_for_each(
    Iterator first, Iterator last, Callback &&callback,
    const std::size_t parallelism = std::thread::hardware_concurrency(),
    const std::size_t stack_size_bytes = 0) -> void {
  std::queue<Iterator> tasks;
  for (auto iterator = first; iterator != last; ++iterator) {
    tasks.push(iterator);
  }

  std::mutex queue_mutex;
  std::mutex exception_mutex;

  auto effective_callback = std::forward<Callback>(callback);

  std::exception_ptr exception = nullptr;
  auto handle_exception = [&exception_mutex,
                           &exception](std::exception_ptr pointer) {
    std::lock_guard<std::mutex> lock{exception_mutex};
    if (!exception) {
      exception = pointer;
    }
  };

  const auto effective_parallelism{
      std::max(parallelism, static_cast<std::size_t>(1))};
  std::vector<std::thread> workers;
  workers.reserve(effective_parallelism);

  const auto total{tasks.size()};

  // Worker function that runs the actual per-item work and captures the
  // environment by reference. It will be heap-copied into the native thread
  // API.
  auto worker_callable = [&tasks, &queue_mutex, &effective_callback,
                          &handle_exception, effective_parallelism, total] {
    try {
      while (true) {
        Iterator iterator;
        std::size_t cursor{0};
        {
          std::lock_guard<std::mutex> lock{queue_mutex};
          if (tasks.empty()) {
            return;
          }
          iterator = tasks.front();
          cursor = total - tasks.size() + 1;
          tasks.pop();
        }
        effective_callback(*iterator, effective_parallelism, cursor);
      }
    } catch (...) {
      handle_exception(std::current_exception());
    }
  };

#if defined(_WIN32)
  for (std::size_t index = 0; index < effective_parallelism; ++index) {
    auto *heap_function = new std::function<void()>(worker_callable);
    if (stack_size_bytes > static_cast<std::size_t>(UINT_MAX)) {
      delete heap_function;
      throw std::runtime_error(
          "The requested stack size is too large for this platform");
    }

    auto raw_handle = _beginthreadex(
        nullptr, static_cast<unsigned>(stack_size_bytes),
        &parallel_for_each_windows_thread_start, heap_function, 0, nullptr);
    if (raw_handle == 0) {
      delete heap_function;
      throw std::runtime_error("Could not create thread");
    }

    HANDLE thread_handle = reinterpret_cast<HANDLE>(raw_handle);
    workers.emplace_back([thread_handle] {
      WaitForSingleObject(thread_handle, INFINITE);
      CloseHandle(thread_handle);
    });
  }
#else
  for (std::size_t index = 0; index < effective_parallelism; ++index) {
    // We can't use std::thread, as it doesn't let us tweak the thread stack
    // size
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_size_bytes > 0) {
      pthread_attr_setstacksize(&attr, stack_size_bytes);
    }

    auto *heap_function = new std::function<void()>(worker_callable);
    pthread_t pthread_handle;
    auto raw_handle = pthread_create(
        &pthread_handle, &attr,
        [](void *arg) -> void * {
          auto *function_ptr = static_cast<std::function<void()> *>(arg);
          (*function_ptr)();
          delete function_ptr;
          return nullptr;
        },
        heap_function);
    if (raw_handle != 0) {
      pthread_attr_destroy(&attr);
      delete heap_function;
      throw std::runtime_error("Could not create thread");
    }
    workers.emplace_back(
        [pthread_handle] { pthread_join(pthread_handle, nullptr); });
    pthread_attr_destroy(&attr);
  }
#endif

  for (auto &worker_thread : workers) {
    worker_thread.join();
  }

  if (exception) {
    std::rethrow_exception(exception);
  }
}

} // namespace sourcemeta::core

#endif
