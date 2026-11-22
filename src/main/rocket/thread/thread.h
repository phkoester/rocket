/// @file thread.h
///
/// Thread utilities.

#pragma once

#include <string>
#include <string_view>

namespace rocket::thread {

// Functions ------------------------------------------------------------------------------------------------

/// Returns the name of the current thread, as stored by the operating system.
///
/// A thread that was never named has a platform-specific default name: on Linux, it inherits the name of the
/// thread that created it, which for the main thread is the name of the executable; on macOS and Windows,
/// it is empty.
///
/// The Rocket logging API considers the thread name for logging.
///
/// @ThreadSafe
///
/// @return the name of the current thread, encoded in UTF-8
/// @throw SystemError if the name cannot be obtained
[[nodiscard]] std::string currentThreadName();

/// Sets the name of the current thread, as stored by the operating system.
///
/// The name is visible to debuggers and system tools, e.g., `gdb`, `top -H`, or the Visual Studio debugger.
///
/// Platforms limit the length of the name: on Linux, the name is cut to 15 bytes; on macOS, to 63 bytes.
/// The name is cut at a UTF-8 code-point boundary. On Linux, naming the main thread also renames the process
/// as shown by `ps` or `top`.
///
/// @ThreadSafe
///
/// @param name the name, encoded in UTF-8. Must not contain a NUL character
/// @throw InvalidArgument if @p name contains a NUL character
/// @throw SystemError if the name cannot be set
void setCurrentThreadName(std::string_view name);

} // namespace rocket::thread

// EOF
