/**
 * @file codec-boost.h
 *
 * Boost codec.
 */

#pragma once

#include "codec-boost-decl.h"

#include "codec-global.h"

#include "codec.h"

#ifndef ROCKET_CODEC_H
#error `codec.h` must be included before this file
#endif

// Template definitions -------------------------------------------------------------------------------------

namespace boost {

using ::parseRon;
using ::printRon;

namespace bimaps {

/// @fn_printRon{`boost::bimaps::bimap`}
template<typename K, typename V>
inline std::ostream&
printRon(std::ostream& os, const bimap<K, V>& v) {
  constexpr bool indentChildren = rocket::IsContainer<K>::value || rocket::IsContainer<V>::value;
  return rocket::codec::ron::printing::printMap(os, indentChildren, v.left);
}

} // namespace bimaps

namespace spirit::x3 {

/// @fn_printRon{`boost::spirit::x3::forward_ast`}
template<typename T>
inline std::ostream&
printRon(std::ostream& os, const forward_ast<T>& v) {
  return printRon(os, v.get());
}

/// @fn_printRon{`boost::spirit::x3::variant`}
template<typename... T>
inline std::ostream&
printRon(std::ostream& os, const variant<T...>& v) {
  return boost::apply_visitor([&](auto&& arg) -> auto& {
    os << v.get().which() << ':';
    return printRon(os, std::forward<decltype(arg)>(arg));
  }, v);
}

} // namespace spirit::x3

}  // namespace boost

// EOF
