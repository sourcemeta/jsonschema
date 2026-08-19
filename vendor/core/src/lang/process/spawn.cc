#include <sourcemeta/core/process.h>

// NOLINTBEGIN(misc-include-cleaner)
#include "command_line.h"
// NOLINTEND(misc-include-cleaner)

#include <array>            // std::array
#include <cassert>          // assert
#include <cerrno>           // EAGAIN, EINTR, ENOENT, errno
#include <cstddef>          // std::size_t
#include <filesystem>       // std::filesystem
#include <initializer_list> // std::initializer_list
#include <span>             // std::span
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <utility>          // std::move
#include <vector>           // std::vector

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <sourcemeta/core/text.h>

#include <algorithm>  // std::sort
#include <functional> // std::ref
#include <map>        // std::map
#include <thread>     // std::thread
#include <windows.h> // CreateProcessW, CreatePipe, CreateFileW, DuplicateHandle, GetStdHandle, ReadFile, WriteFile, SetHandleInformation, STARTUPINFOW, PROCESS_INFORMATION, WaitForSingleObject, GetExitCodeProcess, MultiByteToWideChar, CloseHandle, WAIT_FAILED
#else
#include <csignal> // sigset_t, sigemptyset, sigaddset, sigismember, sigpending, sigwait, SIGPIPE, SIG_BLOCK, SIG_SETMASK
#include <fcntl.h> // fcntl, FD_CLOEXEC, F_GETFD, F_SETFD, F_GETFL, F_SETFL, O_NONBLOCK
#include <poll.h> // poll, pollfd, POLLIN, POLLOUT, POLLERR, POLLHUP, POLLNVAL
#include <pthread.h> // pthread_sigmask
#include <spawn.h> // posix_spawnp, posix_spawnattr_t, posix_spawnattr_init, posix_spawnattr_destroy, posix_spawn_file_actions_t, posix_spawn_file_actions_init, posix_spawn_file_actions_destroy, posix_spawn_file_actions_adddup2, pid_t
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS
#include <unistd.h> // pipe, read, write, close, chdir, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO

extern char **environ;
#endif

namespace {

// Large enough that a chatty program is drained in a handful of system calls,
// small enough to sit on the stack of any thread
constexpr std::size_t TRANSFER_BUFFER_SIZE{16384};

} // namespace

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)

namespace {

class Handle {
public:
  Handle() = default;
  explicit Handle(HANDLE value) : value_{value} {}
  ~Handle() { this->close(); }
  Handle(const Handle &) = delete;
  auto operator=(const Handle &) -> Handle & = delete;
  Handle(Handle &&other) noexcept : value_{other.value_} {
    other.value_ = nullptr;
  }
  auto operator=(Handle &&other) noexcept -> Handle & {
    if (this != &other) {
      this->close();
      this->value_ = other.value_;
      other.value_ = nullptr;
    }

    return *this;
  }

  [[nodiscard]] auto get() const noexcept -> HANDLE { return this->value_; }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return this->value_ != nullptr && this->value_ != INVALID_HANDLE_VALUE;
  }

  auto close() noexcept -> void {
    if (this->valid()) {
      CloseHandle(this->value_);
    }

    this->value_ = nullptr;
  }

private:
  HANDLE value_{nullptr};
};

auto to_wide(const std::string_view input) -> std::wstring {
  if (input.empty()) {
    return {};
  }

  const int length{MultiByteToWideChar(
      CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0)};
  if (length <= 0) {
    return {};
  }

  std::wstring result(static_cast<std::size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                      result.data(), length);
  return result;
}

// An inheritable pipe whose parent-side handle is explicitly made
// non-inheritable, so that the child never holds the end the parent works with
auto make_pipe(Handle &read_end, Handle &write_end, const bool inherit_read)
    -> bool {
  SECURITY_ATTRIBUTES attributes{};
  attributes.nLength = sizeof(attributes);
  attributes.lpSecurityDescriptor = nullptr;
  attributes.bInheritHandle = TRUE;

  HANDLE raw_read{nullptr};
  HANDLE raw_write{nullptr};
  if (!CreatePipe(&raw_read, &raw_write, &attributes, 0)) {
    return false;
  }

  read_end = Handle{raw_read};
  write_end = Handle{raw_write};
  const HANDLE parent_end{inherit_read ? raw_write : raw_read};
  return SetHandleInformation(parent_end, HANDLE_FLAG_INHERIT, 0) != 0;
}

