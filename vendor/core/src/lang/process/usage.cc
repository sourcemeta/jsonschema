#include <sourcemeta/core/process.h>

#include <chrono> // std::chrono::duration, std::chrono::duration_cast, std::chrono::microseconds, std::chrono::nanoseconds, std::chrono::seconds, std::chrono::system_clock
#include <cstdint>  // std::int64_t, std::uint64_t
#include <optional> // std::optional

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h> // GetCurrentProcess, GetProcessTimes, GetProcessHandleCount, FILETIME, ULARGE_INTEGER, DWORD

// Declared in terms of what the header above brings in, so it stays in a block
// of its own rather than being sorted ahead of it
#include <psapi.h> // GetProcessMemoryInfo, PROCESS_MEMORY_COUNTERS
#elif defined(__APPLE__)
#include <cstddef> // std::size_t
#include <cstdlib> // std::malloc, std::free

#include <libproc.h> // proc_pidinfo, proc_bsdinfo, PROC_PIDLISTFDS, PROC_PIDLISTFD_SIZE, PROC_PIDTBSDINFO, PROC_PIDTBSDINFO_SIZE
#include <mach/mach.h> // mach_task_self, task_info, task_info_t, mach_task_basic_info, mach_msg_type_number_t, MACH_TASK_BASIC_INFO, MACH_TASK_BASIC_INFO_COUNT, KERN_SUCCESS
#include <sys/resource.h> // getrusage, getrlimit, rusage, rlimit, RUSAGE_SELF, RLIMIT_NOFILE, RLIM_INFINITY
#include <unistd.h>       // getpid
#elif defined(__linux__)
#include <sourcemeta/core/numeric.h>

#include <array>       // std::array
#include <cerrno>      // EINTR, errno
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

#include <dirent.h>       // opendir, readdir, closedir, DIR, dirent
#include <fcntl.h>        // open, O_RDONLY, O_CLOEXEC
#include <sys/resource.h> // getrlimit, rlimit, RLIMIT_NOFILE, RLIM_INFINITY
#include <unistd.h>       // read, close, sysconf, _SC_CLK_TCK, _SC_PAGESIZE
#endif

#if defined(_WIN32) && !defined(__MSYS__) && !defined(__CYGWIN__) &&           \
    !defined(__MINGW32__) && !defined(__MINGW64__)

namespace {

// The platform counts time in hundreds of nanoseconds
constexpr double UNITS_PER_SECOND{10000000.0};
constexpr std::int64_t NANOSECONDS_PER_UNIT{100};

// The platform counts from the start of the year 1601, which is this many of
// its own units before the Unix epoch
constexpr std::uint64_t EPOCH_OFFSET_UNITS{11644473600ULL * 10000000ULL};

// The class of process information that carries the size of the mapped address
// space, which no documented interface answers with
constexpr ULONG VM_COUNTERS_INFORMATION_CLASS{3};

// What that class answers with, mirroring the kernel's VM_COUNTERS. It is
// absent from the public headers, so the layout is spelled out here. Only the
// second member is read, and everything before it has held its place since
// Windows NT
struct VirtualMemoryCounters {
  SIZE_T peak_virtual_size;
  SIZE_T virtual_size;
  ULONG page_fault_count;
  SIZE_T peak_working_set_size;
  SIZE_T working_set_size;
  SIZE_T quota_peak_paged_pool_usage;
  SIZE_T quota_paged_pool_usage;
  SIZE_T quota_peak_non_paged_pool_usage;
  SIZE_T quota_non_paged_pool_usage;
  SIZE_T pagefile_usage;
  SIZE_T peak_pagefile_usage;
};

using QueryProcessInformation = LONG(NTAPI *)(HANDLE, ULONG, PVOID, ULONG,
                                              PULONG);

auto to_units(const FILETIME &value) noexcept -> std::uint64_t {
  ULARGE_INTEGER converted{};
  converted.LowPart = value.dwLowDateTime;
  converted.HighPart = value.dwHighDateTime;
  return converted.QuadPart;
}

// Resolved once rather than on every call. Looking a module up takes the
// loader lock, and a reading may be taken on a thread that other work shares.
// The address of an export is fixed for the lifetime of the process, so this
// remembers a constant rather than a measurement
auto query_process_information() noexcept -> QueryProcessInformation {
  static const auto entry{reinterpret_cast<QueryProcessInformation>(
      reinterpret_cast<void *>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),
                                              "NtQueryInformationProcess")))};
  return entry;
}

