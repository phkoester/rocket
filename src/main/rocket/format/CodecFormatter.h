/**
 * CodecFormatter.h
 *
 * A building block for {fmt} formatters of types that are formatted with #rocket::codec::FormattedCodec.
 */

#pragma once

#include "rocket/TypeId.h"
#include "rocket/type-traits.h"
#include "rocket/codec/FormattedCodec.h"
#include "rocket/format/StringFormatter.h"
#include "rocket/nio/nio.h"

namespace rocket::format::internal {

/**
 * A base for formatters of types that are formatted with #rocket::codec::FormattedCodec.
 *
 * - If the `i` format specifier is used, then the output is indented.
 * - If @p WithType is `true` and the `t` format specifier is used, then the type name is included.
 *
 * The format specifiers may appear in any order.
 *
 * @tparam C the character type
 * @tparam WithType whether the `t` format specifier is accepted
 */
template<typename C, bool WithType = false> requires IsChar<C>
struct CodecFormatter {
  constexpr const C*
  parse(fmt::parse_context<C>& ctx) {
    return parseFlags(ctx, [this](C c) {
      if (c == 'i') {
        indent_ = true;
        return true;
      }
      if (WithType && c == 't') {
        withType_ = true;
        return true;
      }
      return false;
    });
  }

protected:

  /**
   * Formats a value.
   *
   * @param val the value
   * @param type the type whose name is written if the `t` format specifier is used
   * @param ctx the format context
   * @return the output iterator past the written characters
   */
  template<typename T, typename FormatContext>
  FormatContext::iterator
  formatEncoded(const T& val, const std::type_info& type, FormatContext& ctx) const {
    auto out = ctx.out();
    if (withType_) {
      out = writeUtf8<C>(out, TypeId(type).prettyName());
    }
    nio::StringSink buf;
    codec::FormattedCodec().encode(val, buf, { .indent = indent_ });
    return writeUtf8<C>(out, buf.str());
  }

  /**
   * Formats a value without a type name.
   *
   * @param val the value
   * @param ctx the format context
   * @return the output iterator past the written characters
   */
  template<typename T, typename FormatContext>
  FormatContext::iterator
  formatEncoded(const T& val, FormatContext& ctx) const {
    return formatEncoded(val, typeid(T), ctx);
  }

private:

  bool indent_ = false;
  bool withType_ = false;
};

} // namespace rocket::format::internal

// EOF
