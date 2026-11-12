/**
 * @file archive.h
 *
 * Archive and compression functions, based on libarchive.
 */

#pragma once

#include "rocket/rocket.h"

#include <filesystem>

namespace rocket::archive {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Compresses a file or a directory with gzip.
 *
 * - If @p srce is a regular file, @p dest is a plain gzip file containing the file's contents.
 * - If @p srce is a directory, @p dest is a gzip-compressed tar archive (POSIX pax format) of the directory.
 *   The entries are stored relative to the parent of @p srce, so the archive's single top-level entry is the
 *   directory itself. Symbolic links are stored as links and not followed.
 *
 * An existing @p dest is overwritten. If the function fails, a partially written @p dest is removed.
 *
 * @param srce the file or directory to compress
 * @param dest the path to the resulting `.gz` file
 * @throw InvalidArgument if @p srce is neither a regular file nor a directory, or if @p dest is a directory,
 *   equal to @p srce, or located inside @p srce
 * @throw InvalidState if reading @p srce or writing @p dest fails
 */
void gzip(const std::filesystem::path& srce, const std::filesystem::path& dest);

} // namespace rocket::archive

// EOF
