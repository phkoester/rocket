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

// #HashConsumerImpl ----------------------------------------------------------------------------------------

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

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T, typename Hash>
struct HashConsumerImpl<DataType::Tuple, T, Hash> {
  template<typename... Args>
  u64
  consume(const T& val, Args&&... args) {
    u64 ret = std::tuple_size<T>::value;
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
struct HashConsumerImpl<DataType::Array, T, Hash> {
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
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(refs, *val.instance);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::MemberRef, T, Hash> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  u64
  consume(const T& val, const C& instance) {
    // For #MemberRef, include the name in the hash
    auto ret = Hash()(val.name());
    combine<false>(ret, HashConsumerImpl<ElemDataType, Elem, Hash>().consume(val.get(instance)));
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<DataType::VarRef, T, Hash> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  u64
  consume(const T& val) {
    // For #VarRef, don't include the name in the hash
    return HashConsumerImpl<ElemDataType, Elem, Hash>().consume(val.get());
  }
};

} // namespace internal

// #HashConsumer (no pun intended, I swear ...) -------------------------------------------------------------

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

// #HashEncoder ---------------------------------------------------------------------------------------------

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
