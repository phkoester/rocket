/**
 * @file CompareEncoder.h
 */

#pragma once

#include "rocket/functional.h"
#include "rocket/codec/codec.h"

namespace rocket::codec {

namespace internal {

// #CmpOrdering ---------------------------------------------------------------------------------------------

template<typename Cmp, typename T>
struct CmpOrderingImpl {
  using Type = decltype(Cmp()(std::declval<T>(), std::declval<T>()));
};

template<typename Cmp, typename C, typename T>
struct CmpOrderingImpl<Cmp, reflect::MemberRef<C, T>> {
  using Type = CmpOrderingImpl<Cmp, T>::Type;
};

template<typename Cmp, typename T, u64 Extent>
struct CmpOrderingImpl<Cmp, std::span<T, Extent>> {
  using Type = CmpOrderingImpl<Cmp, T>::Type;
};

template<typename Cmp, typename T>
using CmpOrdering = CmpOrderingImpl<Cmp, T>::Type;

// #CmpCommonOrdering ---------------------------------------------------------------------------------------

template<typename Cmp, typename... T>
struct CmpCommonOrderingImpl {
  using Type = std::common_comparison_category_t<CmpOrdering<Cmp, T>...>;
};

template<typename Cmp, typename... T>
struct CmpCommonOrderingImpl<Cmp, std::tuple<T...>> {
  using Type = CmpCommonOrderingImpl<Cmp, T...>::Type;
};

template<typename Cmp, typename... T>
using CmpCommonOrdering = CmpCommonOrderingImpl<Cmp, T...>::Type;

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

  using Ordering = CmpCommonOrdering<Cmp, bool, Elem>;

  Ordering
  consume(const T& lhs, const T& rhs) {
    const auto hasValueCmp = Cmp()(lhs.has_value(), rhs.has_value());
    if (not std::is_eq(hasValueCmp)) {
      return hasValueCmp;
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
  using Ordering = CmpCommonOrdering<Cmp, T>;

  template<typename... Args>
  Ordering
  consume(const T& lhs, const T& rhs, Args&&... args) {
    return consume(
      lhs,
      rhs,
      std::make_index_sequence<std::tuple_size<T>::value>(),
      std::forward<Args>(args)...);
  }

private:

  template<u64... Index, typename... Args>
  Ordering
  consume(const T& lhs, const T& rhs, std::index_sequence<Index...>, Args&&... args) {
    Ordering ret = std::strong_ordering::equal;
    (... && consumeElem(ret, std::get<Index>(lhs), std::get<Index>(rhs), std::forward<Args>(args)...));
    return ret;
  }

  template<typename Elem, typename... Args>
  bool
  consumeElem(Ordering& result, const Elem& lhs, const Elem& rhs, Args&&... args) {
    if (not std::is_eq(result)) {
      return false;
    }
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    result = CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(lhs, rhs, std::forward<Args>(args)...);
    return true;
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::List, T, Cmp> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  using Ordering = CmpOrdering<Cmp, Elem>;

  Ordering
  consume(const T& lhs, const T& rhs) {
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

  using Ordering = CmpOrdering<Cmp, Elem>;

  static constexpr auto Unordered = IsUnordered<T>;
  static_assert(not Unordered, "Cannot compare unordered sets");

  Ordering
  consume(const T& lhs, const T& rhs) {
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
struct CompareConsumerImpl<DataType::Map, T, Cmp> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  using Ordering = CmpCommonOrdering<Cmp, Key, Elem>;

  static constexpr auto Unordered = IsUnordered<T>;
  static_assert(not Unordered, "Cannot compare unordered maps");

  Ordering
  consume(const T& lhs, const T& rhs) {
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
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::Bimap, T, Cmp> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  using Ordering = CmpCommonOrdering<Cmp, Key, Elem>;

  static constexpr auto Unordered = IsUnordered<T>;
  static_assert(not Unordered, "Cannot compare unordered bimaps");

  Ordering
  consume(const T& lhs, const T& rhs) {
    const auto minSize = std::min(lhs.size(), rhs.size());

    auto lhsIt = lhs.left.begin(), rhsIt = rhs.left.begin();
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
    return CompareConsumerImpl<ElemDataType, Elem, Cmp>().consume(refs, refs, lhs.get(), rhs.get());
  }
};

template<typename T, typename Cmp>
struct CompareConsumerImpl<DataType::MemberRef, T, Cmp> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  using Ordering = CmpCommonOrdering<Cmp, std::string_view, Elem>;

  template<typename C>
  Ordering
  consume(const T& lhs, const T& rhs, const C& lhsInstance, const C& rhsInstance) {
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

  using Ordering = CmpOrdering<Cmp, Elem>;

  Ordering
  consume(const T& lhs, const T& rhs) {
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
 * A three-way-comparison encoder.
 *
 * Comparing unordered containers is not supported. #rocket::codec::EqualToEncoder can be used to compare
 * unordered containers for equality.
 *
 * @tparam Cmp the comparator to use. The comparator must be able to compare all primitive data types,
 *   including 128-bit data types, and strings. The result type must be any of #std::strong_ordering,
 *   #std::weak_ordering, or #std::partial_ordering.
 */
template<typename Cmp = StdCompare>
using CompareEncoder = Encoder<CompareConsumer<Cmp>>;

} // namespace rocket::codec

// EOF
