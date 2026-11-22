// windows.h
//
// Windows-specific definitions.

#pragma once

#ifndef ROCKET_OS_WINDOWS
  #error this header is for Windows only
#endif

#define NOMINMAX

#ifndef ROCKET_WINDOWS_FULL
  #define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include <string>
#include <string_view>
#include <system_error>

namespace rocket::windows {

// Functions ------------------------------------------------------------------------------------------------

/// Returns the error code for a failed `HRESULT`.
///
/// @param hr a failed `HRESULT`
/// @return the error code, in #std::system_category
std::error_code errorCode(HRESULT hr);

/// Converts the UTF-16 string @p str to UTF-8.
///
/// @param str a UTF-16 string
/// @return the UTF-8 string
/// @throw SystemError if the conversion fails
std::string toUtf8(std::wstring_view str);

/// Converts the UTF-8 string @p str to UTF-16.
///
/// @param str a UTF-8 string
/// @return the UTF-16 string
/// @throw SystemError if @p str is not valid UTF-8 or the conversion fails
std::wstring toUtf16(std::string_view str);

} // namespace rocket::windows

// EOF
