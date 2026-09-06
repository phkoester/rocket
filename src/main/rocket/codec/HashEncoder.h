/**
 * @file HashEncoder.h
 *
 * Hashing arbitrary C++ data structures.
 */

#pragma once

#include "rocket/functional.h"
#include "rocket/hash/hash.h"
#include "rocket/codec/codec.h"

#include <boost/container_hash/hash.hpp>

namespace rocket::codec {

namespace internal {

// Functions ------------------------------------------------------------------------------------------------

template<bool Unordered>
void
combine(u64& seed, u64 hash) {
  if constexpr (Unordered) {
    // For unordered containers, use a permutative hashing scheme so that the element order does not affect
    // the hash value
    seed += hash;
  } else {
    // Otherwise, do it like the Boost guys do it
    hash::combine(seed, hash);
  }
}

// `HashConsumerImpl` ---------------------------------------------------------------------------------------

template<DataType DataType, typename T, typename Hash>
struct HashConsumerImpl;

template<typename Hash>
struct HashConsumerImpl<DataType::Bool, bool, Hash> {
  u64 consume(bool val) { return Hash()(val); }
};

template<typename C, typename Hash>
struct HashConsumerImpl<DataType::Char, C, Hash> {
  u64 consume(C val) { return Hash()(val); }
};

template<typename E, typename Hash>
struct HashConsumerImpl<DataType::Enum, E, Hash> {
  u64 consume(E val) { return Hash()(val); }
};

template<typename I, typename Hash>
struct HashConsumerImpl<DataType::Integer, I, Hash> {
  u64 consume(I val) { return Hash()(val); }
};

template<typename F, typename Hash>
struct HashConsumerImpl<DataType::Float, F, Hash> {
  u64 consume(F val) { return Hash()(val); }
};

template<typename P, typename Hash>
struct HashConsumerImpl<DataType::Pointer, P, Hash> {
  u64 consume(P val) { return Hash()(val); }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::String, T, Hash> {
  u64 consume(const T& val) { return Hash()(val); }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Optional, T, Hash> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  u64
  consume(const T& val) {
    if (not val) {
      return Hash()(0);
    }

    u64 ret = 1;
    const u64 elemHash = HashConsumerImpl<ElemDataType, Elem, Hash>().consume(*val);
    combine<false>(ret, elemHash);
    return ret;
  }
};

// For `MemberRef`, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Tuple, T, Hash> {
  template<typename... Args>
  u64
  consume(const T& val, Args&&... args) {
    u64 ret = std::tuple_size_v<T>;
    std::apply([&](auto&&... arg) {
      (consumeElem(ret, std::forward<decltype(arg)>(arg), std::forward<Args>(args)...), ...);
    }, val);
    return ret;
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(u64& seed, const Elem& elem, Args&&... args) {
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    auto elemHash = HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem, std::forward<Args>(args)...);
    combine<false>(seed, elemHash);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::List, T, Hash> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  u64
  consume(const T& val) {
    u64 ret = val.size();
    for (const auto& elem : val) {
      auto elemHash = HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem);
      combine<false>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Set, T, Hash> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;

  u64
  consume(const T& val) {
    u64 ret = val.size();
    for (const auto& elem : val) {
      auto elemHash = HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem);
      combine<Unordered>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Map, T, Hash> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;

  u64
  consume(const T& val) {
    u64 ret = val.size();
    for (const auto& [key, elem] : val) {
      const auto keyHash = HashConsumerImpl<KeyDataType, Key, Hash>().consume(key);
      combine<Unordered>(ret, keyHash);
      const auto elemHash = HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem);
      combine<Unordered>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Bimap, T, Hash> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;

  u64
  consume(const T& val) {
    u64 ret = val.size();
    for (const auto& [key, elem] : val.left) {
      const auto keyHash = HashConsumerImpl<KeyDataType, Key, Hash>().consume(key);
      combine<IsUnordered<T>>(ret, keyHash);
      const auto elemHash = HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem);
      combine<Unordered>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Interval, T, Hash> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  u64
  consume(const T& val) {
    u64 ret = HashConsumerImpl<ADataType, A, Hash>().consume(val.a);
    combine<false>(ret, HashConsumerImpl<BDataType, B, Hash>().consume(val.b));
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Duration, T, Hash> {
  using Period = T::period;

  u64
  consume(T val) { // Take by value
    u64 ret = Hash()(val.count());
    combine<false>(ret, Hash()(Period::num));
    combine<false>(ret, Hash()(Period::den));
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::YearMonthDay, T, Hash> {
  static constexpr auto I32DataType = DataTypes<i32>::Value;

  u64
  consume(const T& val) {
    const i32 year = static_cast<std_int>(val.year());
    const i32 month = static_cast<std_unsigned>(val.month());
    const i32 day = static_cast<std_unsigned>(val.day());

    u64 ret = Hash()(year);
    combine<false>(ret, Hash()(month));
    combine<false>(ret, Hash()(day));
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::HourMinuteSecond, T, Hash> {
  using Precision = T::precision;
  static constexpr auto PrecisionDataType = DataTypes<Precision>::Value;
  static_assert(PrecisionDataType == DataType::Duration);

  u64
  consume(const T& val) {
    const Precision duration = val.to_duration();
    return HashConsumerImpl<PrecisionDataType, Precision, Hash>().consume(duration);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::TimeZone, T, Hash> {
  u64
  consume(T val) {
    const std::string_view name = val->name();
    return Hash()(name);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::TimePoint, T, Hash> {
  using Duration = T::duration;
  static constexpr auto DurationDataType = DataTypes<Duration>::Value;
  static_assert(DurationDataType == DataType::Duration);

  u64
  consume(T val) { // Take by value
    const Duration duration = val.time_since_epoch();
    return HashConsumerImpl<DurationDataType, Duration, Hash>().consume(duration);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::ZonedTime, T, Hash> {
  using TimeZone = const std::chrono::time_zone*;
  static constexpr auto TimeZoneDataType = DataTypes<TimeZone>::Value;
  static_assert(TimeZoneDataType == DataType::TimeZone);
  using Duration = T::duration;
  using SysTime = std::chrono::sys_time<Duration>;
  static constexpr auto SysTimeDataType = DataTypes<SysTime>::Value;
  static_assert(SysTimeDataType == DataType::TimePoint);

  u64
  consume(const T& val) {
    const auto* tz = val.get_time_zone();
    u64 ret = HashConsumerImpl<TimeZoneDataType, TimeZone, Hash>().consume(tz);
    combine<false>(ret, HashConsumerImpl<SysTimeDataType, SysTime, Hash>().consume(val));
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Declared, T, Hash> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  u64
  consume(const T& val) {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(refs, val);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Instance, T, Hash> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  u64
  consume(const T& val) {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(refs, val.get());
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::MemberRef, T, Hash> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  u64
  consume(const T& val, const C& instance) {
    // For `MemberRef`, include the name in the hash
    u64 ret = Hash()(val.name());
    combine<false>(ret, HashConsumerImpl<ElemDataType, Elem, Hash>().consume(val.get(instance)));
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::VarRef, T, Hash> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  u64
  consume(const T& val) {
    // For `VarRef`, don't include the name in the hash
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(val.get());
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::CodePoint, T, Hash> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  u64
  consume(const T& val) {
    const Elem elem = static_cast<Elem>(val);
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Character, T, Hash> {
  using Elem = T::View;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  u64
  consume(const T& val) {
    const Elem elem = static_cast<Elem>(val);
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(elem);
  }
};

} // namespace internal

// `HashConsumer` (no pun intended) -------------------------------------------------------------------------

/**
 * The consumer for the #HashEncoder.
 *
 * @tparam Hash the hasher to use
 */
template<typename Hash>
struct HashConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::HashConsumerImpl<DataType, T, Hash>;
};

// `HashEncoder` --------------------------------------------------------------------------------------------

/**
 * The hash encoder.
 *
 * @tparam Hash the hasher to use. The hasher must be able to provide hash values for all primitive data
 *   types, including 128-bit data types, and strings
 */
template<typename Hash = BoostHash>
using HashEncoder = Encoder<HashConsumer<Hash>>;

} // namespace rocket::codec

// EOF