// The documented memory interface answers with the commit charge, which leaves
// out file backed mappings and reserved regions and so is a different quantity
// from what the other platforms report. This asks the undocumented interface
// that does answer with the mapped address space, and says nothing at all
// where it is unavailable
auto mapped_address_space() noexcept -> std::optional<std::uint64_t> {
  const auto entry{query_process_information()};
  if (entry == nullptr) {
    return std::nullopt;
  }

  VirtualMemoryCounters counters{};
  if (entry(GetCurrentProcess(), VM_COUNTERS_INFORMATION_CLASS, &counters,
            static_cast<ULONG>(sizeof(counters)), nullptr) < 0) {
    return std::nullopt;
  }

  return counters.virtual_size;
}

} // namespace

namespace sourcemeta::core {

auto process_usage() noexcept -> ProcessUsage {
  ProcessUsage result;

  FILETIME creation{};
  FILETIME termination{};
  FILETIME kernel{};
  FILETIME user{};
  if (GetProcessTimes(GetCurrentProcess(), &creation, &termination, &kernel,
                      &user) != 0) {
    result.cpu_time = std::chrono::nanoseconds{
        static_cast<std::int64_t>(to_units(kernel) + to_units(user)) *
        NANOSECONDS_PER_UNIT};
  }

  PROCESS_MEMORY_COUNTERS counters{};
  counters.cb = sizeof(counters);
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)) !=
      0) {
    result.resident_bytes = counters.WorkingSetSize;
  }

  result.virtual_bytes = mapped_address_space();
  return result;
}

auto process_descriptors() noexcept -> ProcessDescriptors {
  ProcessDescriptors result;

  DWORD handles{0};
  if (GetProcessHandleCount(GetCurrentProcess(), &handles) != 0) {
    result.open = handles;
  }

  // The platform enforces no per-process ceiling comparable to the POSIX one,
  // so there is nothing true to say about the maximum
  return result;
}

auto process_start_time() noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  FILETIME creation{};
  FILETIME termination{};
  FILETIME kernel{};
  FILETIME user{};
  if (GetProcessTimes(GetCurrentProcess(), &creation, &termination, &kernel,
                      &user) == 0) {
    return std::nullopt;
  }

  const auto units{to_units(creation)};
  if (units < EPOCH_OFFSET_UNITS) {
    return std::nullopt;
  }

  const std::chrono::duration<double> since_epoch{
      static_cast<double>(units - EPOCH_OFFSET_UNITS) / UNITS_PER_SECOND};
  return std::chrono::system_clock::time_point{
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          since_epoch)};
}

} // namespace sourcemeta::core

#elif defined(__APPLE__)

namespace {

auto descriptor_ceiling() noexcept -> std::optional<std::uint64_t> {
  rlimit limit{};
  if (getrlimit(RLIMIT_NOFILE, &limit) != 0 ||
      limit.rlim_cur == RLIM_INFINITY) {
    return std::nullopt;
  }

  return static_cast<std::uint64_t>(limit.rlim_cur);
}

} // namespace

namespace sourcemeta::core {

auto process_usage() noexcept -> ProcessUsage {
  ProcessUsage result;

  rusage consumed{};
  if (getrusage(RUSAGE_SELF, &consumed) == 0) {
    result.cpu_time =
        std::chrono::seconds{static_cast<std::chrono::seconds::rep>(
            consumed.ru_utime.tv_sec + consumed.ru_stime.tv_sec)} +
        std::chrono::microseconds{static_cast<std::chrono::microseconds::rep>(
            consumed.ru_utime.tv_usec + consumed.ru_stime.tv_usec)};
  }

  mach_task_basic_info information{};
  mach_msg_type_number_t count{MACH_TASK_BASIC_INFO_COUNT};
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&information),
                &count) == KERN_SUCCESS) {
    result.resident_bytes = information.resident_size;
    result.virtual_bytes = information.virtual_size;
  }

  return result;
}

