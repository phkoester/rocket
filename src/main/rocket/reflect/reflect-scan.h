/**
 * @file reflect-scan.h
 *
 * Scanning support for C++ reflection: declared objects and instances.
 *
 * The scanners read what the formatters in `rocket/reflect/reflect-format.h` write. Variable references
 * (#rocket::reflect::VarRef) cannot be scanned, just as #rocket::codec::FormattedCodec cannot decode them.
 */

#pragma once

#include "rocket/TypeId.h"
#include "rocket/type-traits.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/nio/nio.h"
#include "rocket/reflect/Declared.h"
#include "rocket/reflect/Instance.h"

#include <scn/scan.h>

#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>

namespace rocket::internal::scan::reflect {

// `IteratorSource` -----------------------------------------------------------------------------------------

/**
 * A #rocket::nio::Source that reads the input of a scnlib scanner, given as an iterator and a sentinel.
 *
 * Positions are counted from the iterator the source was constructed with. Seeking is supported within the
 * input, but not relative to its end.
 *
 * @tparam It the iterator type
 * @tparam Sent the sentinel type
 */
template<typename It, typename Sent>
struct IteratorSource : nio::Source {
  using Source::read;
  using Source::seek;

  /**
   * @ctor
   *
   * @param begin the start of the input
   * @param end the end of the input
   */
  IteratorSource(It begin, Sent end) : begin_(begin), it_(begin), end_(end) { seekable_ = true; }

  [[nodiscard]] bool bad() const override { return false; }

  [[nodiscard]] bool eof() const override { return it_ == end_; }

  [[nodiscard]] i32 handle() const override { return -1; }

  /// Returns the iterator at the current position.
  [[nodiscard]] It iterator() const { return it_; }

  std::optional<char>
  peek() override {
    if (it_ == end_) {
      return std::nullopt;
    }
    return static_cast<char>(*it_);
  }

  u64
  read(std::span<u8> out) override {
    u64 n = 0;
    for (; n < out.size() && it_ != end_; ++n) {
      out[n] = static_cast<u8>(*it_);
      advance();
    }
    return n;
  }

  std::optional<std::string>
  readln() override {
    if (it_ == end_) {
      return std::nullopt;
    }
    std::string ret;
    while (it_ != end_) {
      const char c = *it_;
      advance();
      if (c == '\n') {
        break;
      }
      ret += c;
    }
    if (not ret.empty() && ret.back() == '\r') {
      ret.pop_back();
    }
    return ret;
  }

  bool
  seek(i64 offset, nio::SeekMode mode) override {
    i64 target = 0;
    switch (mode) {
    case nio::SeekMode::beg:
      target = offset;
      break;
    case nio::SeekMode::cur:
      target = static_cast<i64>(pos_) + offset;
      break;
    case nio::SeekMode::end:
      return false;
    }
    if (target < 0) {
      return false;
    }

    const auto targetPos = static_cast<u64>(target);
    if (targetPos < pos_) {
      if constexpr (std::bidirectional_iterator<It>) {
        while (pos_ > targetPos) {
          --it_;
          --pos_;
        }
        return true;
      } else {
        it_ = begin_;
        pos_ = 0;
      }
    }

    // Move forward; on failure, the position is left unchanged
    const It it = it_;
    const u64 pos = pos_;
    while (pos_ < targetPos) {
      if (it_ == end_) {
        it_ = it;
        pos_ = pos;
        return false;
      }
      advance();
    }
    return true;
  }

  [[nodiscard]] std::optional<u64> tell() override { return pos_; }

protected:

  scn::scan_expected<void>
  vscan(std::string_view fmt, scn::scan_args args) override {
    return consume(scn::vscan(scn::ranges::subrange<It, Sent>(it_, end_), fmt, args));
  }

  scn::scan_expected<void>
  vscan(const std::locale& locale, std::string_view fmt, scn::scan_args args) override {
    return consume(scn::vscan(locale, scn::ranges::subrange<It, Sent>(it_, end_), fmt, args));
  }

private:

  It begin_;
  It it_;
  Sent end_;
  u64 pos_ = 0;

  void
  advance() {
    ++it_;
    ++pos_;
  }

