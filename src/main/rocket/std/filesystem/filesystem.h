/**
 * @file filesystem.h
 */

#pragma once

#include <filesystem>

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

/**
 * Returns a unique path to atemporary directory.
 *
 * The directory is created and removed on exit, but not on termination.
 *
 * @return a path to a newly created temporary directory
 */
std::filesystem::path tempDir();

/**
 * Returns a unique path to a temporary file.
 *
 * The file is not created, but removed on exit as well as on termination.
 *
 * @return a path to a new temporary file
 */
std::filesystem::path tempFile();

} // namespace rocket::filesystem

// EOF
