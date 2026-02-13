/**
 * @file HashEncoder.h
 *
 * Hashing arbitrary C++ data structures.
 */

#pragma once

#include "rocket/codec/codec.h"
#include "rocket/hash/hash.h"

namespace rocket::codec {

namespace internal {

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
  u64 consume(E val) {
    // Needed by MSVC
    const auto underlying = std::to_underlying(val);
    return Hash()(underlying);
  }
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
  u64 consume(T val) {
    // Needed by MSVC
    const u64 integer = reinterpret_cast<u64>(val);
    static_assert(sizeof(integer) == sizeof(T));
    return Hash()(integer);
  }
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
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    u64 ret = 1;
    const u64 elemHash = HashConsumerImpl<elemValueType, Elem, Hash>().consume(*val);
    hash::combine(ret, elemHash);
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
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    auto elemHash = HashConsumerImpl<elemValueType, Elem, Hash>().consume(elem, std::forward<Args>(args)...);
    hash::combine(seed, elemHash);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Array, T, Hash> {
  u64 consume(const T& val) {
    using Elem = T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    u64 ret = val.size();
    for (const auto& elem : val) {
      auto elemHash = HashConsumerImpl<elemValueType, Elem, Hash>().consume(elem);
      hash::combine(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Set, T, Hash> {
  u64
  consume(const T& val) {
    using Elem = T::value_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    u64 ret = val.size();
    for (const auto& elem : val) {
      auto elemHash = HashConsumerImpl<elemValueType, Elem, Hash>().consume(elem);
      hash::combine(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Map, T, Hash> {
  u64
  consume(const T& val) {
    using Key = T::key_type;
    constexpr auto keyValueType = ValueTypes<Key>::value;
    using Elem = T::mapped_type;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    u64 ret = val.size();
    for (const auto& [key, elem] : val) {
      const auto keyHash = HashConsumerImpl<keyValueType, Key, Hash>().consume(key);
      hash::combine(ret, keyHash);
      const auto elemHash = HashConsumerImpl<elemValueType, Elem, Hash>().consume(elem);
      hash::combine(ret, elemHash);
    }
    return ret;
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::Bimap, T, Hash> {
  u64
  consume(const T& val) {
    using Key = PurgeType<typename T::left_value_type::first_type>;
    constexpr auto keyValueType = ValueTypes<Key>::value;
    using Elem = PurgeType<typename T::left_value_type::second_type>;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    u64 ret = val.size();
    for (const auto& [key, elem] : val.left) {
      const auto keyHash = HashConsumerImpl<keyValueType, Key, Hash>().consume(key);
      hash::combine(ret, keyHash);
      const auto elemHash = HashConsumerImpl<elemValueType, Elem, Hash>().consume(elem);
      hash::combine(ret, elemHash);
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
    constexpr auto elemValueType = ValueTypes<Elem>::value;
    static_assert(elemValueType == ValueType::Tuple);

    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    return HashConsumerImpl<elemValueType, Elem, Hash>().consume(refs, val);
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::MemberRef, T, Hash> {
  template<typename C>
  u64
  consume(const T& val, const C& instance) {
    using Elem = T::ValueType;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    // We don't include the name in the hash, because it's not part of the value
    return HashConsumerImpl<elemValueType, Elem, Hash>().consume(val.get(instance));
  }
};

template<typename T, typename Hash>
struct HashConsumerImpl<ValueType::VarRef, T, Hash> {
  u64
  consume(const T& val) {
    using Elem = T::ValueType;
    constexpr auto elemValueType = ValueTypes<Elem>::value;

    // We don't include the name in the hash, because it's not part of the value
    return HashConsumerImpl<elemValueType, Elem, Hash>().consume(val.get());
  }
};

} // namespace internal

// #StandardHash --------------------------------------------------------------------------------------------

/**
 * The default hasher, which uses #std::hash.
 */
struct StandardHash {
  /**
   * Hash function.
   *
   * @tparam T the type of the value to hash
   * @param val the value to hash
   * @return the hash value
   */
  template<typename T>
  [[nodiscard]] u64
  operator()(const T& val) const {
    return std::hash<T>()(val);
  }
};

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
template<typename Hash = StandardHash>
using HashEncoder = Encoder<HashConsumer<Hash>>;

} // namespace rocket::codec

// EOF
