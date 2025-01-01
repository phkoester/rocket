/**
 * @file print.h
 *
 * A collection of GoogleTest printers.
 */

#pragma once

#include "rocket/boost.h"

// 'boost' --------------------------------------------------------------------------------------------------

namespace boost {

namespace bimaps {

/// @fn_PrintTo{boost::bimaps::bimap}
template<typename K, typename V>
inline void
PrintTo(const bimap<K, V>& v, std::ostream* os) {
  printRon(*os, v);
}

} // namespace bimaps

} // namespace boost

// EOF