auto process_descriptors() noexcept -> ProcessDescriptors {
  ProcessDescriptors result;

  result.maximum = descriptor_ceiling();

  // Asking for no bytes answers with room for the highest descriptor in use
  // plus a margin, which is not a count of what is open, so the listing has to
  // be taken and measured by what it actually fills. Taking it opens nothing,
  // so there is no entry of its own to discount
  const auto capacity{proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, nullptr, 0)};
  if (capacity <= 0) {
    return result;
  }

  // The only platform where anything here has to reach for the heap, and it
  // is done without the standard containers so that running out of memory
  // stays a returned absence rather than an exception this cannot throw
  auto *listing{std::malloc(static_cast<std::size_t>(capacity))};
  if (listing == nullptr) {
    return result;
  }

  const auto written{
      proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, listing, capacity)};
  std::free(listing);
  if (written > 0) {
    result.open = static_cast<std::uint64_t>(written) / PROC_PIDLISTFD_SIZE;
  }

  return result;
}

auto process_start_time() noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  proc_bsdinfo information{};
  if (proc_pidinfo(getpid(), PROC_PIDTBSDINFO, 0, &information,
                   PROC_PIDTBSDINFO_SIZE) != PROC_PIDTBSDINFO_SIZE) {
    return std::nullopt;
  }

  const std::chrono::duration<double> since_epoch{
      static_cast<double>(information.pbi_start_tvsec) +
      static_cast<double>(information.pbi_start_tvusec) / 1000000.0};
  return std::chrono::system_clock::time_point{
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          since_epoch)};
}

} // namespace sourcemeta::core

#elif defined(__linux__)

namespace {

// Bounded by what the kernel writes, which is a handful of fields wider than
// the last one anything here reads
constexpr std::size_t STAT_BUFFER_SIZE{1024};

// One past the highest index this reads, being the resident set size
constexpr std::size_t STAT_FIELD_COUNT{22};

// As wide as an unsigned 64-bit value ever spells out to
constexpr std::size_t DIGIT_BUFFER_SIZE{20};

auto read_bounded(const char *path, char *buffer,
                  const std::size_t size) noexcept
    -> std::optional<std::string_view> {
  const auto descriptor{open(path, O_RDONLY | O_CLOEXEC)};
  if (descriptor < 0) {
    return std::nullopt;
  }

  std::size_t total{0};
  while (total < size) {
    const auto count{read(descriptor, buffer + total, size - total)};
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }

      close(descriptor);
      return std::nullopt;
    }

    if (count == 0) {
      break;
    }

    total += static_cast<std::size_t>(count);
  }

  close(descriptor);
  return std::string_view{buffer, total};
}

// The second field is a command name in parentheses that may itself contain
// spaces, so what follows it is found from the last parenthesis rather than by
// counting separators from the beginning. See proc(5)
auto stat_fields(const std::string_view line) noexcept
    -> std::array<std::string_view, STAT_FIELD_COUNT> {
  std::array<std::string_view, STAT_FIELD_COUNT> result{};
  const auto command{line.rfind(')')};
  if (command == std::string_view::npos) {
    return result;
  }

  auto cursor{command + 1};
  std::size_t index{0};
  while (cursor < line.size() && index < result.size()) {
    while (cursor < line.size() && line[cursor] == ' ') {
      cursor += 1;
    }

    const auto start{cursor};
    while (cursor < line.size() && line[cursor] != ' ' &&
           line[cursor] != '\n') {
      cursor += 1;
    }

    if (cursor == start) {
      break;
    }

    result[index] = line.substr(start, cursor - start);
    index += 1;
  }

  return result;
}

// The file is too wide to hold in a bounded buffer on a machine with many
// interrupt sources, so the boot time is picked out of it as it streams past
auto boot_time_seconds() noexcept -> std::optional<std::uint64_t> {
  static constexpr std::string_view PREFIX{"btime "};
  const auto descriptor{open("/proc/stat", O_RDONLY | O_CLOEXEC)};
  if (descriptor < 0) {
    return std::nullopt;
  }

  std::array<char, STAT_BUFFER_SIZE> buffer{};
  std::array<char, DIGIT_BUFFER_SIZE> digits{};
  std::size_t length{0};
  std::size_t matched{0};
  bool skipping{false};
  bool collecting{false};
  bool done{false};

  while (!done) {
    const auto count{read(descriptor, buffer.data(), buffer.size())};
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }

      close(descriptor);
      return std::nullopt;
    }

    if (count == 0) {
      break;
    }

    for (std::size_t index = 0; index < static_cast<std::size_t>(count);
         index++) {
      const auto character{buffer[index]};
      if (collecting) {
        if (character < '0' || character > '9' || length == digits.size()) {
          done = true;
          break;
        }

        digits[length] = character;
        length += 1;
      } else if (skipping) {
        skipping = character != '\n';
      } else if (character == '\n') {
        matched = 0;
      } else if (character == PREFIX[matched]) {
        matched += 1;
        collecting = matched == PREFIX.size();
      } else {
        skipping = true;
        matched = 0;
      }
    }
  }

  close(descriptor);
  if (length == 0) {
    return std::nullopt;
  }

  return sourcemeta::core::to_uint64_t(std::string_view{digits.data(), length});
}