// A standard handle the caller owns may be absent, as when there is no console,
// or may not be inheritable. Naming any handle means naming all three, so the
// ones this call does not replace are handed over as inheritable duplicates it
// owns and closes, falling back to the null device so that the program always
// receives something it can use
auto inheritable_standard_handle(const DWORD stream, Handle &storage)
    -> HANDLE {
  const HANDLE original{GetStdHandle(stream)};
  if (original != nullptr && original != INVALID_HANDLE_VALUE) {
    HANDLE duplicate{nullptr};
    if (DuplicateHandle(GetCurrentProcess(), original, GetCurrentProcess(),
                        &duplicate, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
      storage = Handle{duplicate};
      return duplicate;
    }
  }

  SECURITY_ATTRIBUTES attributes{};
  attributes.nLength = sizeof(attributes);
  attributes.lpSecurityDescriptor = nullptr;
  attributes.bInheritHandle = TRUE;
  const HANDLE null_device{CreateFileW(
      L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
      &attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
  if (null_device == INVALID_HANDLE_VALUE) {
    return INVALID_HANDLE_VALUE;
  }

  storage = Handle{null_device};
  return null_device;
}

auto read_handle_to_string(HANDLE handle, std::string &destination) -> void {
  std::array<char, TRANSFER_BUFFER_SIZE> buffer{};
  DWORD count{0};
  while (ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()),
                  &count, nullptr) &&
         count > 0) {
    destination.append(buffer.data(), count);
  }
}

// The block is a run of null terminated name=value strings closed by an extra
// null. Windows expects the names ordered case-insensitively
auto to_environment_block(
    const std::map<std::string_view, std::string_view> &environment)
    -> std::wstring {
  std::vector<const std::pair<const std::string_view, std::string_view> *>
      entries;
  entries.reserve(environment.size());
  for (const auto &entry : environment) {
    entries.push_back(&entry);
  }

  std::sort(
      entries.begin(), entries.end(), [](const auto *left, const auto *right) {
        return sourcemeta::core::less_ignore_case(left->first, right->first);
      });

  std::wstring block;
  for (const auto *entry : entries) {
    block.append(to_wide(entry->first));
    block.push_back(L'=');
    block.append(to_wide(entry->second));
    block.push_back(L'\0');
  }

  // The block closes with an empty entry, so an environment with nothing in it
  // is still two nulls rather than one
  if (entries.empty()) {
    block.push_back(L'\0');
  }

  block.push_back(L'\0');
  return block;
}

} // namespace

#else

namespace {

class Descriptor {
public:
  Descriptor() = default;
  explicit Descriptor(const int value) : value_{value} {}
  ~Descriptor() { this->close(); }
  Descriptor(const Descriptor &) = delete;
  auto operator=(const Descriptor &) -> Descriptor & = delete;
  Descriptor(Descriptor &&other) noexcept : value_{other.value_} {
    other.value_ = -1;
  }
  auto operator=(Descriptor &&other) noexcept -> Descriptor & {
    if (this != &other) {
      this->close();
      this->value_ = other.value_;
      other.value_ = -1;
    }

    return *this;
  }

  [[nodiscard]] auto get() const noexcept -> int { return this->value_; }

  [[nodiscard]] auto valid() const noexcept -> bool {
    return this->value_ != -1;
  }

  auto close() noexcept -> void {
    if (this->value_ != -1) {
      ::close(this->value_);
      this->value_ = -1;
    }
  }

private:
  int value_{-1};
};

// Writing to a pipe whose read end is gone raises SIGPIPE, whose default
// disposition terminates the process before the write can report EPIPE. A
// library cannot install a process-wide disposition, so the signal is blocked
// for the calling thread alone and any instance raised meanwhile is consumed
// before the previous mask is restored
class SignalGuard {
public:
  SignalGuard() {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGPIPE);
    this->active_ = pthread_sigmask(SIG_BLOCK, &blocked, &this->previous_) == 0;
  }