  /// Moves past the input that a successful scan consumed.
  template<typename Result>
  scn::scan_expected<void>
  consume(const Result& result) {
    if (not result) {
      return scn::unexpected(result.error());
    }
    const It end = result->begin();
    while (it_ != end) {
      advance();
    }
    return {};
  }
};

// `Scanner` ------------------------------------------------------------------------------------------------

/**
 * A scnlib scanner for types that are decoded with #rocket::codec::FormattedCodec.
 *
 * - If the `t` format specifier is used, then the name of @p Named must precede the value, as written by the
 *   formatter with the `t` format specifier. Whitespace before the name is skipped.
 *
 * Whitespace before the value is skipped. The `i` format specifier is not needed: indented and unindented
 * input are read alike.
 *
 * @tparam T the type to scan
 * @tparam Named the type whose name is read if the `t` format specifier is used
 */
template<typename T, typename Named>
struct Scanner {
  /// @cond undocumented

  template<typename ParseContext>
  constexpr ParseContext::iterator
  parse(ParseContext& pctx) {
    auto it = pctx.begin();
    const auto end = pctx.end();
    for (; it != end && *it != '}'; ++it) {
      if (*it != 't') {
        pctx.on_error("invalid format specifier");
        return it;
      }
      withType_ = true;
    }
    return it;
  }

  template<typename Context>
  scn::scan_expected<typename Context::iterator>
  scan(T& val, Context& ctx) const {
    IteratorSource<typename Context::iterator, typename Context::sentinel> in(ctx.begin(), ctx.end());

    if (withType_) {
      if (auto result = readTypeName(in); not result) {
        return scn::unexpected(result.error());
      }
    }

    auto result = codec::FormattedCodec().decode<T>(in);
    if (not result) {
      return scn::unexpected(scn::scan_error(
        in.eof() ? scn::scan_error::end_of_input : scn::scan_error::invalid_scanned_value,
        "invalid reflected value"));
    }
    val = std::move(*result);
    return in.iterator();
  }

  /// @endcond

private:

  bool withType_ = false;

  /// Skips whitespace and reads the name of `Named`.
  static scn::scan_expected<void>
  readTypeName(nio::Source& in) {
    while (const auto c = in.peek()) {
      if (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r' && *c != '\v' && *c != '\f') {
        break;
      }
      in.seek(1, nio::SeekMode::cur);
    }

    const std::string name = TypeId(typeid(Named)).prettyName();
    for (const char expected : name) {
      const auto c = in.peek();
      if (not c) {
        return scn::unexpected(
          scn::scan_error(scn::scan_error::end_of_input, "expected a type name"));
      }
      if (*c != expected) {
        return scn::unexpected(
          scn::scan_error(scn::scan_error::invalid_scanned_value, "unexpected type name"));
      }
      in.seek(1, nio::SeekMode::cur);
    }
    return {};
  }
};

} // namespace rocket::internal::scan::reflect

// `scn::scanner<rocket::reflect::Declared>` ----------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::reflect::Declared}
 *
 * The value is decoded with #rocket::codec::FormattedCodec, e.g., `(x=1, y="a")`. Members may appear in any
 * order, and missing members keep their default values.
 *
 * - If the `t` format specifier is used, then the type name must precede the value, e.g., `MyStruct(x=1)`.
 *   The name must be that of the scanned type.
 * - The `i` format specifier is not needed: whitespace, including line breaks, is skipped.
 */
template<typename T> requires rocket::reflect::Declared<T>::value
struct scn::scanner<T, char> : rocket::internal::scan::reflect::Scanner<T, T> {};

// `scn::scanner<rocket::reflect::Instance>` ----------------------------------------------------------------

/**
 * @spec_scn_scanner{#rocket::reflect::Instance}
 *
 * The value is decoded with #rocket::codec::FormattedCodec, e.g., `(x=1, y="a")`. Members may appear in any
 * order, and missing members keep their default values.
 *
 * - If the `t` format specifier is used, then the name of the instance type must precede the value, e.g.,
 *   `MyStruct(x=1)`.
 * - The `i` format specifier is not needed: whitespace, including line breaks, is skipped.
 */
template<typename T, typename Inner>
struct scn::scanner<rocket::reflect::Instance<T, Inner>, char> :
  rocket::internal::scan::reflect::Scanner<rocket::reflect::Instance<T, Inner>, T> {};

// EOF
