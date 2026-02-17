/**
 * @file EqualToEncoder.h
 */

#pragma once

#include "rocket/functional.h"
#include "rocket/codec/codec.h"

namespace rocket::codec {

namespace internal {

// #EqualToConsumerImpl -------------------------------------------------------------------------------------

template<DataType DataType, typename T, typename EqualTo>
struct EqualToConsumerImpl;

template<typename EqualTo>
struct EqualToConsumerImpl<DataType::Bool, bool, EqualTo> {
  bool consume(bool lhs, bool rhs) { return EqualTo()(lhs, rhs); }
};

template<typename C, typename EqualTo>
struct EqualToConsumerImpl<DataType::Char, C, EqualTo> {
  bool consume(C lhs, C rhs) { return EqualTo()(lhs, rhs); }
};

template<typename E, typename EqualTo>
struct EqualToConsumerImpl<DataType::Enum, E, EqualTo> {
  bool consume(E lhs, E rhs) { return EqualTo()(lhs, rhs); }
};

template<typename I, typename EqualTo>
struct EqualToConsumerImpl<DataType::Integer, I, EqualTo> {
  bool consume(I lhs, I rhs) { return EqualTo()(lhs, rhs); }
};

template<typename F, typename EqualTo>
struct EqualToConsumerImpl<DataType::Float, F, EqualTo> {
  bool consume(F lhs, F rhs) { return EqualTo()(lhs, rhs); }
};

template<typename P, typename EqualTo>
struct EqualToConsumerImpl<DataType::Pointer, P, EqualTo> {
  bool consume(P lhs, P rhs) { return EqualTo()(lhs, rhs); }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::String, T, EqualTo> {
  bool consume(const T& lhs, const T& rhs) { return EqualTo()(lhs, rhs); }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Optional, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    if (not lhs && not rhs) {
      return true;
    }
    if (lhs && rhs) {
      using Elem = T::value_type;
      constexpr auto ElemDataType = DataTypes<Elem>::Value;
      return EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(*lhs, *rhs);
    }
    return false;
  }
};

// For #MemberRef, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Tuple, T, EqualTo> {
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
    return EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(lhs, rhs, std::forward<Args>(args)...);
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Array, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    for (u64 i = 0; i < size; ++i) {
      const Elem& lhsElem = lhs[i];
      const Elem& rhsElem = rhs[i];
      const auto elemEqualTo = EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(lhsElem, rhsElem);
      if (!elemEqualTo) {
        return false;
      }
    }

