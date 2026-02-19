/**
 * @file CompareEncoder.h
 */

#pragma once

#include "rocket/functional.h"
#include "rocket/codec/codec.h"

namespace rocket::codec {

namespace internal {

template<typename Cmp, typename T>
struct CmpOrdering {
  using Type = decltype(Cmp()(std::declval<T>(), std::declval<T>()));
};

template<typename Cmp, typename C, typename T>
struct CmpOrdering<Cmp, reflect::MemberRef<C, T>> {
  using Type = CmpOrdering<Cmp, T>;
};

template<typename Cmp, typename T>
struct TupleCmpOrdering {
  using Type = CmpOrdering<Cmp, T>;
};

template<typename Cmp, typename C, typename... T>
struct TupleCmpOrdering<Cmp, std::tuple<reflect::MemberRef<C, T>...>> {
  using Type = TupleCmpOrdering<Cmp, std::tuple<T...>>::Type;
};

// #CompareConsumerImpl -------------------------------------------------------------------------------------

template<DataType DataType, typename T, typename Cmp>
struct CompareConsumerImpl;

template<typename Cmp>
struct CompareConsumerImpl<DataType::Bool, bool, Cmp> {
  auto consume(bool lhs, bool rhs) { return Cmp()(lhs, rhs); }
};

template<typename C, typename Cmp>
struct CompareConsumerImpl<DataType::Char, C, Cmp> {
  auto consume(C lhs, C rhs) { return Cmp()(lhs, rhs); }
};

template<typename E, typename Cmp>
struct CompareConsumerImpl<DataType::Enum, E, Cmp> {
  auto consume(E lhs, E rhs) { return Cmp()(lhs, rhs); }
};

template<typename I, typename Cmp>
struct CompareConsumerImpl<DataType::Integer, I, Cmp> {
  auto consume(I lhs, I rhs) { return Cmp()(lhs, rhs); }
};

template<typename F, typename Cmp>
struct CompareConsumerImpl<DataType::Float, F, Cmp> {
  auto consume(F lhs, F rhs) { return Cmp()(lhs, rhs); }
};

template<typename P, typename Cmp>
struct CompareConsumerImpl<DataType::Pointer, P, Cmp> {
  auto consume(P lhs, P rhs) { return Cmp()(lhs, rhs); }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::String, T, Cmp> {
  auto consume(const T& lhs, const T& rhs) { return Cmp()(lhs, rhs); }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Optional, T, Cmp> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  using BoolOrdering = CmpOrdering<Cmp, bool>;
  using ElemOrdering = CmpOrdering<Cmp, Elem>;
  using Ordering = CommonOrdering<BoolOrdering, ElemOrdering>;
  static_assert(std::is_same_v<Ordering, std::strong_ordering>); // XXX

  Ordering
  consume(const T& lhs, const T& rhs) {
    const auto boolCmp = Cmp()(lhs.has_value(), rhs.has_value());
    if (not std::is_eq(boolCmp)) {
      return boolCmp;
    }

    if (not lhs) {
      // Both optionals are null
      return std::strong_ordering::equal;
    }

    // Both optionals are nonnull
    return CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(*lhs, *rhs);
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Tuple, T, Cmp> {
  // XXX using Ordering = typename TupleCmpOrdering<Cmp, T>::Type;

  template<typename... Args>
  std::partial_ordering
  consume(const T& lhs, const T& rhs, Args&&... args) {
    return consume(
      lhs,
      rhs,
      std::make_index_sequence<std::tuple_size<T>::value>(),
      std::forward<Args>(args)...);
  }

private:

  template<u64... Index, typename... Args>
  std::partial_ordering
  consume(const T& lhs, const T& rhs, std::index_sequence<Index...>, Args&&... args) {
    std::partial_ordering ret = std::strong_ordering::equal;
    (... && consumeElem(ret, std::get<Index>(lhs), std::get<Index>(rhs), std::forward<Args>(args)...));
    return ret;
  }

  template<typename Elem, typename... Args>
  bool
  consumeElem(std::partial_ordering& result, const Elem& lhs, const Elem& rhs, Args&&... args) {
    if (not std::is_eq(result)) {
      return false;
    }
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    result = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhs, rhs, std::forward<Args>(args)...);
    return true;
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Array, T, Cmp> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  auto
  consume(const T& lhs, const T& rhs) ->
  decltype(CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(*lhs.begin(), *rhs.begin())) {
    const auto minSize = std::min(lhs.size(), rhs.size());

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    for (u64 i = 0; i < minSize; ++i) {
      const Elem& lhsElem = *lhsIt;
      const Elem& rhsElem = *rhsIt;
      const auto elemCmp = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhsElem, rhsElem);
      if (not std::is_eq(elemCmp)) {
        return elemCmp;
      }
      ++lhsIt; ++rhsIt;
    }

    return lhs.size() <=> rhs.size();
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Set, T, Cmp> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;
  using OrderedSet = std::set<Elem>;
  static constexpr auto OrderedSetDataType = DataTypes<OrderedSet>::Value;

  auto
  consume(const T& lhs, const T& rhs) ->
  decltype(CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(*lhs.begin(), *rhs.begin())) {
    if constexpr (Unordered) {
      return consumeUnordered(lhs, rhs);
    }

    const auto minSize = std::min(lhs.size(), rhs.size());

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    for (u64 i = 0; i < minSize; ++i) {
      const Elem& lhsElem = *lhsIt;
      const Elem& rhsElem = *rhsIt;
      const auto elemCmp = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhsElem, rhsElem);
      if (not std::is_eq(elemCmp)) {
        return elemCmp;
      }
      ++lhsIt; ++rhsIt;
    }

    return lhs.size() <=> rhs.size();
  }

private:

  /**
   * Compares unordered sets.
   *
   * For unordered sets, we copy the elements to a sorted set upfront.
   */
  auto
  consumeUnordered(const T& lhs, const T& rhs) {
    return CompareConsumerImpl<OrderedSetDataType, OrderedSet, Cmp>().consume(
      OrderedSet(lhs.begin(), lhs.end()),
      OrderedSet(rhs.begin(), rhs.end()));
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Map, T, Cmp> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;
  using OrderedKeys = std::set<Key>;

  std::partial_ordering
  consume(const T& lhs, const T& rhs) {
    if constexpr (Unordered) {
      return consumeUnordered(lhs, rhs);
    }

    const auto minSize = std::min(lhs.size(), rhs.size());

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    for (u64 i = 0; i < minSize; ++i) {
      const Key& lhsKey = lhsIt->first;
      const Key& rhsKey = rhsIt->first;
      const auto keyCmp = CompareConsumerImpl<KeyDataType, Key, Cmp>().consume(lhsKey, rhsKey);
      if (not std::is_eq(keyCmp)) {
        return keyCmp;
      }
      const Elem& lhsElem = lhsIt->second;
      const Elem& rhsElem = rhsIt->second;
      const auto elemCmp = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhsElem, rhsElem);
      if (not std::is_eq(elemCmp)) {
        return elemCmp;
      }
      ++lhsIt; ++rhsIt;
    }

    return lhs.size() <=> rhs.size();
  }

private:

  /**
   * Compares unordered maps.
   *
   * For unordered maps, we copy the keys to a sorted set upfront.
   */
  std::partial_ordering
  consumeUnordered(const T& lhs, const T& rhs) {
    OrderedKeys lhsKeys;
    for (const auto& [key, _] : lhs) {
      lhsKeys.insert(key);
    }

    OrderedKeys rhsKeys;
    for (const auto& [key, _] : rhs) {
      rhsKeys.insert(key);
    }

    const auto minSize = std::min(lhs.size(), rhs.size());

    auto lhsKeysIt = lhsKeys.begin(), rhsKeysIt = rhsKeys.begin();
    for (u64 i = 0; i < minSize; ++i) {
      const Key& lhsKey = *lhsKeysIt;
      const Key& rhsKey = *rhsKeysIt;
      const auto keyCmp = CompareConsumerImpl<KeyDataType, Key, Cmp>().consume(lhsKey, rhsKey);
      if (not std::is_eq(keyCmp)) {
        return keyCmp;
      }
      const Elem& lhsElem = lhs.find(lhsKey)->second;
      const Elem& rhsElem = rhs.find(rhsKey)->second;
      const auto elemCmp = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhsElem, rhsElem);
      if (not std::is_eq(elemCmp)) {
        return elemCmp;
      }
      ++lhsKeysIt; ++rhsKeysIt;
    }

    return lhs.size() <=> rhs.size();
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Bimap, T, Cmp> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;
  using OrderedKeys = std::set<Key>;

  std::partial_ordering
  consume(const T& lhs, const T& rhs) {
    if constexpr (Unordered) {
      return consumeUnordered(lhs, rhs);
    }

    const auto minSize = std::min(lhs.size(), rhs.size());

    auto lhsIt = lhs.left.begin(), rhsIt = rhs.left.begin();
    while (lhsIt != lhs.left.end()) {
      const Key& lhsKey = lhsIt->first;
      const Key& rhsKey = rhsIt->first;
      const auto keyCmp = CompareConsumerImpl<KeyDataType, Key, Cmp>().consume(lhsKey, rhsKey);
      if (not std::is_eq(keyCmp)) {
        return keyCmp;
      }
      const Elem& lhsElem = lhsIt->second;
      const Elem& rhsElem = rhsIt->second;
      const auto elemCmp = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhsElem, rhsElem);
      if (not std::is_eq(elemCmp)) {
        return elemCmp;
      }
      ++lhsIt; ++rhsIt;
    }

    return lhs.size() <=> rhs.size();
  }

private:

  /**
   * Compares unordered bimaps.
   *
   * For unordered bimaps, we copy the keys to a sorted set upfront.
   */
  std::partial_ordering
  consumeUnordered(const T& lhs, const T& rhs) {
    OrderedKeys lhsKeys;
    for (const auto& [key, _] : lhs.left) {
      lhsKeys.insert(key);
    }

    OrderedKeys rhsKeys;
    for (const auto& [key, _] : rhs.left) {
      rhsKeys.insert(key);
    }

    auto minSize = std::min(lhsKeys.size(), rhsKeys.size());

    auto lhsKeysIt = lhsKeys.begin(), rhsKeysIt = rhsKeys.begin();
    for (auto it = lhsKeysIt; it != lhsKeys.end(); ++it) {
      const Key& lhsKey = *lhsKeysIt;
      const Key& rhsKey = *rhsKeysIt;
      const auto keyCmp = CompareConsumerImpl<KeyDataType, Key, Cmp>().consume(lhsKey, rhsKey);
      if (not std::is_eq(keyCmp)) {
        return keyCmp;
      }
      const Elem& lhsElem = lhs.left.find(lhsKey)->second;
      const Elem& rhsElem = rhs.left.find(rhsKey)->second;
      const auto elemCmp = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhsElem, rhsElem);
      if (not std::is_eq(elemCmp)) {
        return elemCmp;
      }
      ++lhsKeysIt; ++rhsKeysIt;
    }

    return lhs.size() <=> rhs.size();
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Declared, T, Cmp> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  auto
  consume(const T& lhs, const T& rhs) {
    // Here we have to pass two additional arguments, the left and right instances, to the tuple consumer.
    // The tuple consumer will pass them on to the member-reference consumer
    return CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(refs, refs, lhs, rhs);
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Instance, T, Cmp> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  auto
  consume(const T& lhs, const T& rhs) {
    // Here we have to pass two additional arguments, the left and right instances, to the tuple consumer.
    // The tuple consumer will pass them on to the member-reference consumer
    return CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(
      refs, refs, *lhs.instance, *rhs.instance);
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::MemberRef, T, Cmp> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  auto
  consume(const T& lhs, const T& rhs, const C& lhsInstance, const C& rhsInstance) ->
    decltype(CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhs.get(lhsInstance), rhs.get(rhsInstance))) {
    // For #MemberRef, compare the names
    const auto nameCmp = Cmp()(lhs.name(), rhs.name());
    if (not std::is_eq(nameCmp)) {
      return nameCmp;
    }

    return CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhs.get(lhsInstance), rhs.get(rhsInstance));
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::VarRef, T, Cmp> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  auto
  consume(const T& lhs, const T& rhs) ->
   decltype(CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhs.get(), rhs.get())) {
    // For #VarRef, don't compare the names
    return CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhs.get(), rhs.get());
  }
};

} // namespace internal

// #CompareConsumer -----------------------------------------------------------------------------------------

/**
 * The consumer for the #CompareEncoder.
 *
 * @tparam Cmp the comparator to use
 */
template<typename Cmp>
struct CompareConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::CompareConsumerImpl<DataType, T, Cmp>;
};

// #CompareEncoder ------------------------------------------------------------------------------------------

/**
 * An three-way-compariosn encoder.
 *
 * @tparam Cmp the comparator to use. The comparator must be able to compare all primitive data types,
 *   including 128-bit data types, and strings.
 */
template<typename Cmp = StdCompare>
using CompareEncoder = Encoder<CompareConsumer<Cmp>>;

} // namespace rocket::codec

// EOF
