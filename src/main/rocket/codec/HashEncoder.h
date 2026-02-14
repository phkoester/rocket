/**
 * @file HashEncoder.h
 *
 * Hashing arbitrary C++ data structures.
 */

#pragma once

#include "rocket/codec/codec.h"
#include "rocket/hash/hash.h"

#include <boost/container_hash/hash.hpp>

namespace rocket::codec {

namespace internal {

// Functions ------------------------------------------------------------------------------------------------

template<bool Unordered>
void
combine(u64& seed, u64 hash) {
  if constexpr (Unordered) {
    // For unordered containers, use a permutative hashing scheme so that the element order does not
    // affect the hash value
    seed += hash;
  } else {
    // Otherwise, do it like the Boost guys do it
    hash::combine(seed, hash);
  }
}

// #HashConsumerImpl ----------------------------------------------------------------------------------------

template<ValueType ValueType, typename T, typename Hash>
struct HashConsumerImpl;

template<typename Hash>
struct HashConsumerImpl<ValueType::Bool, bool, Hash> {
  u64 consume(bool val) { return Hash()(val); }
};

template<typename C, typename Hash>
struct HashConsumerImpl<ValueType::Char, C, Hash> {
  u64 consume(C val) { return Hash()(val); }
};

template<typename E, typename Hash>
struct HashConsumerImpl<ValueType::Enum, E, Hash> {
  u64 consume(E val) { return Hash()(val); }
};

template<typename I, typename Hash>
struct HashConsumerImpl<ValueType::Integer, I, Hash> {
  u64 consume(I val) { return Hash()(val); }
};

template<typename F, typename Hash>
struct HashConsumerImpl<ValueType::Float, F, Hash> {
  u64 consume(F val) { return Hash()(val); }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Pointer, T, Hash> {
  u64 consume(T val) { return Hash()(val); }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::String, T, Hash> {
  u64 consume(const T& val) { return Hash()(val); }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Optional, T, Hash> {
  u64
  consume(const T& val) {
    if (not val) {
      return Hash()(0);
    }

    using Elem = T::value_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    u64 ret = 1;
    const u64 elemHash = HashConsumerImpl<ElemEncode, Elem, Hash>().consume(*val);
    combine<false>(ret, elemHash);
    return ret;
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Tuple, T, Hash> {
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
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;
    auto elemHash = HashConsumerImpl<ElemEncode, Elem, Hash>().consume(elem, std::forward<Args>(args)...);
    combine<false>(seed, elemHash);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Array, T, Hash> {
  u64 consume(const T& val) {
    using Elem = T::value_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    u64 ret = val.size();
    for (const auto& elem : val) {
      auto elemHash = HashConsumerImpl<ElemEncode, Elem, Hash>().consume(elem);
      combine<false>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Set, T, Hash> {
  u64
  consume(const T& val) {
    using Elem = T::value_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    constexpr auto Unordered = IsHashed<T>;

    u64 ret = val.size();
    for (const auto& elem : val) {
      auto elemHash = HashConsumerImpl<ElemEncode, Elem, Hash>().consume(elem);
      combine<Unordered>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Map, T, Hash> {
  u64
  consume(const T& val) {
    using Key = T::key_type;
    constexpr auto KeyEncode = ValueTypes<Key>::Encode;
    using Elem = T::mapped_type;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    constexpr auto Unordered = IsHashed<T>;

    u64 ret = val.size();
    for (const auto& [key, elem] : val) {
      const auto keyHash = HashConsumerImpl<KeyEncode, Key, Hash>().consume(key);
      combine<Unordered>(ret, keyHash);
      const auto elemHash = HashConsumerImpl<ElemEncode, Elem, Hash>().consume(elem);
      combine<Unordered>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Bimap, T, Hash> {
  u64
  consume(const T& val) {
    using Key = PurgeType<typename T::left_value_type::first_type>;
    constexpr auto KeyEncode = ValueTypes<Key>::Encode;
    using Elem = PurgeType<typename T::left_value_type::second_type>;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    constexpr auto Unordered = IsHashed<T>;

    u64 ret = val.size();
    for (const auto& [key, elem] : val.left) {
      const auto keyHash = HashConsumerImpl<KeyEncode, Key, Hash>().consume(key);
      combine<Unordered>(ret, keyHash);
      const auto elemHash = HashConsumerImpl<ElemEncode, Elem, Hash>().consume(elem);
      combine<Unordered>(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::MemberRefProvider, T, Hash> {
  u64
  consume(const T& val) {
    constexpr auto& refs = rocket::reflect::MemberRefProvider<T>::refs;
    using Elem = PurgeType<decltype(refs)>;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;
    static_assert(ElemEncode == ValueType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    return HashConsumerImpl<ElemEncode, Elem, Hash>().consume(refs, val);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::MemberRef, T, Hash> {
  template<typename C>
  u64
  consume(const T& val, const C& instance) {
    using Elem = T::ValueType;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    // We don't include the name in the hash, because it's not part of the value
    return HashConsumerImpl<ElemEncode, Elem, Hash>().consume(val.get(instance));
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::VarRef, T, Hash> {
  u64
  consume(const T& val) {
    using Elem = T::ValueType;
    constexpr auto ElemEncode = ValueTypes<Elem>::Encode;

    // We don't include the name in the hash, because it's not part of the value
    return HashConsumerImpl<ElemEncode, Elem, Hash>().consume(val.get());
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
  template<ValueType ValueType, typename T>
  using Type = internal::HashConsumerImpl<ValueType, T, Hash>;
};

// #HashEncoder ---------------------------------------------------------------------------------------------

/**
 * The hash encoder.
 *
 * @tparam Hash the hasher to use
 */
template<typename Hash = hash::BoostHash>
using HashEncoder = Encoder<HashConsumer<Hash>>;

} // namespace rocket::codec

// EOF
