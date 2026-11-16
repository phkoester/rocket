/**
 * @file enum-scan.h
 *
 * Scanning support for Rocket enums.
 */

#pragma once

#include "rocket/enum-fwd.h"

#include <scn/scan.h>

#include <cstddef>
#include <iterator>
#include <string>

// `scn::scanner<rocket::Enum>` -----------------------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::Enum}
 *
 * This scanner uses the same format specifiers as the underlying string scanner. It scans a word and then
 * gives back the characters that follow the longest enum name the word starts with, so that, for example,
 * `Red,` scans as `Red` and leaves the comma in the input.
 */
template<typename E> requires rocket::Enum<E>::value
struct scn::scanner<E, char> : scn::scanner<std::string, char> {
  /// @cond undocumented

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(E& val, Context& ctx) const {
    // This must scan a #std::string, not a #std::string_view, because the latter does not work when scanning
    // from a #std::istream
    std::string str;
    auto scanResult = scn::scanner<std::string, char>::scan(str, ctx);
    if (not scanResult) {
      return scn::unexpected(scanResult.error());
    }
    const auto enumResult = rocket::Enum<E>::toType(str, false);
    if (not enumResult) {
      return scn::unexpected(scn::scan_error(scn::scan_error::invalid_scanned_value, "invalid enum value"));
    }
    const auto [size, enumVal] = *enumResult;
    val = enumVal;

    // Give back what follows the scanned portion of the word. The iterators of scnlib are bidirectional
    auto it = *scanResult;
    std::advance(it, static_cast<std::ptrdiff_t>(size) - static_cast<std::ptrdiff_t>(str.size()));
    return it;
  }

  /// @endcond
};

// EOF
