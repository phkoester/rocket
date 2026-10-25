/**
 * @file CborCodec.h
 *
 * A CBOR codec, using libcbor.
 */

#pragma once

#include "rocket/Guard.h"
#include "rocket/codec/codec.h"

#include <cbor.h>
#include <iterator>

namespace rocket::codec {

namespace internal::cbor {

// `CborItem` -----------------------------------------------------------------------------------------------

/**
 * A managed CBOR item with move semantics. It helps to avoid leaks when exceptions are thrown. More
 * generally, it serves as a bridge between a C API and the Rocket codec consumers and producers that make
 * use of exceptions.
 */
template<typename T>
struct CborItem {
  explicit CborItem(cbor_item_t* ptr) : ptr_(ptr) {
    ROCKET_CHECK(ptr, ptr != nullptr);
    ROCKET_ASSERT(cbor_refcount(ptr) > 0);
  }

  CborItem(const CborItem&) = delete;

  CborItem(CborItem&& rhs) noexcept : ptr_(rhs.ptr_), val_(std::move(rhs.val_)) {
    rhs.ptr_ = nullptr;
    rhs.val_ = std::nullopt;
  }

  ~CborItem() {
    if (ptr_ != nullptr) {
      ROCKET_ASSERT(cbor_refcount(ptr_) > 0);
      cbor_decref(&ptr_);
    }
    val_ = std::nullopt;
  }

  CborItem& operator=(const CborItem& rhs) = delete;

  CborItem& operator=(CborItem&& rhs) = delete;

  [[nodiscard]] cbor_item_t*
  ptr() {
    ROCKET_EXPECT(ptr_ != nullptr);
    return ptr_;
  }

  /// May only be called once.
  void
  setValue(T&& val) {
    ROCKET_EXPECT(not val_);
    val_ = std::move(val);
  }

  /// This is a one-shot move operation.
  [[nodiscard]] T
  value() {
    ROCKET_EXPECT(val_.has_value());
    T val = std::move(*val_);
    val_ = std::nullopt;
    return val;
  }

private:

  cbor_item_t* ptr_; ///< The managed CBOR item.
  /// The carried C++ value, if any. It can be massive, which is why we move so often in this implementation.
  std::optional<T> val_;
};

// `DecoderState` -------------------------------------------------------------------------------------------

struct DecoderState {
  u64 index;

  void nextItem() { ++index; }
};

// Functions ------------------------------------------------------------------------------------------------

std::string_view messageOf(cbor_error_code errorCode);

u64 positionOf(std::span<const u8> cbor, u64 index);

// `CborConsumerImpl` ---------------------------------------------------------------------------------------

template<DataType DataType, typename T>
struct CborConsumerImpl;

template<>
struct CborConsumerImpl<DataType::Bool, bool> {
  [[nodiscard]] CborItem<bool>
  consume(bool val) const {
    return CborItem<bool>(cbor_build_bool(val));
  }
};

template<typename C>
struct CborConsumerImpl<DataType::Char, C> {
  [[nodiscard]] CborItem<C>
  consume(C val) const {
    if constexpr (std::same_as<C, char>) {
      return CborItem<C>(cbor_build_uint8(static_cast<u8>(val)));
    } else {
      return CborItem<C>(cbor_build_uint32(static_cast<u32>(val)));
    }
  }
};

template<typename E>
struct CborConsumerImpl<DataType::Enum, E> {
  using Underlying = std::underlying_type_t<E>;
  static constexpr auto UnderlyingDataType = DataTypes<Underlying>::Value;

