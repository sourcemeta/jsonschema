#ifndef SOURCEMETA_CORE_STACKTRACE_POSIX_H_
#define SOURCEMETA_CORE_STACKTRACE_POSIX_H_

#include <sourcemeta/core/stacktrace.h>

#include <array>            // std::array
#include <atomic>           // std::atomic
#include <csignal>          // sigaction, struct sigaction, SIG*, raise
#include <cstddef>          // std::size_t
#include <cstdint>          // std::uintptr_t
#include <cstring>          // std::strlen
#include <initializer_list> // std::initializer_list

#include <dlfcn.h>        // dladdr, Dl_info
#include <execinfo.h>     // backtrace
#include <sys/ucontext.h> // ucontext_t
#include <unistd.h>       // write, getpid, STDERR_FILENO

namespace {

constexpr int maximum_frames{128};
constexpr std::size_t hex_buffer_size{2 + (sizeof(std::uintptr_t) * 2)};
constexpr std::size_t decimal_buffer_size{24};

auto raw_write(int file_descriptor, const char *data, std::size_t size)
    -> void {
  [[maybe_unused]] const auto result{::write(file_descriptor, data, size)};
}

auto write_text(int file_descriptor, const char *text) -> void {
  raw_write(file_descriptor, text, std::strlen(text));
}

auto write_hex(int file_descriptor, std::uintptr_t value) -> void {
  constexpr const char *digits{"0123456789abcdef"};
  std::array<char, hex_buffer_size> buffer{{'0', 'x'}};
  std::size_t index{2};
  if (value == 0) {
    buffer[index++] = '0';
  } else {
    std::array<char, sizeof(std::uintptr_t) * 2> temporary{};
    std::size_t length{0};
    while (value != 0) {
      temporary[length++] = digits[value & 0xF];
      value >>= 4;
    }
    while (length > 0) {
      buffer[index++] = temporary[--length];
    }
  }
  raw_write(file_descriptor, buffer.data(), index);
}

auto write_decimal(int file_descriptor, unsigned long value) -> void {
  std::array<char, decimal_buffer_size> buffer{};
  std::size_t length{0};
  if (value == 0) {
    buffer[length++] = '0';
  } else {
    std::array<char, decimal_buffer_size> temporary{};
    std::size_t temporary_length{0};
    while (value != 0) {
      temporary[temporary_length++] = static_cast<char>('0' + (value % 10));
      value /= 10;
    }
    while (temporary_length > 0) {
      buffer[length++] = temporary[--temporary_length];
    }
  }
  raw_write(file_descriptor, buffer.data(), length);
}

auto write_frame(int file_descriptor, int frame_index, void *address) -> void {
  Dl_info information{};
  const int resolved{::dladdr(address, &information)};

  write_text(file_descriptor, "#");
  write_decimal(file_descriptor, static_cast<unsigned long>(frame_index));
  write_text(file_descriptor, " ");
  write_hex(file_descriptor, reinterpret_cast<std::uintptr_t>(address));
  write_text(file_descriptor, " ");

  const char *symbol_name{(resolved != 0 && information.dli_sname != nullptr)
                              ? information.dli_sname
                              : "<unknown>"};
  write_text(file_descriptor, symbol_name);

  if (resolved != 0 && information.dli_saddr != nullptr) {
    // Subtract as integers. Pointer subtraction across unrelated objects is
    // undefined behavior in C++
    const auto offset{reinterpret_cast<std::uintptr_t>(address) -
                      reinterpret_cast<std::uintptr_t>(information.dli_saddr)};
    write_text(file_descriptor, " +");
    write_hex(file_descriptor, offset);
  }

  if (resolved != 0 && information.dli_fname != nullptr) {
    write_text(file_descriptor, "\n  in ");
    write_text(file_descriptor, information.dli_fname);
  }
  write_text(file_descriptor, "\n");
}

__attribute__((noinline)) auto write_backtrace(int file_descriptor,
                                               int frames_to_skip,
                                               void *crash_pc = nullptr)
    -> void {
  std::array<void *, maximum_frames> frames{};
  const int captured{::backtrace(frames.data(), maximum_frames)};

  int frame_index{0};
  void *suppress_saddr{nullptr};
  if (crash_pc != nullptr) {
    write_frame(file_descriptor, frame_index, crash_pc);
    frame_index = frame_index + 1;
    Dl_info crash_information{};
    if (::dladdr(crash_pc, &crash_information) != 0 &&
        crash_information.dli_saddr != nullptr) {
      suppress_saddr = crash_information.dli_saddr;
    }
  }

  for (int index{frames_to_skip}; index < captured; ++index) {
    void *address{frames[static_cast<std::size_t>(index)]};
    if (suppress_saddr != nullptr) {
      Dl_info frame_information{};
      if (::dladdr(address, &frame_information) != 0 &&
          frame_information.dli_saddr == suppress_saddr) {
        suppress_saddr = nullptr;
        continue;
      }
    }
    write_frame(file_descriptor, frame_index, address);
    frame_index = frame_index + 1;
  }
}

auto extract_crash_pc(void *context) -> void * {
  if (context == nullptr) {
    return nullptr;
  }
  [[maybe_unused]] const auto *user_context{
      static_cast<const ucontext_t *>(context)};
  std::uintptr_t program_counter{0};
#if defined(__APPLE__) && defined(__aarch64__)
  program_counter = user_context->uc_mcontext->__ss.__pc;
#elif defined(__APPLE__) && defined(__x86_64__)
  program_counter = user_context->uc_mcontext->__ss.__rip;
#elif defined(__linux__) && defined(__aarch64__)
  program_counter = user_context->uc_mcontext.pc;
#elif defined(__linux__) && defined(__x86_64__)
  program_counter =
      static_cast<std::uintptr_t>(user_context->uc_mcontext.gregs[REG_RIP]);
#endif
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  return reinterpret_cast<void *>(program_counter);
}

constexpr const char *separator{"========================================"
                                "========================================\n"};

std::atomic<bool> crash_handler_installed{false};

} // namespace

