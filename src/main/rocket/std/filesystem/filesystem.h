/**
 * @file filesystem.h
 */

#pragma once

#include <filesystem>

namespace rocket::filesystem {

// Functions ------------------------------------------------------------------------------------------------

std::filesystem::path tempDir();

std::filesystem::path tempFile();

} // namespace rocket::filesystem

// EOF
