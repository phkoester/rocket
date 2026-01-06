/**
 * @file std.h
 *
 * Additional standard library support.
 */

#include "rocket/hash.h"

#include <functional>
#include <tuple>

namespace std {

// `hash<tuple>` --------------------------------------------------------------------------------------------

/// @spec_std_hash{`tuple`}
template<typename... T>
struct hash<tuple<T...>> {
  /// @cond undocumented

  size_t
  operator()(const tuple<T...>& v) const {
    using TupleType = rocket::PurgeType<decltype(v)>;
    size_t ret = tuple_size<TupleType>::value;
    apply([&](auto&&... arg) { (rocket::hashCombine(ret, arg), ...); }, v);
    return ret;
  }

  /// @endcond
};

} // namespace std

// EOF