auto descriptor_ceiling() noexcept -> std::optional<std::uint64_t> {
  rlimit limit{};
  if (getrlimit(RLIMIT_NOFILE, &limit) != 0 ||
      limit.rlim_cur == RLIM_INFINITY) {
    return std::nullopt;
  }

  return static_cast<std::uint64_t>(limit.rlim_cur);
}

} // namespace

namespace sourcemeta::core {

auto process_usage() noexcept -> ProcessUsage {
  ProcessUsage result;

  std::array<char, STAT_BUFFER_SIZE> buffer{};
  const auto line{
      read_bounded("/proc/self/stat", buffer.data(), buffer.size())};
  if (!line.has_value()) {
    return result;
  }

  const auto fields{stat_fields(line.value())};

  const auto ticks{sysconf(_SC_CLK_TCK)};
  const auto user{to_uint64_t(fields[11])};
  const auto system{to_uint64_t(fields[12])};
  if (ticks > 0 && user.has_value() && system.has_value()) {
    // Split rather than scaled whole, so that a rate that does not divide a
    // second evenly still converts exactly and nothing overflows on the way
    const auto total{static_cast<std::int64_t>(user.value() + system.value())};
    const auto rate{static_cast<std::int64_t>(ticks)};
    result.cpu_time =
        std::chrono::seconds{total / rate} +
        std::chrono::nanoseconds{(total % rate) * 1000000000 / rate};
  }

  result.virtual_bytes = to_uint64_t(fields[20]);

  const auto pages{to_uint64_t(fields[21])};
  const auto page_size{sysconf(_SC_PAGESIZE)};
  if (pages.has_value() && page_size > 0) {
    result.resident_bytes =
        pages.value() * static_cast<std::uint64_t>(page_size);
  }

  return result;
}

auto process_descriptors() noexcept -> ProcessDescriptors {
  ProcessDescriptors result;

  DIR *listing{opendir("/proc/self/fd")};
  if (listing != nullptr) {
    std::uint64_t entries{0};
    for (const dirent *entry{readdir(listing)}; entry != nullptr;
         entry = readdir(listing)) {
      const std::string_view name{entry->d_name};
      if (name != "." && name != "..") {
        entries += 1;
      }
    }

    closedir(listing);
    // Reading the list takes a descriptor of its own, which the list then
    // includes, so what is counted is one more than what was open
    result.open = entries > 0 ? entries - 1 : 0;
  }

  result.maximum = descriptor_ceiling();
  return result;
}

auto process_start_time() noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  const auto boot{boot_time_seconds()};
  if (!boot.has_value()) {
    return std::nullopt;
  }

  const auto ticks{sysconf(_SC_CLK_TCK)};
  if (ticks <= 0) {
    return std::nullopt;
  }

  std::array<char, STAT_BUFFER_SIZE> buffer{};
  const auto line{
      read_bounded("/proc/self/stat", buffer.data(), buffer.size())};
  if (!line.has_value()) {
    return std::nullopt;
  }

  const auto fields{stat_fields(line.value())};
  const auto started{to_uint64_t(fields[19])};
  if (!started.has_value()) {
    return std::nullopt;
  }

  const std::chrono::duration<double> since_epoch{
      static_cast<double>(boot.value()) +
      static_cast<double>(started.value()) / static_cast<double>(ticks)};
  return std::chrono::system_clock::time_point{
      std::chrono::duration_cast<std::chrono::system_clock::duration>(
          since_epoch)};
}

} // namespace sourcemeta::core

#else

namespace sourcemeta::core {

auto process_usage() noexcept -> ProcessUsage { return {}; }

auto process_descriptors() noexcept -> ProcessDescriptors { return {}; }

auto process_start_time() noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  return std::nullopt;
}

} // namespace sourcemeta::core

#endif
