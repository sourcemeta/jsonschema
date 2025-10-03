if(NOT Archive_FOUND)

  set(ARCHIVE_DIR "${PROJECT_SOURCE_DIR}/vendor/libarchive/libarchive")
  set(ARCHIVE_PUBLIC_HEADER "${ARCHIVE_DIR}/archive.h")
  set(ARCHIVE_PRIVATE_HEADERS
    "${ARCHIVE_DIR}/archive_entry.h")

  add_library(archive
    "${ARCHIVE_PUBLIC_HEADER}" ${ARCHIVE_PRIVATE_HEADERS}

    "${ARCHIVE_DIR}/archive_acl.c"
    "${ARCHIVE_DIR}/archive_acl_private.h"
    "${ARCHIVE_DIR}/archive_check_magic.c"
    "${ARCHIVE_DIR}/archive_cmdline.c"
    "${ARCHIVE_DIR}/archive_cmdline_private.h"
    "${ARCHIVE_DIR}/archive_crc32.h"
    "${ARCHIVE_DIR}/archive_cryptor.c"
    "${ARCHIVE_DIR}/archive_cryptor_private.h"
    "${ARCHIVE_DIR}/archive_digest.c"
    "${ARCHIVE_DIR}/archive_digest_private.h"
    "${ARCHIVE_DIR}/archive_endian.h"
    "${ARCHIVE_DIR}/archive_entry.c"
    "${ARCHIVE_DIR}/archive_entry_copy_stat.c"
    "${ARCHIVE_DIR}/archive_entry_link_resolver.c"
    "${ARCHIVE_DIR}/archive_entry_locale.h"
    "${ARCHIVE_DIR}/archive_entry_private.h"
    "${ARCHIVE_DIR}/archive_entry_sparse.c"
    "${ARCHIVE_DIR}/archive_entry_stat.c"
    "${ARCHIVE_DIR}/archive_entry_strmode.c"
    "${ARCHIVE_DIR}/archive_entry_xattr.c"
    "${ARCHIVE_DIR}/archive_hmac.c"
    "${ARCHIVE_DIR}/archive_hmac_private.h"
    "${ARCHIVE_DIR}/archive_match.c"
    "${ARCHIVE_DIR}/archive_openssl_evp_private.h"
    "${ARCHIVE_DIR}/archive_openssl_hmac_private.h"
    "${ARCHIVE_DIR}/archive_options.c"
    "${ARCHIVE_DIR}/archive_options_private.h"
    "${ARCHIVE_DIR}/archive_pack_dev.h"
    "${ARCHIVE_DIR}/archive_pack_dev.c"
    "${ARCHIVE_DIR}/archive_parse_date.c"
    "${ARCHIVE_DIR}/archive_pathmatch.c"
    "${ARCHIVE_DIR}/archive_pathmatch.h"
    "${ARCHIVE_DIR}/archive_platform.h"
    "${ARCHIVE_DIR}/archive_platform_acl.h"
    "${ARCHIVE_DIR}/archive_platform_xattr.h"
    "${ARCHIVE_DIR}/archive_ppmd_private.h"
    "${ARCHIVE_DIR}/archive_ppmd8.c"
    "${ARCHIVE_DIR}/archive_ppmd8_private.h"
    "${ARCHIVE_DIR}/archive_ppmd7.c"
    "${ARCHIVE_DIR}/archive_ppmd7_private.h"
    "${ARCHIVE_DIR}/archive_private.h"
    "${ARCHIVE_DIR}/archive_random.c"
    "${ARCHIVE_DIR}/archive_random_private.h"
    "${ARCHIVE_DIR}/archive_rb.c"
    "${ARCHIVE_DIR}/archive_rb.h"
    "${ARCHIVE_DIR}/archive_read.c"
    "${ARCHIVE_DIR}/archive_read_add_passphrase.c"
    "${ARCHIVE_DIR}/archive_read_append_filter.c"
    "${ARCHIVE_DIR}/archive_read_data_into_fd.c"
    "${ARCHIVE_DIR}/archive_read_disk_entry_from_file.c"
    "${ARCHIVE_DIR}/archive_read_disk_posix.c"
    "${ARCHIVE_DIR}/archive_read_disk_private.h"
    "${ARCHIVE_DIR}/archive_read_disk_set_standard_lookup.c"
    "${ARCHIVE_DIR}/archive_read_extract.c"
    "${ARCHIVE_DIR}/archive_read_extract2.c"
    "${ARCHIVE_DIR}/archive_read_open_fd.c"
    "${ARCHIVE_DIR}/archive_read_open_file.c"
    "${ARCHIVE_DIR}/archive_read_open_filename.c"
    "${ARCHIVE_DIR}/archive_read_open_memory.c"
    "${ARCHIVE_DIR}/archive_read_private.h"
    "${ARCHIVE_DIR}/archive_read_set_format.c"
    "${ARCHIVE_DIR}/archive_read_set_options.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_all.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_by_code.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_bzip2.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_compress.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_gzip.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_grzip.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_lrzip.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_lz4.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_lzop.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_none.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_program.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_rpm.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_uu.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_xz.c"
    "${ARCHIVE_DIR}/archive_read_support_filter_zstd.c"
    "${ARCHIVE_DIR}/archive_read_support_format_7zip.c"
    "${ARCHIVE_DIR}/archive_read_support_format_all.c"
    "${ARCHIVE_DIR}/archive_read_support_format_ar.c"
    "${ARCHIVE_DIR}/archive_read_support_format_by_code.c"
    "${ARCHIVE_DIR}/archive_read_support_format_cab.c"
    "${ARCHIVE_DIR}/archive_read_support_format_cpio.c"
    "${ARCHIVE_DIR}/archive_read_support_format_empty.c"
    "${ARCHIVE_DIR}/archive_read_support_format_iso9660.c"
    "${ARCHIVE_DIR}/archive_read_support_format_lha.c"
    "${ARCHIVE_DIR}/archive_read_support_format_mtree.c"
    "${ARCHIVE_DIR}/archive_read_support_format_rar.c"
    "${ARCHIVE_DIR}/archive_read_support_format_rar5.c"
    "${ARCHIVE_DIR}/archive_read_support_format_raw.c"
    "${ARCHIVE_DIR}/archive_read_support_format_tar.c"
    "${ARCHIVE_DIR}/archive_read_support_format_warc.c"
    "${ARCHIVE_DIR}/archive_read_support_format_xar.c"
    "${ARCHIVE_DIR}/archive_read_support_format_zip.c"
    "${ARCHIVE_DIR}/archive_string.c"
    "${ARCHIVE_DIR}/archive_string.h"
    "${ARCHIVE_DIR}/archive_string_composition.h"
    "${ARCHIVE_DIR}/archive_string_sprintf.c"
    "${ARCHIVE_DIR}/archive_time.c"
    "${ARCHIVE_DIR}/archive_time_private.h"
    "${ARCHIVE_DIR}/archive_util.c"
    "${ARCHIVE_DIR}/archive_version_details.c"
    "${ARCHIVE_DIR}/archive_virtual.c"
    "${ARCHIVE_DIR}/archive_write.c"
    "${ARCHIVE_DIR}/archive_write_disk_posix.c"
    "${ARCHIVE_DIR}/archive_write_disk_private.h"
    "${ARCHIVE_DIR}/archive_write_disk_set_standard_lookup.c"
    "${ARCHIVE_DIR}/archive_write_private.h"
    "${ARCHIVE_DIR}/archive_write_open_fd.c"
    "${ARCHIVE_DIR}/archive_write_open_file.c"
    "${ARCHIVE_DIR}/archive_write_open_filename.c"
    "${ARCHIVE_DIR}/archive_write_open_memory.c"
    "${ARCHIVE_DIR}/archive_write_add_filter.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_b64encode.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_by_name.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_bzip2.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_compress.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_grzip.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_gzip.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_lrzip.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_lz4.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_lzop.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_none.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_program.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_uuencode.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_xz.c"
    "${ARCHIVE_DIR}/archive_write_add_filter_zstd.c"
    "${ARCHIVE_DIR}/archive_write_set_format.c"
    "${ARCHIVE_DIR}/archive_write_set_format_7zip.c"
    "${ARCHIVE_DIR}/archive_write_set_format_ar.c"
    "${ARCHIVE_DIR}/archive_write_set_format_by_name.c"
    "${ARCHIVE_DIR}/archive_write_set_format_cpio.c"
    "${ARCHIVE_DIR}/archive_write_set_format_cpio_binary.c"
    "${ARCHIVE_DIR}/archive_write_set_format_cpio_newc.c"
    "${ARCHIVE_DIR}/archive_write_set_format_cpio_odc.c"
    "${ARCHIVE_DIR}/archive_write_set_format_filter_by_ext.c"
    "${ARCHIVE_DIR}/archive_write_set_format_gnutar.c"
    "${ARCHIVE_DIR}/archive_write_set_format_iso9660.c"
    "${ARCHIVE_DIR}/archive_write_set_format_mtree.c"
    "${ARCHIVE_DIR}/archive_write_set_format_pax.c"
    "${ARCHIVE_DIR}/archive_write_set_format_private.h"
    "${ARCHIVE_DIR}/archive_write_set_format_raw.c"
    "${ARCHIVE_DIR}/archive_write_set_format_shar.c"
    "${ARCHIVE_DIR}/archive_write_set_format_ustar.c"
    "${ARCHIVE_DIR}/archive_write_set_format_v7tar.c"
    "${ARCHIVE_DIR}/archive_write_set_format_warc.c"
    "${ARCHIVE_DIR}/archive_write_set_format_xar.c"
    "${ARCHIVE_DIR}/archive_write_set_format_zip.c"
    "${ARCHIVE_DIR}/archive_write_set_options.c"
    "${ARCHIVE_DIR}/archive_write_set_passphrase.c"
    "${ARCHIVE_DIR}/archive_xxhash.h"
    "${ARCHIVE_DIR}/filter_fork_posix.c"
    "${ARCHIVE_DIR}/filter_fork.h"
    "${ARCHIVE_DIR}/xxhash.c"

    "${ARCHIVE_DIR}/archive_blake2.h"
    "${ARCHIVE_DIR}/archive_blake2_impl.h"
    "${ARCHIVE_DIR}/archive_blake2s_ref.c"
    "${ARCHIVE_DIR}/archive_blake2sp_ref.c"

    # Windows
    "${ARCHIVE_DIR}/archive_entry_copy_bhfi.c"
    "${ARCHIVE_DIR}/archive_read_disk_windows.c"
    "${ARCHIVE_DIR}/archive_windows.c"
    "${ARCHIVE_DIR}/archive_windows.h"
    "${ARCHIVE_DIR}/archive_write_disk_windows.c"
    "${ARCHIVE_DIR}/filter_fork_windows.c"

    # ACL
    "${ARCHIVE_DIR}/archive_disk_acl_darwin.c"
    "${ARCHIVE_DIR}/archive_disk_acl_freebsd.c"
    "${ARCHIVE_DIR}/archive_disk_acl_linux.c"
    "${ARCHIVE_DIR}/archive_disk_acl_sunos.c")

  # Run the CMake configure step in the upstream `CMakeLists.txt` and check `config.h`
  if(APPLE)
    set(HAVE_INT16_T YES)
    set(HAVE_INT32_T YES)
    set(HAVE_INT64_T YES)
    set(HAVE_INTMAX_T YES)
    set(HAVE_UINT8_T YES)
    set(HAVE_UINT16_T YES)
    set(HAVE_UINT32_T YES)
    set(HAVE_UINT64_T YES)
    set(HAVE_UINTMAX_T YES)
    set(ARCHIVE_ACL_DARWIN YES)
    set(ARCHIVE_CRYPTO_MD5_LIBSYSTEM YES)
    set(ARCHIVE_CRYPTO_SHA1_LIBSYSTEM YES)
    set(ARCHIVE_CRYPTO_SHA256_LIBSYSTEM YES)
    set(ARCHIVE_CRYPTO_SHA384_LIBSYSTEM YES)
    set(ARCHIVE_CRYPTO_SHA512_LIBSYSTEM YES)
    set(ARCHIVE_XATTR_DARWIN YES)
    set(HAVE_ACL_CREATE_ENTRY YES)
    set(HAVE_ACL_GET_FD_NP YES)
    set(HAVE_ACL_GET_LINK_NP YES)
    set(HAVE_ACL_GET_PERM_NP YES)
    set(HAVE_ACL_INIT YES)
    set(HAVE_ACL_PERMSET_T YES)
    set(HAVE_ACL_SET_FD YES)
    set(HAVE_ACL_SET_FD_NP YES)
    set(HAVE_ACL_SET_FILE YES)
    set(HAVE_ARC4RANDOM_BUF YES)
    set(HAVE_CHFLAGS YES)
    set(HAVE_CHOWN YES)
    set(HAVE_CHROOT YES)
    set(HAVE_COPYFILE_H YES)
    set(HAVE_CTIME_R YES)
    set(HAVE_CTYPE_H YES)
    set(HAVE_DECL_ACL_SYNCHRONIZE YES)
    set(HAVE_DECL_ACL_TYPE_EXTENDED YES)
    set(HAVE_DECL_INT32_MAX YES)
    set(HAVE_DECL_INT32_MIN YES)
    set(HAVE_DECL_INT64_MAX YES)
    set(HAVE_DECL_INT64_MIN YES)
    set(HAVE_DECL_INTMAX_MAX YES)
    set(HAVE_DECL_INTMAX_MIN YES)
    set(HAVE_DECL_SIZE_MAX YES)
    set(HAVE_DECL_SSIZE_MAX YES)
    set(HAVE_DECL_STRERROR_R YES)
    set(HAVE_DECL_UINT32_MAX YES)
    set(HAVE_DECL_UINT64_MAX YES)
    set(HAVE_DECL_UINTMAX_MAX YES)
    set(HAVE_DECL_XATTR_NOFOLLOW YES)
    set(HAVE_DIRENT_H YES)
    set(HAVE_DIRFD YES)
    set(HAVE_DLFCN_H YES)
    set(HAVE_D_MD_ORDER YES)
    set(HAVE_EFTYPE YES)
    set(HAVE_EILSEQ YES)
    set(HAVE_ERRNO_H YES)
    set(HAVE_FCHDIR YES)
    set(HAVE_FCHFLAGS YES)
    set(HAVE_FCHMOD YES)
    set(HAVE_FCHOWN YES)
    set(HAVE_FCNTL YES)
    set(HAVE_FCNTL_H YES)
    set(HAVE_FDOPENDIR YES)
    set(HAVE_FGETXATTR YES)
    set(HAVE_FLISTXATTR YES)
    set(HAVE_FNMATCH YES)
    set(HAVE_FNMATCH_H YES)
    set(HAVE_FORK YES)
    set(HAVE_FSEEKO YES)
    set(HAVE_FSETXATTR YES)
    set(HAVE_FSTAT YES)
    set(HAVE_FSTATAT YES)
    set(HAVE_FSTATFS YES)
    set(HAVE_FSTATVFS YES)
    set(HAVE_FTRUNCATE YES)
    set(HAVE_FUTIMENS YES)
    set(HAVE_FUTIMES YES)
    set(HAVE_GETEUID YES)
    set(HAVE_GETGRGID_R YES)
    set(HAVE_GETGRNAM_R YES)
    set(HAVE_GETLINE YES)
    set(HAVE_GETPID YES)
    set(HAVE_GETPWNAM_R YES)
    set(HAVE_GETPWUID_R YES)
    set(HAVE_GETVFSBYNAME YES)
    set(HAVE_GETXATTR YES)
    set(HAVE_GMTIME_R YES)
    set(HAVE_GRP_H YES)
    set(HAVE_INTTYPES_H YES)
    set(HAVE_LANGINFO_H YES)
    set(HAVE_LCHFLAGS YES)
    set(HAVE_LCHMOD YES)
    set(HAVE_LCHOWN YES)
    set(HAVE_LIMITS_H YES)
    set(HAVE_LINK YES)
    set(HAVE_LINKAT YES)
    set(HAVE_LISTXATTR YES)
    set(HAVE_LOCALCHARSET_H YES)
    set(HAVE_LOCALE_CHARSET YES)
    set(HAVE_LOCALE_H YES)
    set(HAVE_LOCALTIME_R YES)
    set(HAVE_LSTAT YES)
    set(HAVE_LUTIMES YES)
    set(HAVE_MBRTOWC YES)
    set(HAVE_MEMBERSHIP_H YES)
    set(HAVE_MEMMOVE YES)
    set(HAVE_MEMORY_H YES)
    set(HAVE_MKDIR YES)
    set(HAVE_MKFIFO YES)
    set(HAVE_MKNOD YES)
    set(HAVE_MKSTEMP YES)
    set(HAVE_NL_LANGINFO YES)
    set(HAVE_OPENAT YES)
    set(HAVE_PATHS_H YES)
    set(HAVE_PIPE YES)
    set(HAVE_POLL YES)
    set(HAVE_POLL_H YES)
    set(HAVE_POSIX_SPAWNP YES)
    set(HAVE_PTHREAD_H YES)
    set(HAVE_PWD_H YES)
    set(HAVE_READDIR_R YES)
    set(HAVE_READLINK YES)
    set(HAVE_READLINKAT YES)
    set(HAVE_READPASSPHRASE YES)
    set(HAVE_READPASSPHRASE_H YES)
    set(HAVE_REGEX_H YES)
    set(HAVE_SELECT YES)
    set(HAVE_SETENV YES)
    set(HAVE_SETLOCALE YES)
    set(HAVE_SIGACTION YES)
    set(HAVE_SIGNAL_H YES)
    set(HAVE_SPAWN_H YES)
    set(HAVE_STATFS YES)
    set(HAVE_STATVFS YES)
    set(HAVE_STDARG_H YES)
    set(HAVE_STDINT_H YES)
    set(HAVE_STDLIB_H YES)
    set(HAVE_STRCHR YES)
    set(HAVE_STRNLEN YES)
    set(HAVE_STRDUP YES)
    set(HAVE_STRERROR YES)
    set(HAVE_STRERROR_R YES)
    set(HAVE_STRFTIME YES)
    set(HAVE_STRINGS_H YES)
    set(HAVE_STRING_H YES)
    set(HAVE_STRRCHR YES)
    set(HAVE_STRUCT_STATFS YES)
    set(HAVE_STRUCT_STATFS_F_IOSIZE YES)
    set(HAVE_STRUCT_STAT_ST_BIRTHTIME YES)
    set(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC YES)
    set(HAVE_STRUCT_STAT_ST_BLKSIZE YES)
    set(HAVE_STRUCT_STAT_ST_FLAGS YES)
    set(HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC YES)
    set(HAVE_STRUCT_TM_TM_GMTOFF YES)
    set(HAVE_STRUCT_VFSCONF YES)
    set(HAVE_SYMLINK YES)
    set(HAVE_SYSCONF YES)
    set(HAVE_SYS_ACL_H YES)
    set(HAVE_SYS_CDEFS_H YES)
    set(HAVE_SYS_IOCTL_H YES)
    set(HAVE_SYS_MOUNT_H YES)
    set(HAVE_SYS_PARAM_H YES)
    set(HAVE_SYS_POLL_H YES)
    set(HAVE_SYS_SELECT_H YES)
    set(HAVE_SYS_STATVFS_H YES)
    set(HAVE_SYS_STAT_H YES)
    set(HAVE_SYS_TIME_H YES)
    set(HAVE_SYS_TYPES_H YES)
    set(HAVE_SYS_UTSNAME_H YES)
    set(HAVE_SYS_WAIT_H YES)
    set(HAVE_SYS_XATTR_H YES)
    set(HAVE_TCGETATTR YES)
    set(HAVE_TCSETATTR YES)
    set(HAVE_TIMEGM YES)
    set(HAVE_TIME_H YES)
    set(HAVE_TZSET YES)
    set(HAVE_UNISTD_H YES)
    set(HAVE_UNLINKAT YES)
    set(HAVE_UNSETENV YES)
    set(HAVE_UTIME YES)
    set(HAVE_UTIMENSAT YES)
    set(HAVE_UTIMES YES)
    set(HAVE_UTIME_H YES)
    set(HAVE_VFORK YES)
    set(HAVE_VPRINTF YES)
    set(HAVE_WCHAR_H YES)
    set(HAVE_WCHAR_T YES)
    set(HAVE_WCRTOMB YES)
    set(HAVE_WCSCMP YES)
    set(HAVE_WCSCPY YES)
    set(HAVE_WCSLEN YES)
    set(HAVE_WCTOMB YES)
    set(HAVE_WCTYPE_H YES)
    set(HAVE_WMEMCMP YES)
    set(HAVE_WMEMCPY YES)
    set(HAVE_WMEMMOVE YES)
  elseif(SOURCEMETA_OS_LINUX)
    set(HAVE_INT16_T YES)
    set(HAVE_INT32_T YES)
    set(HAVE_INT64_T YES)
    set(HAVE_INTMAX_T YES)
    set(HAVE_UINT8_T YES)
    set(HAVE_UINT16_T YES)
    set(HAVE_UINT32_T YES)
    set(HAVE_UINT64_T YES)
    set(HAVE_UINTMAX_T YES)
    set(ARCHIVE_XATTR_LINUX YES)
    set(HAVE_ARC4RANDOM_BUF YES)
    set(HAVE_CHOWN YES)
    set(HAVE_CHROOT YES)
    set(HAVE_CLOSEFROM YES)
    set(HAVE_CLOSE_RANGE YES)
    set(HAVE_CTIME_R YES)
    set(HAVE_CTYPE_H YES)
    set(HAVE_DECL_INT32_MAX YES)
    set(HAVE_DECL_INT32_MIN YES)
    set(HAVE_DECL_INT64_MAX YES)
    set(HAVE_DECL_INT64_MIN YES)
    set(HAVE_DECL_INTMAX_MAX YES)
    set(HAVE_DECL_INTMAX_MIN YES)
    set(HAVE_DECL_SIZE_MAX YES)
    set(HAVE_DECL_SSIZE_MAX YES)
    set(HAVE_DECL_STRERROR_R YES)
    set(HAVE_DECL_UINT32_MAX YES)
    set(HAVE_DECL_UINT64_MAX YES)
    set(HAVE_DECL_UINTMAX_MAX YES)
    set(HAVE_DIRENT_H YES)
    set(HAVE_DIRFD YES)
    set(HAVE_DLFCN_H YES)
    set(HAVE_EILSEQ YES)
    set(HAVE_ERRNO_H YES)
    set(HAVE_FCHDIR YES)
    set(HAVE_FCHMOD YES)
    set(HAVE_FCHOWN YES)
    set(HAVE_FCNTL YES)
    set(HAVE_FCNTL_H YES)
    set(HAVE_FDOPENDIR YES)
    set(HAVE_FGETXATTR YES)
    set(HAVE_FLISTXATTR YES)
    set(HAVE_FNMATCH YES)
    set(HAVE_FNMATCH_H YES)
    set(HAVE_FORK YES)
    set(HAVE_FSEEKO YES)
    set(HAVE_FSETXATTR YES)
    set(HAVE_FSTAT YES)
    set(HAVE_FSTATAT YES)
    set(HAVE_FSTATFS YES)
    set(HAVE_FSTATVFS YES)
    set(HAVE_FTRUNCATE YES)
    set(HAVE_FUTIMENS YES)
    set(HAVE_FUTIMES YES)
    set(HAVE_FUTIMESAT YES)
    set(HAVE_GETEUID YES)
    set(HAVE_GETGRGID_R YES)
    set(HAVE_GETGRNAM_R YES)
    set(HAVE_GETLINE YES)
    set(HAVE_GETPID YES)
    set(HAVE_GETPWNAM_R YES)
    set(HAVE_GETPWUID_R YES)
    set(HAVE_GETXATTR YES)
    set(HAVE_GMTIME_R YES)
    set(HAVE_GRP_H YES)
    set(HAVE_INTTYPES_H YES)
    set(HAVE_LANGINFO_H YES)
    set(HAVE_LCHMOD YES)
    set(HAVE_LCHOWN YES)
    set(HAVE_LGETXATTR YES)
    set(HAVE_LIMITS_H YES)
    set(HAVE_LINK YES)
    set(HAVE_LINKAT YES)
    set(HAVE_LINUX_FIEMAP_H YES)
    set(HAVE_LINUX_FS_H YES)
    set(HAVE_LINUX_MAGIC_H YES)
    set(HAVE_LINUX_TYPES_H YES)
    set(HAVE_LISTXATTR YES)
    set(HAVE_LLISTXATTR YES)
    set(HAVE_LOCALE_H YES)
    set(HAVE_LOCALTIME_R YES)
    set(HAVE_LSETXATTR YES)
    set(HAVE_LSTAT YES)
    set(HAVE_LUTIMES YES)
    set(HAVE_MBRTOWC YES)
    set(HAVE_MEMMOVE YES)
    set(HAVE_MEMORY_H YES)
    set(HAVE_MKDIR YES)
    set(HAVE_MKFIFO YES)
    set(HAVE_MKNOD YES)
    set(HAVE_MKSTEMP YES)
    set(HAVE_NL_LANGINFO YES)
    set(HAVE_OPENAT YES)
    set(HAVE_PATHS_H YES)
    set(HAVE_PIPE YES)
    set(HAVE_POLL YES)
    set(HAVE_POLL_H YES)
    set(HAVE_POSIX_SPAWNP YES)
    set(HAVE_PTHREAD_H YES)
    set(HAVE_PWD_H YES)
    set(HAVE_READLINK YES)
    set(HAVE_READLINKAT YES)
    set(HAVE_REGEX_H YES)
    set(HAVE_SELECT YES)
    set(HAVE_SETENV YES)
    set(HAVE_SETLOCALE YES)
    set(HAVE_SIGACTION YES)
    set(HAVE_SIGNAL_H YES)
    set(HAVE_SPAWN_H YES)
    set(HAVE_STATFS YES)
    set(HAVE_STATVFS YES)
    set(HAVE_STDARG_H YES)
    set(HAVE_STDINT_H YES)
    set(HAVE_STDLIB_H YES)
    set(HAVE_STRCHR YES)
    set(HAVE_STRNLEN YES)
    set(HAVE_STRDUP YES)
    set(HAVE_STRERROR YES)
    set(HAVE_STRERROR_R YES)
    set(HAVE_STRFTIME YES)
    set(HAVE_STRINGS_H YES)
    set(HAVE_STRING_H YES)
    set(HAVE_STRRCHR YES)
    set(HAVE_STRUCT_STAT_ST_BLKSIZE YES)
    set(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC YES)
    set(HAVE_STRUCT_TM_TM_GMTOFF YES)
    set(HAVE_SYMLINK YES)
    set(HAVE_SYSCONF YES)
    set(HAVE_SYS_CDEFS_H YES)
    set(HAVE_SYS_IOCTL_H YES)
    set(HAVE_SYS_MOUNT_H YES)
    set(HAVE_SYS_PARAM_H YES)
    set(HAVE_SYS_POLL_H YES)
    set(HAVE_SYS_SELECT_H YES)
    set(HAVE_SYS_STATFS_H YES)
    set(HAVE_SYS_STATVFS_H YES)
    set(HAVE_SYS_STAT_H YES)
    set(HAVE_SYS_SYSMACROS_H YES)
    set(HAVE_SYS_TIME_H YES)
    set(HAVE_SYS_TYPES_H YES)
    set(HAVE_SYS_UTSNAME_H YES)
    set(HAVE_SYS_VFS_H YES)
    set(HAVE_SYS_WAIT_H YES)
    set(HAVE_SYS_XATTR_H YES)
    set(HAVE_TCGETATTR YES)
    set(HAVE_TCSETATTR YES)
    set(HAVE_TIMEGM YES)
    set(HAVE_TIME_H YES)
    set(HAVE_TZSET YES)
    set(HAVE_UNISTD_H YES)
    set(HAVE_UNLINKAT YES)
    set(HAVE_UNSETENV YES)
    set(HAVE_UTIME YES)
    set(HAVE_UTIMENSAT YES)
    set(HAVE_UTIMES YES)
    set(HAVE_UTIME_H YES)
    set(HAVE_VFORK YES)
    set(HAVE_VPRINTF YES)
    set(HAVE_WCHAR_H YES)
    set(HAVE_WCHAR_T YES)
    set(HAVE_WCRTOMB YES)
    set(HAVE_WCSCMP YES)
    set(HAVE_WCSCPY YES)
    set(HAVE_WCSLEN YES)
    set(HAVE_WCTOMB YES)
    set(HAVE_WCTYPE_H YES)
    set(HAVE_WMEMCMP YES)
    set(HAVE_WMEMCPY YES)
    set(HAVE_WMEMMOVE YES)
    set(HAVE_WORKING_FS_IOC_GETFLAGS YES)
    set(MAJOR_IN_SYSMACROS YES)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    set(HAVE_INT16_T YES)
    set(HAVE_INT32_T YES)
    set(HAVE_INT64_T YES)
    set(HAVE_INTMAX_T YES)
    set(HAVE_UINT8_T YES)
    set(HAVE_UINT16_T YES)
    set(HAVE_UINT32_T YES)
    set(HAVE_UINT64_T YES)
    set(HAVE_UINTMAX_T YES)
    set(ARCHIVE_XATTR_LINUX YES)
    set(HAVE_ACL_CREATE_ENTRY YES)
    set(HAVE_ACL_GET_PERM YES)
    set(HAVE_ACL_INIT YES)
    set(HAVE_ACL_PERMSET_T YES)
    set(HAVE_ACL_SET_FD YES)
    set(HAVE_ACL_SET_FILE YES)
    set(HAVE_ATTR_XATTR_H YES)
    set(HAVE_CHOWN YES)
    set(HAVE_CHROOT YES)
    set(HAVE_CLOSE_RANGE YES)
    set(HAVE_CTIME_R YES)
    set(HAVE_CTYPE_H YES)
    set(HAVE_CYGWIN_CONV_PATH YES)
    set(HAVE_DECL_INT32_MAX YES)
    set(HAVE_DECL_INT32_MIN YES)
    set(HAVE_DECL_INT64_MAX YES)
    set(HAVE_DECL_INT64_MIN YES)
    set(HAVE_DECL_INTMAX_MAX YES)
    set(HAVE_DECL_INTMAX_MIN YES)
    set(HAVE_DECL_SIZE_MAX YES)
    set(HAVE_DECL_SSIZE_MAX YES)
    set(HAVE_DECL_STRERROR_R YES)
    set(HAVE_DECL_UINT32_MAX YES)
    set(HAVE_DECL_UINT64_MAX YES)
    set(HAVE_DECL_UINTMAX_MAX YES)
    set(HAVE_DIRENT_H YES)
    set(HAVE_DIRFD YES)
    set(HAVE_DLFCN_H YES)
    set(HAVE_D_MD_ORDER YES)
    set(HAVE_EFTYPE YES)
    set(HAVE_EILSEQ YES)
    set(HAVE_ERRNO_H YES)
    set(HAVE_FCHDIR YES)
    set(HAVE_FCHMOD YES)
    set(HAVE_FCHOWN YES)
    set(HAVE_FCNTL YES)
    set(HAVE_FCNTL_H YES)
    set(HAVE_FDOPENDIR YES)
    set(HAVE_FGETXATTR YES)
    set(HAVE_FLISTXATTR YES)
    set(HAVE_FNMATCH YES)
    set(HAVE_FNMATCH_H YES)
    set(HAVE_FORK YES)
    set(HAVE_FSEEKO YES)
    set(HAVE_FSETXATTR YES)
    set(HAVE_FSTAT YES)
    set(HAVE_FSTATAT YES)
    set(HAVE_FSTATFS YES)
    set(HAVE_FSTATVFS YES)
    set(HAVE_FTRUNCATE YES)
    set(HAVE_FUTIMENS YES)
    set(HAVE_FUTIMES YES)
    set(HAVE_GETEUID YES)
    set(HAVE_GETGRGID_R YES)
    set(HAVE_GETGRNAM_R YES)
    set(HAVE_GETLINE YES)
    set(HAVE_GETPID YES)
    set(HAVE_GETPWNAM_R YES)
    set(HAVE_GETPWUID_R YES)
    set(HAVE_GETXATTR YES)
    set(HAVE_GMTIME_R YES)
    set(HAVE_GRP_H YES)
    set(HAVE_INTTYPES_H YES)
    set(HAVE_IO_H YES)
    set(HAVE_LANGINFO_H YES)
    set(HAVE_LCHOWN YES)
    set(HAVE_LGETXATTR YES)
    set(HAVE_LIMITS_H YES)
    set(HAVE_LINK YES)
    set(HAVE_LINKAT YES)
    set(HAVE_LISTXATTR YES)
    set(HAVE_LLISTXATTR YES)
    set(HAVE_LOCALE_H YES)
    set(HAVE_LOCALTIME_R YES)
    set(HAVE_LSETXATTR YES)
    set(HAVE_LSTAT YES)
    set(HAVE_MBRTOWC YES)
    set(HAVE_MEMMOVE YES)
    set(HAVE_MEMORY_H YES)
    set(HAVE_MKDIR YES)
    set(HAVE_MKFIFO YES)
    set(HAVE_MKNOD YES)
    set(HAVE_MKSTEMP YES)
    set(HAVE_NL_LANGINFO YES)
    set(HAVE_OPENAT YES)
    set(HAVE_PATHS_H YES)
    set(HAVE_PIPE YES)
    set(HAVE_POLL YES)
    set(HAVE_POLL_H YES)
    set(HAVE_POSIX_SPAWNP YES)
    set(HAVE_PROCESS_H YES)
    set(HAVE_PTHREAD_H YES)
    set(HAVE_PWD_H YES)
    set(HAVE_READLINK YES)
    set(HAVE_READLINKAT YES)
    set(HAVE_REGEX_H YES)
    set(HAVE_SELECT YES)
    set(HAVE_SETENV YES)
    set(HAVE_SETLOCALE YES)
    set(HAVE_SIGACTION YES)
    set(HAVE_SIGNAL_H YES)
    set(HAVE_SPAWN_H YES)
    set(HAVE_STATFS YES)
    set(HAVE_STATVFS YES)
    set(HAVE_STDARG_H YES)
    set(HAVE_STDINT_H YES)
    set(HAVE_STDLIB_H YES)
    set(HAVE_STRCHR YES)
    set(HAVE_STRNLEN YES)
    set(HAVE_STRDUP YES)
    set(HAVE_STRERROR YES)
    set(HAVE_STRERROR_R YES)
    set(HAVE_STRFTIME YES)
    set(HAVE_STRINGS_H YES)
    set(HAVE_STRING_H YES)
    set(HAVE_STRRCHR YES)
    set(HAVE_STRUCT_STAT_ST_BIRTHTIME YES)
    set(HAVE_STRUCT_STAT_ST_BLKSIZE YES)
    set(HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC YES)
    set(HAVE_STRUCT_TM_TM_GMTOFF YES)
    set(HAVE_SYMLINK YES)
    set(HAVE_SYSCONF YES)
    set(HAVE_SYS_ACL_H YES)
    set(HAVE_SYS_CDEFS_H YES)
    set(HAVE_SYS_IOCTL_H YES)
    set(HAVE_SYS_MOUNT_H YES)
    set(HAVE_SYS_PARAM_H YES)
    set(HAVE_SYS_POLL_H YES)
    set(HAVE_SYS_SELECT_H YES)
    set(HAVE_SYS_STATFS_H YES)
    set(HAVE_SYS_STATVFS_H YES)
    set(HAVE_SYS_STAT_H YES)
    set(HAVE_SYS_SYSMACROS_H YES)
    set(HAVE_SYS_TIME_H YES)
    set(HAVE_SYS_TYPES_H YES)
    set(HAVE_SYS_UTIME_H YES)
    set(HAVE_SYS_UTSNAME_H YES)
    set(HAVE_SYS_VFS_H YES)
    set(HAVE_SYS_WAIT_H YES)
    set(HAVE_SYS_XATTR_H YES)
    set(HAVE_TCGETATTR YES)
    set(HAVE_TCSETATTR YES)
    set(HAVE_TIMEGM YES)
    set(HAVE_TIME_H YES)
    set(HAVE_TZSET YES)
    set(HAVE_UNISTD_H YES)
    set(HAVE_UNLINKAT YES)
    set(HAVE_UNSETENV YES)
    set(HAVE_UTIME YES)
    set(HAVE_UTIMENSAT YES)
    set(HAVE_UTIMES YES)
    set(HAVE_UTIME_H YES)
    set(HAVE_VFORK YES)
    set(HAVE_VPRINTF YES)
    set(HAVE_WCHAR_H YES)
    set(HAVE_WCHAR_T YES)
    set(HAVE_WCRTOMB YES)
    set(HAVE_WCSCMP YES)
    set(HAVE_WCSCPY YES)
    set(HAVE_WCSLEN YES)
    set(HAVE_WCTOMB YES)
    set(HAVE_WCTYPE_H YES)
    set(HAVE_WINCRYPT_H YES)
    set(HAVE_WINDOWS_H YES)
    set(HAVE_WINIOCTL_H YES)
    set(HAVE_WMEMCMP YES)
    set(HAVE_WMEMCPY YES)
    set(HAVE_WMEMMOVE YES)
    set(MAJOR_IN_SYSMACROS YES)
  elseif(WIN32)
    set(HAVE_INT16_T YES)
    set(HAVE_INT32_T YES)
    set(HAVE_INT64_T YES)
    set(HAVE_INTMAX_T YES)
    set(HAVE_UINT8_T YES)
    set(HAVE_UINT16_T YES)
    set(HAVE_UINT32_T YES)
    set(HAVE_UINT64_T YES)
    set(HAVE_UINTMAX_T YES)
    set(HAVE___INT64 YES)
    set(HAVE_UNSIGNED___INT64 YES)
    set(ARCHIVE_CRYPTO_MD5_WIN YES)
    set(ARCHIVE_CRYPTO_SHA1_WIN YES)
    set(ARCHIVE_CRYPTO_SHA256_WIN YES)
    set(ARCHIVE_CRYPTO_SHA384_WIN YES)
    set(ARCHIVE_CRYPTO_SHA512_WIN YES)
    set(HAVE_CTYPE_H YES)
    set(HAVE_DECL_INT32_MAX YES)
    set(HAVE_DECL_INT32_MIN YES)
    set(HAVE_DECL_INT64_MAX YES)
    set(HAVE_DECL_INT64_MIN YES)
    set(HAVE_DECL_INTMAX_MAX YES)
    set(HAVE_DECL_INTMAX_MIN YES)
    set(HAVE_DECL_SIZE_MAX YES)
    set(HAVE_DECL_UINT32_MAX YES)
    set(HAVE_DECL_UINT64_MAX YES)
    set(HAVE_DECL_UINTMAX_MAX YES)
    set(HAVE_DIRECT_H YES)
    set(HAVE_EILSEQ YES)
    set(HAVE_ERRNO_H YES)
    set(HAVE_FCNTL_H YES)
    set(HAVE_FSTAT YES)
    set(HAVE_GETPID YES)
    set(HAVE_INTTYPES_H YES)
    set(HAVE_IO_H YES)
    set(HAVE_LIMITS_H YES)
    set(HAVE_LOCALE_H YES)
    set(HAVE_MBRTOWC YES)
    set(HAVE_MEMORY_H YES)
    set(HAVE_MKDIR YES)
    set(HAVE_PROCESS_H YES)
    set(HAVE_SETLOCALE YES)
    set(HAVE_SIGNAL_H YES)
    set(HAVE_STDARG_H YES)
    set(HAVE_STDINT_H YES)
    set(HAVE_STDLIB_H YES)
    set(HAVE_STRCHR YES)
    set(HAVE_STRNLEN YES)
    set(HAVE_STRDUP YES)
    set(HAVE_STRERROR YES)
    set(HAVE_STRFTIME YES)
    set(HAVE_STRING_H YES)
    set(HAVE_STRRCHR YES)
    set(HAVE_SYS_STAT_H YES)
    set(HAVE_SYS_TYPES_H YES)
    set(HAVE_SYS_UTIME_H YES)
    set(HAVE_TZSET YES)
    set(HAVE_UTIME YES)
    set(HAVE_WCHAR_H YES)
    set(HAVE_WCHAR_T YES)
    set(HAVE_WCRTOMB YES)
    set(HAVE_WCSCMP YES)
    set(HAVE_WCSCPY YES)
    set(HAVE_WCSLEN YES)
    set(HAVE_WCTOMB YES)
    set(HAVE_WCTYPE_H YES)
    set(HAVE_WINCRYPT_H YES)
    set(HAVE_WINDOWS_H YES)
    set(HAVE_WINIOCTL_H YES)
    set(HAVE__CrtSetReportMode YES)
    set(HAVE_CTIME_S YES)
    set(HAVE__FSEEKI64 YES)
    set(HAVE__GET_TIMEZONE YES)
    set(HAVE_GMTIME_S YES)
    set(HAVE_LOCALTIME_S YES)
    set(HAVE__MKGMTIME YES)
    set(gid_t short)
    set(id_t short)
    set(mode_t "unsigned short")
    set(pid_t int)
    set(ssize_t int64_t)
    set(uid_t short)
  endif()

  set(HAVE_ZLIB_H YES)
  configure_file("${PROJECT_SOURCE_DIR}/vendor/libarchive/build/cmake/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libarchive/config.h" @ONLY)
  target_compile_definitions(archive
    PUBLIC PLATFORM_CONFIG_H="${CMAKE_CURRENT_BINARY_DIR}/libarchive/config.h")
  target_link_libraries(archive PUBLIC zlib)

  if(BUILD_SHARED_LIBS)
    target_compile_definitions(archive PUBLIC __LIBARCHIVE_ENABLE_VISIBILITY)
  else()
    target_compile_definitions(archive PUBLIC LIBARCHIVE_STATIC)
  endif()

  if(SOURCEMETA_OS_LINUX)
    target_compile_definitions(archive PUBLIC _GNU_SOURCE)
  endif()

  if(SOURCEMETA_COMPILER_GCC)
    target_compile_options(archive PRIVATE -Wno-deprecated-declarations)
  endif()

  if(MSVC)
    target_compile_definitions(archive PUBLIC ZLIB_WINAPI)
  endif()

  target_include_directories(archive PUBLIC
    "$<BUILD_INTERFACE:${ARCHIVE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  add_library(Archive::Archive ALIAS archive)

  set_target_properties(archive
    PROPERTIES
      OUTPUT_NAME archive
      PUBLIC_HEADER "${ARCHIVE_PUBLIC_HEADER}"
      PRIVATE_HEADER "${ARCHIVE_PRIVATE_HEADERS}"
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      EXPORT_NAME archive)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS archive
      EXPORT archive
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_core_dev
      PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_core_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT archive
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/archive"
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/archive-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/archive.cmake\")\n"
      "check_required_components(\"archive\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/archive-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/archive"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(Archive_FOUND ON)
endif()