  ~SignalGuard() {
    if (!this->active_) {
      return;
    }

    // Only consume it if the caller was not already blocking it, as otherwise
    // the instance belongs to whoever established that mask
    if (!sigismember(&this->previous_, SIGPIPE)) {
      sigset_t pending;
      sigemptyset(&pending);
      if (sigpending(&pending) == 0 && sigismember(&pending, SIGPIPE)) {
        sigset_t target;
        sigemptyset(&target);
        sigaddset(&target, SIGPIPE);
        int signal_number{0};
        sigwait(&target, &signal_number);
      }
    }

    pthread_sigmask(SIG_SETMASK, &this->previous_, nullptr);
  }

  SignalGuard(const SignalGuard &) = delete;
  auto operator=(const SignalGuard &) -> SignalGuard & = delete;
  SignalGuard(SignalGuard &&) = delete;
  auto operator=(SignalGuard &&) -> SignalGuard & = delete;

private:
  sigset_t previous_{};
  bool active_{false};
};

#if !defined(__linux__) && !defined(__FreeBSD__)
auto set_close_on_exec(const int descriptor) -> bool {
  const int flags{fcntl(descriptor, F_GETFD)};
  return flags != -1 && fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC) != -1;
}
#endif

auto set_non_blocking(const int descriptor) -> bool {
  const int flags{fcntl(descriptor, F_GETFL)};
  return flags != -1 && fcntl(descriptor, F_SETFL, flags | O_NONBLOCK) != -1;
}

// Duplicating a descriptor onto itself is defined to leave its flags alone, so
// an endpoint that landed on one of the standard descriptors, which happens
// when the caller closed one of its own, would keep close-on-exec and be shut
// rather than handed over. Moving it out of the way keeps every duplication a
// real one
auto relocate_above_standard(Descriptor &descriptor) -> bool {
  if (descriptor.get() > STDERR_FILENO) {
    return true;
  }

  const int moved{fcntl(descriptor.get(), F_DUPFD_CLOEXEC, STDERR_FILENO + 1)};
  if (moved == -1) {
    return false;
  }

  descriptor = Descriptor{moved};
  return true;
}

// Both ends are marked close-on-exec so that a spawn running concurrently on
// another thread cannot leak them into its own child. The descriptors this call
// hands to its child are duplicated onto the standard ones, and duplication
// clears that flag on the copy
auto make_pipe(Descriptor &read_end, Descriptor &write_end) -> bool {
  std::array<int, 2> descriptors{};

#if defined(__linux__) || defined(__FreeBSD__)
  // Setting the flag as part of the creation leaves no window in which another
  // thread can spawn a program that inherits these
  if (::pipe2(descriptors.data(), O_CLOEXEC) != 0) {
    return false;
  }

  read_end = Descriptor{descriptors[0]};
  write_end = Descriptor{descriptors[1]};
#else
  // Without an atomic creation the flag has to be set afterwards, which leaves
  // a window that only matters to a program spawned by another thread in
  // between
  if (::pipe(descriptors.data()) != 0) {
    return false;
  }

  read_end = Descriptor{descriptors[0]};
  write_end = Descriptor{descriptors[1]};
  if (!set_close_on_exec(read_end.get()) ||
      !set_close_on_exec(write_end.get())) {
    return false;
  }
#endif

  return relocate_above_standard(read_end) &&
         relocate_above_standard(write_end);
}

// On every platform this builds for, a would-block error shares its value with
// EAGAIN
auto is_retryable_error() -> bool { return errno == EINTR || errno == EAGAIN; }

// Returns whether the stream is still open
auto drain_stream(Descriptor &descriptor, std::string &destination) -> bool {
  std::array<char, TRANSFER_BUFFER_SIZE> buffer{};
  const auto count{::read(descriptor.get(), buffer.data(), buffer.size())};
  if (count > 0) {
    destination.append(buffer.data(), static_cast<std::size_t>(count));
    return true;
  }
  if (count == -1 && is_retryable_error()) {
    return true;
  }

  descriptor.close();
  return false;
}

auto write_stream(Descriptor &descriptor, const std::string_view input,
                  std::size_t &offset) -> void {
  const auto count{
      ::write(descriptor.get(), input.data() + offset, input.size() - offset)};
  if (count > 0) {
    offset += static_cast<std::size_t>(count);
    if (offset >= input.size()) {
      descriptor.close();
    }

    return;
  }
  if (count == -1 && is_retryable_error()) {
    return;
  }

  // The program is not interested in the rest of its input, such as when it
  // exited before consuming it
  descriptor.close();
}