    return true;
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Set, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    using Elem = T::value_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    constexpr auto Unordered = IsHashed<T>;
    if constexpr (Unordered) {
      using Ordered = std::set<Elem>;
      constexpr auto OrderedDataType = DataTypes<Ordered>::Value;
      return EqualToConsumerImpl<OrderedDataType, Ordered, EqualTo>().consume(
        Ordered(lhs.begin(), lhs.end()),
        Ordered(rhs.begin(), rhs.end())
      );
    }

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    while (lhsIt != lhs.end()) {
      const Elem& lhsElem = *lhsIt;
      const Elem& rhsElem = *rhsIt;
      const auto elemEqualTo = EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(lhsElem, rhsElem);
      if (not elemEqualTo) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Map, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    using Key = T::key_type;
    constexpr auto KeyDataType = DataTypes<Key>::Value;
    using Elem = T::mapped_type;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    constexpr auto Unordered = IsHashed<T>;
    if constexpr (Unordered) {
      using Ordered = std::map<Key, Elem>; // @todo Copy keys only, not the values
      constexpr auto OrderedDataType = DataTypes<Ordered>::Value;
      return EqualToConsumerImpl<OrderedDataType, Ordered, EqualTo>().consume(
        Ordered(lhs.begin(), lhs.end()),
        Ordered(rhs.begin(), rhs.end())
      );
    }

    auto lhsIt = lhs.begin(), rhsIt = rhs.begin();
    while (lhsIt != lhs.end()) {
      const Key& lhsKey = lhsIt->first;
      const Key& rhsKey = rhsIt->first;
      const auto keyEqualTo = EqualToConsumerImpl<KeyDataType, Key, EqualTo>().consume(lhsKey, rhsKey);
      if (not keyEqualTo) {
        return false;
      }
      const Elem& lhsElem = lhsIt->second;
      const Elem& rhsElem = rhsIt->second;
      const auto elemEqualTo = EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(lhsElem, rhsElem);
      if (not elemEqualTo) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Bimap, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    const auto size = lhs.size();
    if (size != rhs.size()) {
      return false;
    }

    using Key = PurgeType<typename T::left_value_type::first_type>;
    constexpr auto KeyDataType = DataTypes<Key>::Value;
    using Elem = PurgeType<typename T::left_value_type::second_type>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    constexpr auto Unordered = IsHashed<T>;
    if constexpr (Unordered) {
      using Ordered = std::map<Key, Elem>; // @todo Copy keys only, not the values
      constexpr auto OrderedDataType = DataTypes<Ordered>::Value;
      return EqualToConsumerImpl<OrderedDataType, Ordered, EqualTo>().consume(
        Ordered(lhs.left.begin(), lhs.left.end()),
        Ordered(rhs.left.begin(), rhs.left.end())
      );
    }

    auto lhsIt = lhs.left.begin(), rhsIt = rhs.left.begin();
    while (lhsIt != lhs.left.end()) {
      const Key& lhsKey = lhsIt->first;
      const Key& rhsKey = rhsIt->first;
      const auto keyEqualTo = EqualToConsumerImpl<KeyDataType, Key, EqualTo>().consume(lhsKey, rhsKey);
      if (not keyEqualTo) {
        return false;
      }
      const Elem& lhsElem = lhsIt->second;
      const Elem& rhsElem = rhsIt->second;
      const auto elemEqualTo = EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(lhsElem, rhsElem);
      if (not elemEqualTo) {
        return false;
      }
      ++lhsIt; ++rhsIt;
    }
    return true;
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Declared, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    constexpr auto& refs = rocket::reflect::Declared<T>::refs;
    using Elem = PurgeType<decltype(refs)>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    static_assert(ElemDataType == DataType::Tuple);

    // Here we have to pass two additional arguments, the left and right instances, to the tuple consumer.
    // The tuple consumer will pass them on to the member-reference consumer
    return EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(
      refs, refs, lhs, rhs);
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::Instance, T, EqualTo> {
  bool
  consume(const T& lhs, const T& rhs) {
    constexpr auto& refs = T::InnerType::refs;
    using Elem = PurgeType<decltype(refs)>;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    static_assert(ElemDataType == DataType::Tuple);

    // Here we have to pass two additional arguments, the left and right instances, to the tuple consumer.
    // The tuple consumer will pass them on to the member-reference consumer
    return EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(
      refs, refs, *lhs.instance, *rhs.instance);
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::MemberRef, T, EqualTo> {
  template<typename C>
  bool
  consume(const T& lhs, const T& rhs, const C& lhsInstance, const C& rhsInstance) {
    if (lhs.name() != rhs.name()) {
      return false;
    }

    using Elem = T::ValueType;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;

    return EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(
      lhs.get(lhsInstance), rhs.get(rhsInstance));
  }
};

template<typename T, typename EqualTo>
struct EqualToConsumerImpl<DataType::VarRef, T, EqualTo> {
  bool
  consume(const T& val) {
    // We don't compare the names here

    using Elem = T::ValueType;
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    return EqualToConsumerImpl<ElemDataType, Elem, EqualTo>().consume(val.get());
  }
};

} // namespace internal

// #EqualToConsumer------------------------------------------------------------------------------------------

/**
 * The consumer for the #EqualToEncoder.
 *
 * @tparam EqualTo the comparator to use
 */
template<typename EqualTo>
struct EqualToConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::EqualToConsumerImpl<DataType, T, EqualTo>;
};

// #EqualToEncoder ------------------------------------------------------------------------------------------

/**
 * An equal-to encoder.
 *
 * @tparam EqualTo the comparator to use. The comparator must be able to compare all primitive data types,
 *   including 128-bit data types, and strings.
 */
template<typename EqualTo = StdEqualTo>
using EqualToEncoder = Encoder<EqualToConsumer<EqualTo>>;

} // namespace rocket::codec

// EOF