// NOTE: `backtrace`, `dladdr`, and `strlen` are not on POSIX's strict
// async-signal-safe list but are reentrant in practice for the synchronous
// faults we care about (null derefs, bad casts, divide-by-zero). We
// deliberately do not demangle. `__cxa_demangle` allocates, which is the one
// operation that would actually risk a deadlock.
//
// Outside any anonymous namespace so `dladdr` reliably resolves the symbol
// name across platforms. `extern "C"` inside an unnamed namespace does NOT
// grant external linkage per C++11
// NOLINTNEXTLINE(misc-definitions-in-headers)
extern "C" __attribute__((visibility("default"))) auto
sourcemeta_core_stacktrace_crash_handler(int signal_number,
                                         siginfo_t * /*info*/, void *context)
    -> void {
  const int file_descriptor{STDERR_FILENO};
  write_text(file_descriptor, "\n");
  write_text(file_descriptor, separator);
  write_text(file_descriptor, "signal:  ");
  write_decimal(file_descriptor, static_cast<unsigned long>(signal_number));
  write_text(file_descriptor, " (");
  const char *signal_name{"UNKNOWN"};
  switch (signal_number) {
    case SIGSEGV:
      signal_name = "SIGSEGV";
      break;
    case SIGABRT:
      signal_name = "SIGABRT";
      break;
    case SIGFPE:
      signal_name = "SIGFPE";
      break;
    case SIGBUS:
      signal_name = "SIGBUS";
      break;
    case SIGILL:
      signal_name = "SIGILL";
      break;
    default:
      break;
  }
  write_text(file_descriptor, signal_name);
  write_text(file_descriptor, ")\n");
  write_text(file_descriptor, "pid:     ");
  write_decimal(file_descriptor, static_cast<unsigned long>(::getpid()));
  write_text(file_descriptor, "\n\n");
  write_backtrace(file_descriptor, /*frames_to_skip=*/1,
                  extract_crash_pc(context));
  write_text(file_descriptor, separator);

  struct sigaction default_action{};
  default_action.sa_handler = SIG_DFL;
  sigemptyset(&default_action.sa_mask);
  ::sigaction(signal_number, &default_action, nullptr);
  ::raise(signal_number);
}

namespace sourcemeta::core {

// NOLINTNEXTLINE(misc-definitions-in-headers)
__attribute__((visibility("default"))) auto stacktrace_on_crash() -> void {
  bool expected{false};
  if (!crash_handler_installed.compare_exchange_strong(expected, true)) {
    return;
  }

  struct sigaction action{};
  action.sa_sigaction = &sourcemeta_core_stacktrace_crash_handler;
  action.sa_flags = static_cast<int>(SA_SIGINFO | SA_RESETHAND | SA_NODEFER);
  sigemptyset(&action.sa_mask);

  for (const int signal_number : {SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL}) {
    ::sigaction(signal_number, &action, nullptr);
  }
}

// NOLINTNEXTLINE(misc-definitions-in-headers)
__attribute__((noinline, visibility("default"))) auto stacktrace() -> void {
  const int file_descriptor{STDERR_FILENO};
  write_text(file_descriptor, separator);
  write_text(file_descriptor, "pid:     ");
  write_decimal(file_descriptor, static_cast<unsigned long>(::getpid()));
  write_text(file_descriptor, "\n\n");
  write_backtrace(file_descriptor, /*frames_to_skip=*/1);
  write_text(file_descriptor, separator);
}

} // namespace sourcemeta::core

#endif