auto transfer(Descriptor &input_descriptor, Descriptor &output_descriptor,
              Descriptor &error_descriptor, const std::string_view input,
              std::string &output, std::string &error) -> bool {
  const SignalGuard guard;
  std::size_t offset{0};

  while (input_descriptor.valid() || output_descriptor.valid() ||
         error_descriptor.valid()) {
    std::array<pollfd, 3> descriptors{};
    std::size_t count{0};
    std::size_t input_index{3};
    std::size_t output_index{3};
    std::size_t error_index{3};

    if (input_descriptor.valid()) {
      descriptors[count].fd = input_descriptor.get();
      descriptors[count].events = POLLOUT;
      input_index = count;
      count += 1;
    }

    if (output_descriptor.valid()) {
      descriptors[count].fd = output_descriptor.get();
      descriptors[count].events = POLLIN;
      output_index = count;
      count += 1;
    }

    if (error_descriptor.valid()) {
      descriptors[count].fd = error_descriptor.get();
      descriptors[count].events = POLLIN;
      error_index = count;
      count += 1;
    }

    if (poll(descriptors.data(), static_cast<nfds_t>(count), -1) == -1) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    if (input_index < count) {
      const auto events{descriptors[input_index].revents};
      if ((events & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        input_descriptor.close();
      } else if ((events & POLLOUT) != 0) {
        write_stream(input_descriptor, input, offset);
      }
    }

    // A hangup still delivers whatever the pipe holds, so the stream is only
    // considered finished once a read reports the end of it
    if (output_index < count && (descriptors[output_index].revents &
                                 (POLLIN | POLLHUP | POLLERR)) != 0) {
      drain_stream(output_descriptor, output);
    }

    if (error_index < count && (descriptors[error_index].revents &
                                (POLLIN | POLLHUP | POLLERR)) != 0) {
      drain_stream(error_descriptor, error);
    }
  }

  return true;
}

} // namespace

#endif