  [[nodiscard]] CborItem<E>
  consume(E val) const {
    auto underlying =
      CborConsumerImpl<UnderlyingDataType, Underlying>().consume(static_cast<Underlying>(val));
    return CborItem<E>(cbor_incref(underlying.ptr()));
  }
};

template<typename I>
struct CborConsumerImpl<DataType::Integer, I> {
  [[nodiscard]] CborItem<I>
  consume(I val) const {
    if constexpr (sizeof(I) == 1) {
      return CborItem<I>(cbor_build_uint8(static_cast<u8>(val)));
    } else if constexpr (sizeof(I) == 2) {
      return CborItem<I>(cbor_build_uint16(static_cast<u16>(val)));
    } else if constexpr (sizeof(I) == 4) {
      return CborItem<I>(cbor_build_uint32(static_cast<u32>(val)));
    } else if constexpr (sizeof(I) == 8) {
      return CborItem<I>(cbor_build_uint64(static_cast<u64>(val)));
    } else if constexpr (sizeof(I) == 16) {
      CborItem<I> ret(cbor_new_definite_array(2));
      CborItem<u64> hi = CborItem<u64>(cbor_build_uint64(static_cast<u64>(val >> 64)));
      ROCKET_EXPECT(cbor_array_push(ret.ptr(), hi.ptr()));
      CborItem<u64> lo = CborItem<u64>(cbor_build_uint64(static_cast<u64>(val)));
      ROCKET_EXPECT(cbor_array_push(ret.ptr(), lo.ptr()));
      return ret;
    } else {
      ROCKET_TERMINATE("Cannot consume integer of type `{}`", typeid(I));
    }
  }
};

template<typename F>
struct CborConsumerImpl<DataType::Float, F> {
  [[nodiscard]] CborItem<F>
  consume(F val) const {
    if constexpr (sizeof(F) == 4) {
      return CborItem<F>(cbor_build_float4(static_cast<f32>(val)));
    } else if constexpr (sizeof(F) == 8) {
      return CborItem<F>(cbor_build_float8(static_cast<f64>(val)));
    } else {
      ROCKET_TERMINATE("Cannot consume float of type `{}`", typeid(F));
    }
  }
};

template<typename P>
struct CborConsumerImpl<DataType::Pointer, P> {
  using Alt = Int<sizeof(P)>::Type;
  static constexpr auto AltDataType = DataTypes<Alt>::Value;

