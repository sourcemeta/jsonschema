#include <sourcemeta/core/process.h>

#include <cassert>          // assert
#include <cerrno>           // ENOENT, EINTR, errno
#include <filesystem>       // std::filesystem
#include <initializer_list> // std::initializer_list
#include <span>             // std::span
#include <string>           // std::string
#include <vector>           // std::vector

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
#define WIN32_LEAN_AND_MEAN
#include <cstddef>   // std::size_t
#include <windows.h> // CreateProcess, PROCESS_INFORMATION, STARTUPINFO, WaitForSingleObject, GetExitCodeProcess, WAIT_FAILED
#else
#include <spawn.h> // posix_spawnp, posix_spawnattr_t, posix_spawnattr_init, posix_spawnattr_destroy, posix_spawn_file_actions_t, posix_spawn_file_actions_init, posix_spawn_file_actions_destroy, pid_t
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS

#if defined(__MSYS__) || defined(__CYGWIN__) || defined(__MINGW32__) ||        \
    defined(__MINGW64__)
#include <unistd.h> // chdir
#endif

extern char **environ;
#endif

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
namespace {

// Quote a single argument for the inverse of CommandLineToArgvW, so that the
// child reconstructs the exact same argument vector
auto append_quoted_argument(std::string &command_line,
                            const std::string_view argument) -> void {
  const bool needs_quoting{argument.empty() ||
                           argument.find_first_of(" \t\"") !=
                               std::string_view::npos};

  if (!needs_quoting) {
    command_line.append(argument);
    return;
  }

  command_line.push_back('"');

  for (auto cursor = argument.cbegin();; ++cursor) {
    std::size_t backslash_count{0};
    while (cursor != argument.cend() && *cursor == '\\') {
      ++cursor;
      ++backslash_count;
    }

    if (cursor == argument.cend()) {
      command_line.append(backslash_count * 2, '\\');
      break;
    } else if (*cursor == '"') {
      command_line.append(backslash_count * 2 + 1, '\\');
      command_line.push_back('"');
    } else {
      command_line.append(backslash_count, '\\');
      command_line.push_back(*cursor);
    }
  }

  command_line.push_back('"');
}

} // namespace
#endif

namespace sourcemeta::core {

auto spawn(const std::string &program,
           std::span<const std::string_view> arguments,
           const std::filesystem::path &directory) -> int {
  assert(directory.is_absolute());
  assert(std::filesystem::exists(directory));
  assert(std::filesystem::is_directory(directory));

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
  std::string command_line;
  append_quoted_argument(command_line, program);

  for (const auto &argument : arguments) {
    command_line.push_back(' ');
    append_quoted_argument(command_line, argument);
  }

  std::vector<char> cmd_line(command_line.begin(), command_line.end());
  cmd_line.push_back('\0');

  STARTUPINFOA startup_info{};
  startup_info.cb = sizeof(startup_info);
  PROCESS_INFORMATION process_info{};
  const std::string working_dir = directory.string();
  const BOOL success =
      CreateProcessA(nullptr,             // lpApplicationName
                     cmd_line.data(),     // lpCommandLine (modifiable)
                     nullptr,             // lpProcessAttributes
                     nullptr,             // lpThreadAttributes
                     TRUE,                // bInheritHandles
                     0,                   // dwCreationFlags
                     nullptr,             // lpEnvironment
                     working_dir.c_str(), // lpCurrentDirectory
                     &startup_info,       // lpStartupInfo
                     &process_info        // lpProcessInformation
      );

  if (!success) {
    const DWORD error_code{GetLastError()};
    if (error_code == ERROR_FILE_NOT_FOUND ||
        error_code == ERROR_PATH_NOT_FOUND) {
      throw ProcessProgramNotFoundError{program};
    }

    throw ProcessSpawnError{program, arguments};
  }

  if (WaitForSingleObject(process_info.hProcess, INFINITE) == WAIT_FAILED) {
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    throw ProcessSpawnError{program, arguments};
  }

  DWORD exit_code;
  if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
    throw ProcessSpawnError{program, arguments};
  }

  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);

  return static_cast<int>(exit_code);
#else
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

  posix_spawnattr_t attributes;
  posix_spawnattr_init(&attributes);

  posix_spawn_file_actions_t file_actions;
  posix_spawn_file_actions_init(&file_actions);

#if defined(__MSYS__) || defined(__CYGWIN__) || defined(__MINGW32__) ||        \
    defined(__MINGW64__)
  // These platforms lack a child-directory file action, so we change the
  // process-wide working directory around the spawn and restore it afterwards
  // This races with any concurrent thread that observes or mutates the current
  // directory while the spawn is in flight
  const std::filesystem::path original_directory{
      std::filesystem::current_path()};
  std::filesystem::current_path(directory);
#else
  // The standardized child-directory file action is not yet provided by every
  // system and toolchain this builds on, so we keep using the long-standing
  // platform extension and silence the deprecation that newer SDKs attach to it
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  const int addchdir_result{
      posix_spawn_file_actions_addchdir_np(&file_actions, directory.c_str())};
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
                   const_cast<char *const *>(argv.data()), environ)};

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

  int status{0};
  while (waitpid(process_id, &status, 0) == -1) {
    if (errno == EINTR) {
      continue;
    }

    throw ProcessSpawnError{program, arguments};
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  throw ProcessSpawnError{program, arguments};
#endif
}

auto spawn(const std::string &program,
           std::initializer_list<std::string_view> arguments,
           const std::filesystem::path &directory) -> int {
  return spawn(
      program,
      std::span<const std::string_view>{arguments.begin(), arguments.size()},
      directory);
}

} // namespace sourcemeta::core
