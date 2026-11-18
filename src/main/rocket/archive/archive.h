/// @file archive.h
///
/// Archive functions, based on libarchive.

#pragma once

#include "rocket/rocket.h"

#include <filesystem>

namespace rocket::archive {

// Functions ------------------------------------------------------------------------------------------------

/// Compresses a file or a directory with gzip.
///
/// - If @p srce is a regular file, @p dest is a plain gzip file containing the file’s contents.
/// - If @p srce is a directory, @p dest is a gzip-compressed tar archive (POSIX pax format) of the
///   directory. The entries are stored relative to the parent of @p srce, so the archive’s single top-level
///   entry is the directory itself. Symbolic links are stored as links and not followed.
///
/// An existing @p dest is overwritten. If the function fails, a partially written @p dest is removed.
///
/// @param srce the file or directory to compress
/// @param dest the path to the resulting `.gz` file
/// @throw #rocket::InvalidArgument if @p srce is empty or neither a regular file nor a directory, or if
///   @p dest is empty, a directory, equal to @p srce, or located inside @p srce
/// @throw #rocket::SystemError if @p srce does not exist or cannot be accessed, if reading from @p srce or
///   writing to @p dest fails, or if libarchive does not support the archive format or gzip compression. The
///   error code is the `errno` value of the failing operation, if any, and `std::errc::io_error` or
///   `std::errc::not_supported` otherwise
/// @throw std::bad_alloc if libarchive cannot allocate memory
void gzip(const std::filesystem::path& srce, const std::filesystem::path& dest);

} // namespace rocket::archive

// EOF
