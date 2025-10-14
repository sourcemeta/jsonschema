#include <sourcemeta/core/process.h>

#include <cassert>          // assert
#include <filesystem>       // std::filesystem
#include <initializer_list> // std::initializer_list
#include <span>             // std::span
#include <vector>           // std::vector

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
#define WIN32_LEAN_AND_MEAN
#include <sstream>   // std::ostringstream
#include <windows.h> // CreateProcess, PROCESS_INFORMATION, STARTUPINFO, WaitForSingleObject, GetExitCodeProcess
#else
#include <spawn.h> // posix_spawnp, posix_spawnattr_t, posix_spawnattr_init, posix_spawnattr_destroy, posix_spawn_file_actions_t, posix_spawn_file_actions_init, posix_spawn_file_actions_destroy, pid_t
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS

#if defined(__MSYS__) || defined(__CYGWIN__) || defined(__MINGW32__) ||        \
    defined(__MINGW64__)
#include <unistd.h> // chdir
#endif

extern char **environ;
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
  std::ostringstream command_line;
  command_line << program;

  for (const auto &argument : arguments) {
    command_line << " ";
    // Quote arguments that contain spaces
    const std::string arg_str{argument};
    if (arg_str.find(' ') != std::string::npos) {
      command_line << "\"" << arg_str << "\"";
    } else {
      command_line << arg_str;
    }
  }

  std::string cmd_line_str = command_line.str();
  std::vector<char> cmd_line(cmd_line_str.begin(), cmd_line_str.end());
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
    throw ProcessProgramNotNotFoundError{program};
  }

  WaitForSingleObject(process_info.hProcess, INFINITE);

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
  std::vector<const char *> argv;
  argv.reserve(arguments.size() + 2);
  argv.push_back(program.c_str());

  for (const auto &argument : arguments) {
    argv.push_back(argument.data());
  }

  argv.push_back(nullptr);

  posix_spawnattr_t attributes;
  posix_spawnattr_init(&attributes);

  posix_spawn_file_actions_t file_actions;
  posix_spawn_file_actions_init(&file_actions);

#if defined(__MSYS__) || defined(__CYGWIN__) || defined(__MINGW32__) ||        \
    defined(__MINGW64__)
  const std::filesystem::path original_directory{
      std::filesystem::current_path()};
  std::filesystem::current_path(directory);
#else
  posix_spawn_file_actions_addchdir_np(&file_actions, directory.c_str());
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
    throw ProcessProgramNotNotFoundError{program};
  }

  int status;
  waitpid(process_id, &status, 0);

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
