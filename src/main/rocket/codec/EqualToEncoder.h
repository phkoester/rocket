/**
 * @file EqualToEncoder.h
 */

#pragma once

#include "rocket/functional.h"
#include "rocket/codec/codec.h"

#include <iostream>

namespace rocket::codec {

namespace internal {

// #EqualToConsumerImpl -------------------------------------------------------------------------------------

template<DataType DataType, typename T, typename Eq>
struct EqualToConsumerImpl;

template<typename Eq>
struct EqualToConsumerImpl<DataType::Bool, bool, Eq> {
  bool consume(bool lhs, bool rhs) { return Eq()(lhs, rhs); }
};

template<typename C, typename Eq>
struct EqualToConsumerImpl<DataType::Char, C, Eq> {
  bool consume(C lhs, C rhs) { return Eq()(lhs, rhs); }
};

template<typename E, typename Eq>
struct EqualToConsumerImpl<DataType::Enum, E, Eq> {
  bool consume(E lhs, E rhs) { return Eq()(lhs, rhs); }
};

template<typename I, typename Eq>
struct EqualToConsumerImpl<DataType::Integer, I, Eq> {
  bool consume(I lhs, I rhs) { return Eq()(lhs, rhs); }
};

template<typename F, typename Eq>
struct EqualToConsumerImpl<DataType::Float, F, Eq> {
  bool consume(F lhs, F rhs) { return Eq()(lhs, rhs); }
};

template<typename P, typename Eq>
struct EqualToConsumerImpl<DataType::Pointer, P, Eq> {
  bool consume(P lhs, P rhs) { return Eq()(lhs, rhs); }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::String, T, Eq> {
  bool consume(const T& lhs, const T& rhs) { return Eq()(lhs, rhs); }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Optional, T, Eq> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  bool
  consume(const T& lhs, const T& rhs) {
    if (not lhs && not rhs) {
      return true;
    }
    if (lhs && rhs) {
      return EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(*lhs, *rhs);
    }
    return false;
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Tuple, T, Eq> {
  template<typename... Args>
  bool
  consume(const T& lhs, const T& rhs, Args&&... args) {
    return consume(
      lhs,
      rhs,
      std::make_index_sequence<std::tuple_size<T>::value>(),
      std::forward<Args>(args)...
    );
  }

private:

  template<u64... Index, typename... Args>
  bool
  consume(const T& lhs, const T& rhs, std::index_sequence<Index...>, Args&&... args) {
    return (... && consumeElem( std::get<Index>(lhs), std::get<Index>(rhs), std::forward<Args>(args)...));
  }

  template<typename Elem, typename... Args>
  bool
  consumeElem(const Elem& lhs, const Elem& rhs, Args&&... args) {
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    return EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhs, rhs, std::forward<Args>(args)...);
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Array, T, Eq> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    while (lhsIt != lhs.end()) {
      const Elem& lhsElem = *lhsIt;
      const Elem& rhsElem = *rhsIt;
      const auto elemEq = EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhsElem, rhsElem);
      if (not elemEq) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Set, T, Eq> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;

  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    if constexpr (Unordered) {
      return consumeUnordered(lhs, rhs);
    }

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    while (lhsIt != lhs.end()) {
      const Elem& lhsElem = *lhsIt;
      const Elem& rhsElem = *rhsIt;
      const auto elemEq = EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhsElem, rhsElem);
      if (not elemEq) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }

private:

  bool
  consumeUnordered(const T& lhs, const T& rhs) {
    for (const auto& lhsElem : lhs) {
      auto it = rhs.find(lhsElem);
      if (it == rhs.end()) {
        return false;
      }
    }
    return true;
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Map, T, Eq> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;

  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    if constexpr (Unordered) {
      return consumeUnordered(lhs, rhs);
    }

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    while (lhsIt != lhs.end()) {
      const Key& lhsKey = lhsIt->first;
      const Key& rhsKey = rhsIt->first;
      const auto keyEq = EqualToConsumerImpl<KeyDataType, Key, Eq>().consume(lhsKey, rhsKey);
      if (not keyEq) {
        return false;
      }
      const Elem& lhsElem = lhsIt->second;
      const Elem& rhsElem = rhsIt->second;
      const auto elemEq = EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhsElem, rhsElem);
      if (not elemEq) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }

private:

  bool
  consumeUnordered(const T& lhs, const T& rhs) {
    for (const auto& [lhsKey, lhsElem] : lhs) {
      auto it = rhs.find(lhsKey);
      if (it == rhs.end()) {
        return false;
      }
      const auto& rhsElem = it->second;
      const auto elemEq = EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhsElem, rhsElem);
      if (not elemEq) {
        return false;
      }
    }
    return true;
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Bimap, T, Eq> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  static constexpr auto Unordered = IsUnordered<T>;

  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    if constexpr (Unordered) {
      return consumeUnordered(lhs, rhs);
    }

    auto lhsIt = lhs.left.begin(), rhsIt = rhs.left.begin();
    while (lhsIt != lhs.left.end()) {
      const Key& lhsKey = lhsIt->first;
      const Key& rhsKey = rhsIt->first;
      const auto keyEq = EqualToConsumerImpl<KeyDataType, Key, Eq>().consume(lhsKey, rhsKey);
      if (not keyEq) {
        return false;
      }
      const Elem& lhsElem = lhsIt->second;
      const Elem& rhsElem = rhsIt->second;
      const auto elemEq = EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhsElem, rhsElem);
      if (not elemEq) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }

private:

  bool
  consumeUnordered(const T& lhs, const T& rhs) {
    for (const auto& [lhsKey, lhsElem] : lhs.left) {
      auto it = rhs.left.find(lhsKey);
      if (it == rhs.left.end()) {
        return false;
      }
      const auto& rhsElem = it->second;
      const auto elemEq = EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhsElem, rhsElem);
      if (not elemEq) {
        return false;
      }
    }
    return true;
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Declared, T, Eq> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  bool
  consume(const T& lhs, const T& rhs) {
    // Here we have to pass two additional arguments, the left and right instances, to the tuple consumer.
    // The tuple consumer will pass them on to the member-reference consumer
    return EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(refs, refs, lhs, rhs);
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::Instance, T, Eq> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  bool
  consume(const T& lhs, const T& rhs) {
    // Here we have to pass two additional arguments, the left and right instances, to the tuple consumer.
    // The tuple consumer will pass them on to the member-reference consumer
    return EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(refs, refs, lhs.get(), rhs.get());
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::MemberRef, T, Eq> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  bool
  consume(const T& lhs, const T& rhs, const C& lhsInstance, const C& rhsInstance) {
    // For #MemberRef, compare the names
    if (lhs.name() != rhs.name()) {
      return false;
    }

    return EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(
      lhs.get(lhsInstance), rhs.get(rhsInstance));
  }
};

template<typename T, typename Eq>
struct EqualToConsumerImpl<DataType::VarRef, T, Eq> {
  using Elem = T::ValueType;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  bool
  consume(const T& lhs, const T& rhs) {
    // For #VarRef, don't compare the names
    return EqualToConsumerImpl<ElemDataType, Elem, Eq>().consume(lhs.get(), rhs.get());
  }
};

} // namespace internal

// #EqualToConsumer------------------------------------------------------------------------------------------

/**
 * The consumer for the #EqualToEncoder.
 *
 * @tparam Eq the comparator to use
 */
template<typename Eq>
struct EqualToConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::EqualToConsumerImpl<DataType, T, Eq>;
};

// #EqualToEncoder ------------------------------------------------------------------------------------------

/**
 * An equal-to encoder.
 *
 * @tparam Eq the comparator to use. The comparator must be able to compare all primitive data types,
 *   including 128-bit data types, and strings.
 */
template<typename Eq = StdEqualTo>
using EqualToEncoder = Encoder<EqualToConsumer<Eq>>;

} // namespace rocket::codec

// EOF