  [[nodiscard]] CborItem<P>
  consume(P val) const {
    auto alt = CborConsumerImpl<AltDataType, Alt>().consume(reinterpret_cast<Alt>(val));
    return CborItem<P>(cbor_incref(alt.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::String, T> {
  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    const std::string utf8(unicode::ConvertTo<char>::apply(val));
    return CborItem<T>(cbor_build_stringn(utf8.c_str(), utf8.size()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    if (not val) {
      return CborItem<T>(cbor_new_null());
    }
    CborItem<Elem> elem = CborConsumerImpl<ElemDataType, Elem>().consume(*val);
    return CborItem<T>(cbor_incref(elem.ptr()));
  }
};

// For `MemberRef`, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct CborConsumerImpl<DataType::Tuple, T> {
  template<typename... Args>
  [[nodiscard]] CborItem<T>
  consume(const T& val, Args&&... args) const {
    CborItem<T> arr(cbor_new_definite_array(std::tuple_size_v<T>));
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg), arr, std::forward<Args>(args)...), ...);
    }, val);
    return arr;
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(const Elem& elem, CborItem<T>& arr, Args&&... args) const {
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    CborItem<Elem> item = CborConsumerImpl<ElemDataType, Elem>().consume(elem, std::forward<Args>(args)...);
    ROCKET_EXPECT(cbor_array_push(arr.ptr(), item.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::List, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    CborItem<T> arr(cbor_new_definite_array(sizeOf(val)));
    for (const auto& elem : val) {
      CborItem<Elem> item = CborConsumerImpl<ElemDataType, Elem>().consume(elem);
      ROCKET_EXPECT(cbor_array_push(arr.ptr(), item.ptr()));
    }
    return arr;
  }

private:

  [[nodiscard]] u64
  sizeOf(const T& val) const {
    if constexpr (IsForwardList<T>) {
      // `std::forward_list` has no `size()`
      return static_cast<u64>(std::distance(val.begin(), val.end()));
    } else {
      return static_cast<u64>(val.size());
    }
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    CborItem<T> arr(cbor_new_definite_array(static_cast<u64>(val.size())));
    for (const auto& elem : val) {
      CborItem<Elem> item = CborConsumerImpl<ElemDataType, Elem>().consume(elem);
      ROCKET_EXPECT(cbor_array_push(arr.ptr(), item.ptr()));
    }
    return arr;
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Map, T> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    CborItem<T> map(cbor_new_definite_map(static_cast<u64>(val.size())));
    for (const auto& [key, elem] : val) {
      CborItem<Key> keyItem = CborConsumerImpl<KeyDataType, Key>().consume(key);
      CborItem<Elem> elemItem = CborConsumerImpl<ElemDataType, Elem>().consume(elem);
      ROCKET_EXPECT(cbor_map_add(map.ptr(), { .key=keyItem.ptr(), .value=elemItem.ptr() }));
    }
    return map;
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    CborItem<T> map(cbor_new_definite_map(static_cast<u64>(val.size())));
    for (const auto& [key, elem] : val.left) {
      CborItem<Key> keyItem = CborConsumerImpl<KeyDataType, Key>().consume(key);
      CborItem<Elem> elemItem = CborConsumerImpl<ElemDataType, Elem>().consume(elem);
      ROCKET_EXPECT(cbor_map_add(map.ptr(), { .key=keyItem.ptr(), .value=elemItem.ptr() }));
    }
    return map;
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Duration, T> {
  using Rep = T::rep;
  static constexpr auto RepDataType = DataTypes<Rep>::Value;

  [[nodiscard]] CborItem<T>
  consume(T val) const { // Take by value
    auto count = CborConsumerImpl<RepDataType, Rep>().consume(val.count());
    return CborItem<T>(cbor_incref(count.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Date, T> {
  using Rep = std::chrono::sys_days::duration::rep;
  static constexpr auto RepDataType = DataTypes<Rep>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    const auto count = std::chrono::sys_days(val).time_since_epoch().count();
    auto item = CborConsumerImpl<RepDataType, Rep>().consume(count);
    return CborItem<T>(cbor_incref(item.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::HourMinuteSecond, T> {
  using Precision = T::precision;
  static constexpr auto PrecisionDataType = DataTypes<Precision>::Value;
  static_assert(PrecisionDataType == DataType::Duration);

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    auto duration = CborConsumerImpl<PrecisionDataType, Precision>().consume(val.to_duration());
    return CborItem<T>(cbor_incref(duration.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::TimeZone, T> {
  [[nodiscard]] CborItem<T>
  consume(T val) const {
    const std::string name(val->name());
    auto item = CborConsumerImpl<DataType::String, std::string>().consume(name);
    return CborItem<T>(cbor_incref(item.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "Clock must be `system_clock`");
  using Duration = T::duration;
  static constexpr auto DurationDataType = DataTypes<Duration>::Value;
  static_assert(DurationDataType == DataType::Duration);

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    auto epoch = CborConsumerImpl<DurationDataType, Duration>().consume(val.time_since_epoch());
    return CborItem<T>(cbor_incref(epoch.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::ZonedTime, T> {
  using Duration = T::duration;
  using SysTime = std::chrono::sys_time<Duration>;
  static constexpr auto SysTimeDataType = DataTypes<SysTime>::Value;
  static_assert(SysTimeDataType == DataType::Time);
  static constexpr auto TimeZoneDataType = DataTypes<TimeZone>::Value;
  static_assert(TimeZoneDataType == DataType::TimeZone);

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    CborItem<T> arr(cbor_new_definite_array(2));
    CborItem<TimeZone> zone = CborConsumerImpl<TimeZoneDataType, TimeZone>().consume(val.get_time_zone());
    ROCKET_EXPECT(cbor_array_push(arr.ptr(), zone.ptr()));
    CborItem<SysTime> time = CborConsumerImpl<SysTimeDataType, SysTime>().consume(val.get_sys_time());
    ROCKET_EXPECT(cbor_array_push(arr.ptr(), time.ptr()));
    return arr;
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    if (val.empty()) {
      return CborItem<T>(cbor_new_null());
    }

    CborItem<T> arr(cbor_new_definite_array(2));
    CborItem<A> a = CborConsumerImpl<ADataType, A>().consume(val.a);
    ROCKET_EXPECT(cbor_array_push(arr.ptr(), a.ptr()));
    CborItem<B> b = CborConsumerImpl<BDataType, B>().consume(val.b);
    ROCKET_EXPECT(cbor_array_push(arr.ptr(), b.ptr()));
    return arr;
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    // Pass the instance on to the tuple consumer, which forwards it to each member reference
    auto item = CborConsumerImpl<ElemDataType, Elem>().consume(refs, val);
    return CborItem<T>(cbor_incref(item.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    // Pass the instance on to the tuple consumer, which forwards it to each member reference
    auto item = CborConsumerImpl<ElemDataType, Elem>().consume(refs, val.get());
    return CborItem<T>(cbor_incref(item.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  [[nodiscard]] CborItem<T>
  consume(const T& val, const C& instance) const {
    auto elem = CborConsumerImpl<ElemDataType, Elem>().consume(val.get(instance));
    return CborItem<T>(cbor_incref(elem.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    auto elem = CborConsumerImpl<ElemDataType, Elem>().consume(val.get());
    return CborItem<T>(cbor_incref(elem.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::CodePoint, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    auto elem = CborConsumerImpl<ElemDataType, Elem>().consume(static_cast<Elem>(val));
    return CborItem<T>(cbor_incref(elem.ptr()));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Character, T> {
  using Elem = T::View;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  [[nodiscard]] CborItem<T>
  consume(const T& val) const {
    auto elem = CborConsumerImpl<ElemDataType, Elem>().consume(static_cast<Elem>(val));
    return CborItem<T>(cbor_incref(elem.ptr()));
  }
};

// `CborProducerImpl` ---------------------------------------------------------------------------------------

template<DataType DataType, typename T>
struct CborProducerImpl;

template<typename Ref, typename Item, typename Instance>
void
assignMember(const Ref& ref, Item& arr, DecoderState& state, u64 index, Instance& instance) {
  using Member = typename Ref::Type;
  constexpr auto MemberDataType = DataTypes<Member>::Value;
  CborItem<Member> member(cbor_array_get(arr.ptr(), index));
  CborProducerImpl<MemberDataType, Member>().produce(member, state);
  ref.get(instance) = std::move(member.value());
}

template<typename Refs, typename Item, typename Instance>
void
produceObject(const Refs& refs, Item& item, DecoderState& state, Instance& instance) {
  state.nextItem();

  if (not cbor_isa_array(item.ptr()) || cbor_array_size(item.ptr()) != std::tuple_size_v<Refs>) {
    throw InputFailure(state.index, "Expected an object");
  }

  u64 index = 0;
  std::apply([&](const auto&... ref) {
    (assignMember(ref, item, state, index++, instance), ...);
  }, refs);
}

template<>
struct CborProducerImpl<DataType::Bool, bool> {
  void
  produce(CborItem<bool>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_is_bool(item.ptr())) {
      throw InputFailure(state.index, "Expected a boolean value");
    }

    item.setValue(cbor_get_bool(item.ptr()));
  }
};

template<typename C>
struct CborProducerImpl<DataType::Char, C> {
  void
  produce(CborItem<C>& item, DecoderState& state) const {
    state.nextItem();

    if constexpr (std::same_as<C, char>) {
      if (cbor_isa_uint(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_8) {
        item.setValue(static_cast<char>(cbor_get_uint8(item.ptr())));
        return;
      }
    } else {
      if (cbor_isa_uint(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_32) {
        item.setValue(static_cast<char32>(cbor_get_uint32(item.ptr())));
        return;
      }
    }

    throw InputFailure(state.index, "Expected a character");
  }
};

template<typename E>
struct CborProducerImpl<DataType::Enum, E> {
  using Underlying = std::underlying_type_t<E>;
  static constexpr auto UnderlyingDataType = DataTypes<Underlying>::Value;

  void
  produce(CborItem<E>& item, DecoderState& state) const {
    CborItem<Underlying> underlying(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<UnderlyingDataType, Underlying>().produce(underlying, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected an enumeration value");
    }
    item.setValue(static_cast<E>(underlying.value()));
  }
};

template<typename I>
struct CborProducerImpl<DataType::Integer, I> {
  void
  produce(CborItem<I>& item, DecoderState& state) const {
    state.nextItem();

    if constexpr (sizeof(I) == 1) {
      if (cbor_isa_uint(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_8) {
        item.setValue(static_cast<I>(cbor_get_uint8(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 2) {
      if (cbor_isa_uint(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_16) {
        item.setValue(static_cast<I>(cbor_get_uint16(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 4) {
      if (cbor_isa_uint(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_32) {
        item.setValue(static_cast<I>(cbor_get_uint32(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 8) {
      if (cbor_isa_uint(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_64) {
        item.setValue(static_cast<I>(cbor_get_uint64(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 16) {
      if (cbor_isa_array(item.ptr()) && cbor_array_size(item.ptr()) == 2) {
        CborItem<u64> hi(cbor_array_get(item.ptr(), 0));
        CborProducerImpl<DataType::Integer, u64>().produce(hi, state);

        CborItem<u64> lo(cbor_array_get(item.ptr(), 1));
        CborProducerImpl<DataType::Integer, u64>().produce(lo, state);

        if (cbor_isa_uint(hi.ptr()) && cbor_int_get_width(hi.ptr()) == CBOR_INT_64 &&
            cbor_isa_uint(lo.ptr()) && cbor_int_get_width(lo.ptr()) == CBOR_INT_64) {
          u128 val128 = hi.value();
          val128 <<= 64;
          val128 |= lo.value();
          item.setValue(static_cast<I>(val128));
          return;
        }
      }
    } else {
      ROCKET_TERMINATE("Cannot produce integer of type `{}`", typeid(I));
    }

    throw InputFailure(state.index, "Expected an integer value");
  }
};

template<typename F>
struct CborProducerImpl<DataType::Float, F> {
  void
  produce(CborItem<F>& item, DecoderState& state) const {
    state.nextItem();

    if constexpr (sizeof(F) == 4) {
      if (cbor_is_float(item.ptr()) && cbor_float_get_width(item.ptr()) == CBOR_FLOAT_32) {
        item.setValue(static_cast<F>(cbor_float_get_float4(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(F) == 8) {
      if (cbor_is_float(item.ptr()) && cbor_float_get_width(item.ptr()) == CBOR_FLOAT_64) {
        item.setValue(static_cast<F>(cbor_float_get_float8(item.ptr())));
        return;
      }
    } else {
      ROCKET_TERMINATE("Cannot produce floating-point value of type `{}`", typeid(F));
    }

    throw InputFailure(state.index, "Expected a floating-point value");
  }
};

template<typename P>
struct CborProducerImpl<DataType::Pointer, P> {
  using Alt = Int<sizeof(P)>::Type;
  static constexpr auto AltDataType = DataTypes<Alt>::Value;

  void
  produce(CborItem<P>& item, DecoderState& state) const {
    CborItem<Alt> alt(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<AltDataType, Alt>().produce(alt, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a pointer");
    }
    item.setValue(reinterpret_cast<P>(alt.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::String, T> {
  using C = T::value_type;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (cbor_isa_string(item.ptr())) {
      const unsigned char* data = cbor_string_handle(item.ptr());
      const u64 size = cbor_string_length(item.ptr());
      // `utf8` may be a string view here
      const std::string_view utf8(reinterpret_cast<const char*>(data), size);
      std::basic_string<C> str(unicode::ConvertTo<C>::apply(utf8));
      if constexpr (std::same_as<T, std::basic_string_view<C>>) {
        // Decoding a string view: make it point to a string bound to the decoder
        const auto& ref = internal::DecoderStore::get()->store(std::move(str));
        item.setValue(ref);
      } else {
        // `std::basic_string`
        item.setValue(std::move(str));
      }
      return;
    }

    throw InputFailure(state.index, "Expected a string");
  }
};

template<typename T>
struct CborProducerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    if (cbor_is_null(item.ptr())) {
      state.nextItem();
      item.setValue(std::nullopt);
      return;
    }

    CborItem<Elem> elem(cbor_incref(item.ptr()));
    CborProducerImpl<ElemDataType, Elem>().produce(elem, state);
    item.setValue(std::move(elem.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::Tuple, T> {
  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_isa_array(item.ptr()) || cbor_array_size(item.ptr()) != std::tuple_size_v<T>) {
      throw InputFailure(state.index, fmt::format("Expected a tuple of size {}", std::tuple_size_v<T>));
    }

    T tuple;
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (produceElem(std::forward<decltype(arg)>(arg), item, state, index++), ...);
    }, tuple);
    item.setValue(std::move(tuple));
  }

private:

  template<typename Elem>
  void
  produceElem(Elem& elem, CborItem<T>& arr, DecoderState& state, u64 index) const {
    CborItem<Elem> elemItem(cbor_array_get(arr.ptr(), index));
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    CborProducerImpl<ElemDataType, Elem>().produce(elemItem, state);
    elem = std::move(elemItem.value());
  }
};

template<typename T>
struct CborProducerImpl<DataType::List, T> {
  static_assert(not IsView<T>, "Cannot decode list view");
  static_assert(not IsForwardList<T>, "Cannot decode forward list");

  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_isa_array(item.ptr())) {
      throw InputFailure(state.index, "Expected a list");
    }

    const u64 size = cbor_array_size(item.ptr());
    if constexpr (IsArray<T>) {
      if (size != std::tuple_size_v<T>) {
        throw InputFailure(state.index, fmt::format("Expected a list of size {}", std::tuple_size_v<T>));
      }

      // Fixed-size array
      T arr;
      for (u64 index = 0; index < size; ++index) {
        arr[index] = produceElem(item, state, index);
      }
      item.setValue(std::move(arr));
    } else {
      // Container with `push_back`
      T list;
      for (u64 index = 0; index < size; ++index) {
        list.push_back(produceElem(item, state, index));
      }
      item.setValue(std::move(list));
    }
  }

private:

  [[nodiscard]] Elem
  produceElem(CborItem<T>& arr, DecoderState& state, u64 index) const {
    CborItem<Elem> elem(cbor_array_get(arr.ptr(), index));
    CborProducerImpl<ElemDataType, Elem>().produce(elem, state);
    return elem.value();
  }
};

template<typename T>
struct CborProducerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_isa_array(item.ptr())) {
      throw InputFailure(state.index, "Expected a set");
    }

    const u64 size = cbor_array_size(item.ptr());
    T set;
    for (u64 index = 0; index < size; ++index) {
      set.insert(produceElem(item, state, index));
    }
    item.setValue(std::move(set));
  }

private:

  [[nodiscard]] Elem
  produceElem(CborItem<T>& arr, DecoderState& state, u64 index) const {
    CborItem<Elem> elem(cbor_array_get(arr.ptr(), index));
    CborProducerImpl<ElemDataType, Elem>().produce(elem, state);
    return elem.value();
  }
};

template<typename T>
struct CborProducerImpl<DataType::Map, T> {
  using Key = T::key_type;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_isa_map(item.ptr())) {
      throw InputFailure(state.index, "Expected a map");
    }

    const u64 size = cbor_map_size(item.ptr());
    T map;
    for (u64 index = 0; index < size; ++index) {
      map.insert(produceElem(item, state, index));
    }
    item.setValue(std::move(map));
  }

private:

  [[nodiscard]] std::pair<Key, Elem>
  produceElem(CborItem<T>& map, DecoderState& state, u64 index) const {
    const cbor_pair& pair = cbor_map_handle(map.ptr())[index];
    CborItem<Key> keyItem(cbor_incref(pair.key));
    CborProducerImpl<KeyDataType, Key>().produce(keyItem, state);
    CborItem<Elem> elemItem(cbor_incref(pair.value));
    CborProducerImpl<ElemDataType, Elem>().produce(elemItem, state);
    return { keyItem.value(), elemItem.value() };
  }
};

template<typename T>
struct CborProducerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  static constexpr auto KeyDataType = DataTypes<Key>::Value;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_isa_map(item.ptr())) {
      throw InputFailure(state.index, "Expected a bimap");
    }

    const u64 size = cbor_map_size(item.ptr());
    T map;
    for (u64 index = 0; index < size; ++index) {
      auto [key, elem] = produceElem(item, state, index);
      map.left.insert({ key, elem });
    }
    item.setValue(std::move(map));
  }

private:

  [[nodiscard]] std::pair<Key, Elem>
  produceElem(CborItem<T>& map, DecoderState& state, u64 index) const {
    const cbor_pair& pair = cbor_map_handle(map.ptr())[index];
    CborItem<Key> keyItem(cbor_incref(pair.key));
    CborProducerImpl<KeyDataType, Key>().produce(keyItem, state);
    CborItem<Elem> elemItem(cbor_incref(pair.value));
    CborProducerImpl<ElemDataType, Elem>().produce(elemItem, state);
    return { keyItem.value(), elemItem.value() };
  }
};

template<typename T>
struct CborProducerImpl<DataType::Duration, T> {
  using Rep = T::rep;
  static constexpr auto RepDataType = DataTypes<Rep>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    CborItem<Rep> count(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<RepDataType, Rep>().produce(count, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a duration");
    }
    item.setValue(T(count.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::Date, T> {
  using Rep = std::chrono::sys_days::duration::rep;
  static constexpr auto RepDataType = DataTypes<Rep>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    using namespace std::chrono;

    CborItem<Rep> count(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<RepDataType, Rep>().produce(count, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a date");
    }
    item.setValue(T(sys_days(days(count.value()))));
  }
};

template<typename T>
struct CborProducerImpl<DataType::HourMinuteSecond, T> {
  using Precision = T::precision;
  static constexpr auto PrecisionDataType = DataTypes<Precision>::Value;
  static_assert(PrecisionDataType == DataType::Duration);

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    CborItem<Precision> duration(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<PrecisionDataType, Precision>().produce(duration, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected an hour, minute, and second");
    }
    item.setValue(T(duration.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::TimeZone, T> {
  void
  produce(CborItem<T>& item, DecoderState& state) const {
    using namespace std::chrono;

    CborItem<std::string> name(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<DataType::String, std::string>().produce(name, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a time zone");
    }
    try {
      item.setValue(locate_zone(name.value()));
    } catch (const std::exception& ex) {
      throw InputFailure(state.index, ex.what());
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "Clock must be `system_clock`");
  using Duration = T::duration;
  static constexpr auto DurationDataType = DataTypes<Duration>::Value;
  static_assert(DurationDataType == DataType::Duration);

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    CborItem<Duration> epoch(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<DurationDataType, Duration>().produce(epoch, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a time point");
    }
    item.setValue(T(epoch.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::ZonedTime, T> {
  static constexpr auto TimeZoneDataType = DataTypes<TimeZone>::Value;
  static_assert(TimeZoneDataType == DataType::TimeZone);
  using Duration = T::duration;
  using SysTime = std::chrono::sys_time<Duration>;
  static constexpr auto SysTimeDataType = DataTypes<SysTime>::Value;
  static_assert(SysTimeDataType == DataType::Time);

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    state.nextItem();

    if (not cbor_isa_array(item.ptr()) || cbor_array_size(item.ptr()) != 2) {
      throw InputFailure(state.index, "Expected a zoned time");
    }

    CborItem<TimeZone> zone(cbor_array_get(item.ptr(), 0));
    CborProducerImpl<TimeZoneDataType, TimeZone>().produce(zone, state);
    CborItem<SysTime> time(cbor_array_get(item.ptr(), 1));
    CborProducerImpl<SysTimeDataType, SysTime>().produce(time, state);
    item.setValue(T(zone.value(), time.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    if (cbor_is_null(item.ptr())) {
      state.nextItem();
      item.setValue(T());
      return;
    }

    state.nextItem();
    if (not cbor_isa_array(item.ptr()) || cbor_array_size(item.ptr()) != 2) {
      throw InputFailure(state.index, "Expected an interval");
    }

    CborItem<A> a(cbor_array_get(item.ptr(), 0));
    CborProducerImpl<ADataType, A>().produce(a, state);
    CborItem<B> b(cbor_array_get(item.ptr(), 1));
    CborProducerImpl<BDataType, B>().produce(b, state);
    item.setValue(T(a.value(), b.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    T val {};
    produceObject<Elem>(refs, item, state, val);
    item.setValue(std::move(val));
  }
};

template<typename T>
struct CborProducerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    T val {};
    produceObject<Elem>(refs, item, state, val.get());
    item.setValue(std::move(val));
  }
};

template<typename T>
struct CborProducerImpl<DataType::MemberRef, T> {
  static_assert(false, "Cannot decode member reference");
};

template<typename T>
struct CborProducerImpl<DataType::VarRef, T> {
  static_assert(false, "Cannot decode variable reference");
};

template<typename T>
struct CborProducerImpl<DataType::CodePoint, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    CborItem<Elem> elem(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<ElemDataType, Elem>().produce(elem, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a code point");
    }
    item.setValue(T(elem.value()));
  }
};

template<typename T>
struct CborProducerImpl<DataType::Character, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(CborItem<T>& item, DecoderState& state) const {
    CborItem<Elem> elem(cbor_incref(item.ptr()));
    try {
      CborProducerImpl<ElemDataType, Elem>().produce(elem, state);
    } catch (const InputFailure& ex) {
      throw InputFailure(ex.position(), "Expected a character");
    }
    item.setValue(std::move(T(elem.value())));
  }
};

} // namespace internal::cbor

// `CborConsumer` -------------------------------------------------------------------------------------------

/// The consumer for #rocket::codec::CborCodec.
struct CborConsumer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::cbor::CborConsumerImpl<DataType, T>;
};

// `CborProducer` -------------------------------------------------------------------------------------------

/// The producer for #rocket::codec::CborCodec.
struct CborProducer {
  /// @type_alias
  template<DataType DataType, typename T>
  using Type = internal::cbor::CborProducerImpl<DataType, T>;
};

// `CborCodec` ----------------------------------------------------------------------------------------------

/**
 * A CBOR codec.
 *
 * The encoder can serialize an arbitrary C++ data structure to a CBOR byte sequence.
 *
 * The decoder can deserialize a CBOR byte sequence to an arbitrary C++ data structure.
 *
 * Decoding to list views and forward lists is not supported. String views and character views, however, are
 * allowed. This is made possible by keeping real strings in a store bound to the decoder. Hence, decoded
 * string views and character views are valid, and valid only, for the lifetime of the decoder.
 */
struct CborCodec : Codec<CborConsumer, CborProducer> {
  using Base = Codec<CborConsumer, CborProducer>; ///< @type_base

  /**
   * Encodes a value.
   *
   * @tparam T the type to encode
   * @param val the value to encode
   * @return a byte sequence, a binary representation of the value @p val
   */
  template<typename T>
  [[nodiscard]] std::vector<u8>
  encode(const T& val) const {
    using namespace internal::cbor;

    // Delegate to the consumer
    CborItem<T> item = Base::encode<T, CborItem<T>>(val);

    // Now serialize the item and create a byte vector for it
    unsigned char* buf; // NOLINT
    u64 bufSize; // NOLINT
    cbor_serialize_alloc(item.ptr(), &buf, &bufSize);
    ROCKET_EXPECT(bufSize > 0, "Failed to allocate memory");
    ROCKET_GUARD([&buf] { free(buf); }); // NOLINT
    return { buf, buf + bufSize };
  }

  /**
   * Decodes a value from a source.
   *
   * @tparam T the type to decode
   * @param cbor the CBOR input byte sequence to decode
   * @return the decoded value
   */
  template<typename T>
  [[nodiscard]] DecodeResult<T>
  decode(std::span<const u8> cbor) const {
    using namespace internal::cbor;

    // Load the item, this must consume the entire input
    cbor_load_result loadResult; // NOLINT
    cbor_item_t* item = cbor_load(cbor.data(), cbor.size(), &loadResult);
    if (loadResult.error.code != CBOR_ERR_NONE) {
      ROCKET_ASSERT(item == nullptr);
      return std::unexpected(DecodeError {
        loadResult.error.position, {}, std::string(messageOf(loadResult.error.code))
      });
    }
    CborItem<T> val(item);
    if (loadResult.read != cbor.size()) {
      return std::unexpected(DecodeError { loadResult.read, {}, "Unconsumed input" });
    }

    // Delegate to the producer
    DecoderState state = { static_cast<u64>(-1) };
    DecodeResult<CborItem<T>> decodeResult = Base::decode<T, CborItem<T>>(val, state);
    if (not decodeResult) {
      const auto& err = decodeResult.error();
      // The position of the error from the base class is actually the index of the item. We need to convert
      // it to the position in the input
      return std::unexpected(DecodeError { positionOf(cbor, err.position), err.ranges, err.message });
    }
    return decodeResult.value().value();
  }
};

} // namespace rocket::codec

// EOF
