// windows.cc

#include "windows.h"

#include "rocket/SystemError.h"

using namespace std;

namespace rocket::windows {

// Functions ------------------------------------------------------------------------------------------------

error_code
errorCode(HRESULT hr) {
  return { static_cast<int>(HRESULT_CODE(hr)), system_category() };
}

string
toUtf8(wstring_view str) {
  if (str.empty()) {
    return {};
  }
  const int strSize = static_cast<int>(str.size());
  const int size = WideCharToMultiByte(CP_UTF8, 0, str.data(), strSize, nullptr, 0, nullptr, nullptr);
  if (size == 0) {
    throw SystemError(error_code(static_cast<int>(GetLastError()), system_category()),
      "cannot convert string to UTF-8");
  }
  string ret(static_cast<size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, str.data(), strSize, ret.data(), size, nullptr, nullptr);
  return ret;
}

wstring
toUtf16(string_view str) {
  if (str.empty()) {
    return {};
  }
  const int strSize = static_cast<int>(str.size());
  const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), strSize, nullptr, 0);
  if (size == 0) {
    throw SystemError(error_code(static_cast<int>(GetLastError()), system_category()),
      "cannot convert string to UTF-16");
  }
  wstring ret(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), strSize, ret.data(), size);
  return ret;
}

} // namespace rocket::windows

// EOF