namespace sourcemeta::core {

namespace {

// The two entry points differ only in what they pipe. Capturing always replaces
// the output streams, and always replaces the input stream so that a program
// never reaches back to whatever the caller had on its own standard input.
// Without capturing, a stream is only replaced when there is input to deliver
auto execute(const std::string &program,
             std::span<const std::string_view> arguments,
             const ProcessInput &input, const bool capture) -> ProcessOutput {
  assert(input.directory.is_absolute());
  assert(std::filesystem::exists(input.directory));
  assert(std::filesystem::is_directory(input.directory));

  const bool input_piped{capture || !input.standard_input.empty()};
  const bool output_piped{capture};

  sourcemeta::core::ProcessOutput result;

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
  Handle input_read;
  Handle input_write;
  Handle output_read;
  Handle output_write;
  Handle error_read;
  Handle error_write;
  if (input_piped && !make_pipe(input_read, input_write, true)) {
    throw ProcessSpawnError{program, arguments};
  }

  if (output_piped && (!make_pipe(output_read, output_write, false) ||
                       !make_pipe(error_read, error_write, false))) {
    throw ProcessSpawnError{program, arguments};
  }

  std::string command_line;
  append_quoted_argument(command_line, program);
  for (const auto &argument : arguments) {
    command_line.push_back(' ');
    append_quoted_argument(command_line, argument);
  }

  std::wstring wide_command_line{to_wide(command_line)};
  wide_command_line.push_back(L'\0');
  const std::wstring working_directory{to_wide(input.directory.string())};
  std::wstring environment_block;
  if (input.environment.has_value()) {
    environment_block = to_environment_block(input.environment.value());
  }

  Handle inherited_input;
  Handle inherited_output;
  Handle inherited_error;
  STARTUPINFOW startup_info{};
  startup_info.cb = sizeof(startup_info);
  if (input_piped || output_piped) {
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdInput =
        input_piped
            ? input_read.get()
            : inheritable_standard_handle(STD_INPUT_HANDLE, inherited_input);
    startup_info.hStdOutput =
        output_piped
            ? output_write.get()
            : inheritable_standard_handle(STD_OUTPUT_HANDLE, inherited_output);
    startup_info.hStdError =
        output_piped
            ? error_write.get()
            : inheritable_standard_handle(STD_ERROR_HANDLE, inherited_error);
  }

  PROCESS_INFORMATION process_info{};
  const DWORD flags{input.environment.has_value()
                        ? static_cast<DWORD>(CREATE_UNICODE_ENVIRONMENT)
                        : static_cast<DWORD>(0)};
  const BOOL success{CreateProcessW(
      nullptr, wide_command_line.data(), nullptr, nullptr, TRUE, flags,
      input.environment.has_value() ? environment_block.data() : nullptr,
      working_directory.c_str(), &startup_info, &process_info)};

  if (!success) {
    const DWORD error_code{GetLastError()};
    if (error_code == ERROR_FILE_NOT_FOUND ||
        error_code == ERROR_PATH_NOT_FOUND) {
      throw ProcessProgramNotFoundError{program};
    }

    throw ProcessSpawnError{program, arguments};
  }

  const Handle process{process_info.hProcess};
  const Handle process_thread{process_info.hThread};

  // The parent must let go of the ends it handed over, as otherwise the reads
  // below never see the end of either stream
  input_read.close();
  output_write.close();
  error_write.close();

  // An anonymous pipe cannot be waited on, so each output stream is drained by
  // a thread of its own while this one feeds the input. Otherwise a program
  // that fills one pipe blocks forever against a parent blocked on writing
  std::thread output_reader;
  std::thread error_reader;
  if (output_piped) {
    output_reader = std::thread{read_handle_to_string, output_read.get(),
                                std::ref(result.standard_output)};
    error_reader = std::thread{read_handle_to_string, error_read.get(),
                               std::ref(result.standard_error)};
  }

  std::size_t offset{0};
  while (input_piped && offset < input.standard_input.size()) {
    // Bounded so that an input of four gibibytes or more cannot wrap on its way
    // into the smaller count this call takes
    const std::size_t remaining{input.standard_input.size() - offset};
    const DWORD chunk{static_cast<DWORD>(
        remaining < TRANSFER_BUFFER_SIZE ? remaining : TRANSFER_BUFFER_SIZE)};
    DWORD written{0};
    if (!WriteFile(input_write.get(), input.standard_input.data() + offset,
                   chunk, &written, nullptr) ||
        written == 0) {
      break;
    }

    offset += written;
  }

  input_write.close();
  if (output_piped) {
    output_reader.join();
    error_reader.join();
  }

  if (WaitForSingleObject(process.get(), INFINITE) == WAIT_FAILED) {
    throw ProcessSpawnError{program, arguments};
  }

  DWORD exit_code{0};
  if (!GetExitCodeProcess(process.get(), &exit_code)) {
    throw ProcessSpawnError{program, arguments};
  }

  result.exit_code = static_cast<int>(exit_code);
  return result;
#else
  Descriptor input_read;
  Descriptor input_write;
  Descriptor output_read;
  Descriptor output_write;
  Descriptor error_read;
  Descriptor error_write;
  if (input_piped && (!make_pipe(input_read, input_write) ||
                      !set_non_blocking(input_write.get()))) {
    throw ProcessSpawnError{program, arguments};
  }

  if (output_piped && (!make_pipe(output_read, output_write) ||
                       !make_pipe(error_read, error_write) ||
                       !set_non_blocking(output_read.get()) ||
                       !set_non_blocking(error_read.get()))) {
    throw ProcessSpawnError{program, arguments};
  }

  std::vector<std::string> owned_arguments;
  owned_arguments.reserve(arguments.size());
  for (const auto &argument : arguments) {
    owned_arguments.emplace_back(argument);
  }

  std::vector<const char *> argv;
  argv.reserve(owned_arguments.size() + 2);
  argv.push_back(program.c_str());
  for (const auto &argument : owned_arguments) {
    argv.push_back(argument.c_str());
  }

  argv.push_back(nullptr);

  std::vector<std::string> owned_environment;
  std::vector<char *> envp;
  if (input.environment.has_value()) {
    owned_environment.reserve(input.environment.value().size());
    for (const auto &entry : input.environment.value()) {
      std::string variable;
      variable.reserve(entry.first.size() + entry.second.size() + 1);
      variable.append(entry.first);
      variable.push_back('=');
      variable.append(entry.second);
      owned_environment.emplace_back(std::move(variable));
    }

    envp.reserve(owned_environment.size() + 1);
    for (auto &entry : owned_environment) {
      envp.push_back(entry.data());
    }

    envp.push_back(nullptr);
  }

  posix_spawnattr_t attributes;
  posix_spawnattr_init(&attributes);

  posix_spawn_file_actions_t file_actions;
  posix_spawn_file_actions_init(&file_actions);

  const bool wiring{
      (!input_piped ||
       posix_spawn_file_actions_adddup2(&file_actions, input_read.get(),
                                        STDIN_FILENO) == 0) &&
      (!output_piped ||
       (posix_spawn_file_actions_adddup2(&file_actions, output_write.get(),
                                         STDOUT_FILENO) == 0 &&
        posix_spawn_file_actions_adddup2(&file_actions, error_write.get(),
                                         STDERR_FILENO) == 0))};
  if (!wiring) {
    posix_spawn_file_actions_destroy(&file_actions);
    posix_spawnattr_destroy(&attributes);
    throw ProcessSpawnError{program, arguments};
  }

#if defined(__MSYS__) || defined(__CYGWIN__) || defined(__MINGW32__) ||        \
    defined(__MINGW64__)
  // These platforms lack a child-directory file action, so we change the
  // process-wide working directory around the spawn and restore it afterwards
  // This races with any concurrent thread that observes or mutates the current
  // directory while the spawn is in flight
  const std::filesystem::path original_directory{
      std::filesystem::current_path()};
  std::filesystem::current_path(input.directory);
#else
  // The standardized child-directory file action is not yet provided by every
  // system and toolchain this builds on, so we keep using the long-standing
  // platform extension and silence the deprecation that newer SDKs attach to it
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  const int addchdir_result{posix_spawn_file_actions_addchdir_np(
      &file_actions, input.directory.c_str())};
#pragma GCC diagnostic pop
  if (addchdir_result != 0) {
    posix_spawn_file_actions_destroy(&file_actions);
    posix_spawnattr_destroy(&attributes);
    throw ProcessSpawnError{program, arguments};
  }
#endif

  pid_t process_id;
  const int spawn_result{
      posix_spawnp(&process_id, program.c_str(), &file_actions, &attributes,
                   const_cast<char *const *>(argv.data()),
                   input.environment.has_value() ? envp.data() : environ)};

  posix_spawn_file_actions_destroy(&file_actions);
  posix_spawnattr_destroy(&attributes);

#if defined(__MSYS__) || defined(__CYGWIN__) || defined(__MINGW32__) ||        \
    defined(__MINGW64__)
  std::filesystem::current_path(original_directory);
#endif

  if (spawn_result != 0) {
    if (spawn_result == ENOENT) {
      throw ProcessProgramNotFoundError{program};
    }

    throw ProcessSpawnError{program, arguments};
  }

  // The parent must let go of the ends it handed over, as otherwise the reads
  // below never see the end of either stream
  input_read.close();
  output_write.close();
  error_write.close();

  if (input.standard_input.empty()) {
    input_write.close();
  }

  const bool transferred{transfer(input_write, output_read, error_read,
                                  input.standard_input, result.standard_output,
                                  result.standard_error)};

  int status{0};
  while (waitpid(process_id, &status, 0) == -1) {
    if (errno == EINTR) {
      continue;
    }

    throw ProcessSpawnError{program, arguments};
  }

  if (!transferred) {
    throw ProcessSpawnError{program, arguments};
  }

  if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  }

  return result;
#endif
}

} // namespace

auto spawn(const std::string &program,
           std::span<const std::string_view> arguments,
           const ProcessInput &input) -> int {
  const auto result{execute(program, arguments, input, false)};
  if (!result.exit_code.has_value()) {
    throw ProcessSpawnError{program, arguments};
  }

  return result.exit_code.value();
}

auto spawn(const std::string &program,
           std::initializer_list<std::string_view> arguments,
           const ProcessInput &input) -> int {
  return spawn(
      program,
      std::span<const std::string_view>{arguments.begin(), arguments.size()},
      input);
}

auto spawn_and_capture(const std::string &program,
                       std::span<const std::string_view> arguments,
                       const ProcessInput &input) -> ProcessOutput {
  return execute(program, arguments, input, true);
}

auto spawn_and_capture(const std::string &program,
                       std::initializer_list<std::string_view> arguments,
                       const ProcessInput &input) -> ProcessOutput {
  return spawn_and_capture(
      program,
      std::span<const std::string_view>{arguments.begin(), arguments.size()},
      input);
}

} // namespace sourcemeta::core
