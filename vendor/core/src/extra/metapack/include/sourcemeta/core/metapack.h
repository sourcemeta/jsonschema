#ifndef SOURCEMETA_CORE_METAPACK_H_
#define SOURCEMETA_CORE_METAPACK_H_

#ifndef SOURCEMETA_CORE_METAPACK_EXPORT
#include <sourcemeta/core/metapack_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/metapack_error.h>
#include <sourcemeta/core/metapack_format.h>
#include <sourcemeta/core/metapack_read.h>
#include <sourcemeta/core/metapack_write.h>
// NOLINTEND(misc-include-cleaner)

/// @defgroup metapack Metapack
/// @brief A file format that puts a memory-mappable metadata header in front
/// of arbitrary contents
///
/// A filesystem answers a few questions about a file without anyone opening
/// it, which is what a stat call is for: how large it is, when it last
/// changed, what kind of file it is. Everything else a file has to say about
/// itself has nowhere to live. Keeping it beside the file, in a sidecar, a
/// database, or a naming convention, costs a second lookup and lets the two
/// drift apart. Keeping it inside the contents means decoding the contents to
/// reach it.
///
/// A Metapack file carries it in front of the contents instead, as a packed
/// little-endian structure whose fields sit at offsets fixed at compile time.
/// A reader memory maps the file, points the header structure at the start of
/// the mapping, and has every field without a read, a copy, or an allocation.
/// What follows is handed back as a view into that same mapping, so the
/// contents cost nothing until something reaches for them.
///
/// What the header holds, field by field, is the
/// @ref sourcemeta::core::MetapackHeader
/// structure itself. After it come the media type, then an extension area,
/// then the contents, each located from the length recorded before it.
///
/// The extension area carries whatever else a given kind of file needs to say
/// about itself, as a packed structure that the writer defines and that a
/// reader interprets in place, exactly like the header itself.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/metapack.h>
/// ```

#endif
