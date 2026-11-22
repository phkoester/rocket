/// @file filesystem.h

#pragma once

#include <filesystem>

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

/// Returns a unique path to a temporary directory.
///
/// The directory is created and removed on exit, but not on termination.
///
/// @return a path to a newly created temporary directory
std::filesystem::path newTempDir();

/// Returns a unique path to a temporary file.
///
/// The file is not created, but removed on exit as well as on termination.
///
/// @return a path to a new temporary file
std::filesystem::path newTempFile();

/// Returns the system-specific temporary directory.
///
/// @return the system-specific temporary directory
std::filesystem::path systemTempDir();

/// Returns the user-specific temporary directory.
///
/// @return the user-specific temporary directory
std::filesystem::path userTempDir();

} // namespace rocket::filesystem

// EOF
