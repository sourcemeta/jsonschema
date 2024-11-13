#ifndef SOURCEMETA_BLAZE_EVALUATOR_TRACE_H_
#define SOURCEMETA_BLAZE_EVALUATOR_TRACE_H_

// We only perform tracing on debugging builds, at least for now
#if !defined(NDEBUG) && defined(__APPLE__) && defined(__clang__)

#include <os/log.h>
#include <os/signpost.h>

// See
// https://www.jviotti.com/2022/02/21/emitting-signposts-to-instruments-on-macos-using-cpp.html

static os_log_t log_handle =
    os_log_create("com.sourcemeta.blaze", OS_LOG_CATEGORY_POINTS_OF_INTEREST);

#define SOURCEMETA_TRACE_REGISTER_ID(name)                                     \
  const os_signpost_id_t name = os_signpost_id_generate(log_handle);           \
  assert((name) != OS_SIGNPOST_ID_INVALID);
#define SOURCEMETA_TRACE_START(id, title)                                      \
  os_signpost_interval_begin(log_handle, id, title);
#define SOURCEMETA_TRACE_END(id, title)                                        \
  os_signpost_interval_end(log_handle, id, title);

#else
#define SOURCEMETA_TRACE_REGISTER_ID(name)
#define SOURCEMETA_TRACE_START(id, title)
#define SOURCEMETA_TRACE_END(id, title)
#endif

#endif
