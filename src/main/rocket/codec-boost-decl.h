/**
 * codec-boost-decl.h
 *
 * Boost codec: declarations.
 */

#pragma once

#include "concept.h"

#include <boost/bimap.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

#ifdef ROCKET_CODEC_H
#error 'codec.h' must be included after this file
#endif

// Template declarations ------------------------------------------------------------------------------------

namespace boost {

namespace bimaps {

template<typename K, typename V> std::ostream& printRon(std::ostream&, const bimap<K, V>&);

} // namespace bimaps

namespace spirit::x3 {

template<typename T> std::ostream& printRon(std::ostream&, const forward_ast<T>&);

template<typename... T> std::ostream& printRon(std::ostream&, const variant<T...>&);

} // namespace spirit::x3

} // namespace boost

// 'IsContainerImpl' specializations ------------------------------------------------------------------------

namespace rocket {

/// @spec_rocket_IsContainerImpl{@c boost::bimaps::bimap}
template<typename K, typename V> struct IsContainerImpl<::boost::bimaps::bimap<K, V>> : std::true_type {};

} // namespace rocket

// EOF
