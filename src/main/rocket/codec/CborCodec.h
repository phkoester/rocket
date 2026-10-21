/**
 * @file CborCodec.h
 *
 * A CBOR codec, using libcbor.
 */

#pragma once

#include "rocket/Guard.h"
#include "rocket/codec/codec.h"

#include <cbor.h>

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
  CborItem() : ptr_(nullptr) {}

  CborItem(cbor_item_t* ptr) : ptr_(ptr) {
    ROCKET_CHECK(ptr, ptr != nullptr);
    ROCKET_ASSERT(cbor_refcount(ptr) > 0);
  }

  CborItem(cbor_item_t* ptr, T&& val) : ptr_(ptr), val_(std::move(val)) {
    ROCKET_CHECK(ptr, ptr != nullptr);
    ROCKET_ASSERT(cbor_refcount(ptr) > 0);
  }

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

  CborItem&
  operator=(CborItem&& rhs) noexcept {
    if (&rhs == this) {
      return *this;
    }
    if (ptr_ != nullptr) {
      ROCKET_ASSERT(cbor_refcount(ptr_) > 0);
      cbor_decref(&ptr_);
    }
    ptr_ = rhs.ptr_;
    rhs.ptr_ = nullptr;
    val_ = std::move(rhs.val_);
    rhs.val_ = std::nullopt;
    return *this;
  }

  /// This is a one-shot move operation.
  template<typename U>
  CborItem<U>
  cast() {
    cbor_item_t* ptr = ptr_;
    ptr_ = nullptr;
    if (val_.has_value()) {
      U converted = static_cast<U>(std::move(*val_));
      val_ = std::nullopt;
      return CborItem<U>(ptr, std::move(converted));
    }
    return CborItem<U>(ptr);
  }

  [[nodiscard]] cbor_item_t*
  ptr() {
    ROCKET_EXPECT(ptr_ != nullptr);
    return ptr_;
  }

  /// This is a one-shot move operation.
  [[nodiscard]] T
  value() {
    ROCKET_EXPECT(val_.has_value());
    T val = std::move(*val_);
    val_ = std::nullopt;
    return val;
  }

  /// This is a one-shot move operation.
  [[nodiscard]] CborItem<T>
  withValue(T&& val) {
    ROCKET_EXPECT(not val_);
    CborItem<T> ret(ptr_, std::move(val));
    ptr_ = nullptr;
    return ret;
  }

private:

  cbor_item_t* ptr_; ///< The managed CBOR item.
  /// The carried C++ value, if any. It can be massive, which is why we move so often in this implementation.
  std::optional<T> val_;
};

// Functions ------------------------------------------------------------------------------------------------

/// Throws a #rocket::InputFailure, always.
void loadFailed(nio::Source& in, const cbor_load_result& result);

template<typename I>
CborItem<I>
loadItem(nio::ContiguousSource& in) {
  auto buf = in.bytes();
  cbor_load_result result;
  cbor_item_t* item = cbor_load(buf.data(), buf.size(), &result);
  if (result.error.code != CBOR_ERR_NONE) {
    ROCKET_ASSERT(item == nullptr);
    loadFailed(in, result);
  }
  CborItem<I> ret(item);
  in.seek(result.read, nio::SeekMode::cur);
  return ret;
}

// `CborConsumerImpl` ---------------------------------------------------------------------------------------

template<DataType DataType, typename T>
struct CborConsumerImpl;

template<>
struct CborConsumerImpl<DataType::Bool, bool> {
  [[nodiscard]] CborItem<bool>
  consume(bool val) const {
    return cbor_build_bool(val);
  }
};

template<typename C>
struct CborConsumerImpl<DataType::Char, C> {
  [[nodiscard]] CborItem<C>
  consume(C val) const {
    if constexpr (std::same_as<C, char>) {
      return cbor_build_uint8(static_cast<u8>(val));
    } else {
      return cbor_build_uint32(static_cast<u32>(val));
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
    return underlying.template cast<E>();
  }
};

template<typename I>
struct CborConsumerImpl<DataType::Integer, I> {
  [[nodiscard]] CborItem<I>
  consume(I val) const {
    if constexpr (sizeof(I) == 1) {
      return cbor_build_uint8(static_cast<u8>(val));
    } else if constexpr (sizeof(I) == 2) {
      return cbor_build_uint16(static_cast<u16>(val));
    } else if constexpr (sizeof(I) == 4) {
      return cbor_build_uint32(static_cast<u32>(val));
    } else if constexpr (sizeof(I) == 8) {
      return cbor_build_uint64(static_cast<u64>(val));
    } else if constexpr (sizeof(I) == 16) {
      CborItem<I> ret(cbor_new_definite_array(2));
      CborItem<u64> hi = cbor_build_uint64(static_cast<u64>(val >> 64));
      ROCKET_EXPECT(cbor_array_push(ret.ptr(), hi.ptr()));
      CborItem<u64> lo = cbor_build_uint64(static_cast<u64>(val));
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
      return cbor_build_float4(static_cast<f32>(val));
    } else if constexpr (sizeof(F) == 8) {
      return cbor_build_float8(static_cast<f64>(val));
    } else {
      ROCKET_TERMINATE("Cannot consume float of type `{}`", typeid(F));
    }
  }
};

#if 0 // NOLINT

template<typename P>
struct CborConsumerImpl<DataType::Pointer, P> {
  void
  consume(P val, nio::Sink& out, CONFIG__) const {
    if (val == nullptr) {
      out.write("null");
      return;
    }
    out.print("\"{}\"", static_cast<const void*>(val));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::String, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const std::string utf8(unicode::ConvertTo<char>::apply(val));
    const std::string escaped = str::escape::escapeCString(utf8, { .json=true, .quote='"' });
    out.print("{}", escaped);
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (not val) {
      out.write("null");
      return;
    }
    CborConsumerImpl<ElemDataType, Elem>().consume(*val, out, config);
  }
};

// For `MemberRef`, the tuple consumer must be able to pass additional arguments to the element consumer
template<typename T>
struct CborConsumerImpl<DataType::Tuple, T> {
  template<typename... Args>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, Args&&... args) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (consumeElem(std::forward<decltype(arg)>(arg), out, config, index++, std::forward<Args>(args)...), ...);
    }, val);
    endContainer(out, config, std::tuple_size_v<T>, ']');
  }

private:

  template<typename Elem, typename... Args>
  void
  consumeElem(const Elem& elem, nio::Sink& out, CONFIG__, u64 index, Args&&... args) const {
    nextElem(out, config, index);
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    CborConsumerImpl<ElemDataType, Elem>().consume(elem, out, config, std::forward<Args>(args)...);
  }
};

template<typename T>
struct CborConsumerImpl<DataType::List, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      CborConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, index, ']');
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '[');
    u64 index = 0;
    for (const auto& elem : val) {
      nextElem(out, config, index++);
      CborConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), ']');
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Map, T> {
  using Key = T::key_type;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val) {
      nextElem(out, config, index++);
      writeJsonString(key, out);
      out.write(": ");
      CborConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    beginContainer(out, config, '{');
    u64 index = 0;
    for (const auto& [key, elem] : val.left) {
      nextElem(out, config, index++);
      writeJsonString(key, out);
      out.write(": ");
      CborConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
    }
    endContainer(out, config, val.size(), '}');
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Duration, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const { // Take by value
    if constexpr (std::same_as<T, std::chrono::microseconds>) {
      out.print("\"{}µs\"", val.count());
    } else if constexpr (std::same_as<T, std::chrono::weeks>) {
      out.print("\"{}w\"", val.count());
    } else if constexpr (std::same_as<T, std::chrono::months>) {
      out.print("\"{}m\"", val.count());
    } else if constexpr (std::same_as<T, std::chrono::years>) {
      out.print("\"{}y\"", val.count());
    } else {
      out.print("\"{}\"", val);
    }
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Date, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(std::format("\"{}\"", val));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::HourMinuteSecond, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(std::format("\"{}\"", val));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::TimeZone, T> {
  void
  consume(T val, nio::Sink& out, CONFIG__) const {
    out.print("\"{}\"", val->name());
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "Clock must be `system_clock`");

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.write(std::format("\"{:%FT%TZ}\"", val)); // Zulu time
  }
};

template<typename T>
struct CborConsumerImpl<DataType::ZonedTime, T> {
  static constexpr auto TimeZoneDataType = DataTypes<TimeZone>::Value;
  static_assert(TimeZoneDataType == DataType::TimeZone);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto* tz = val.get_time_zone();

    const auto info = val.get_info();
    if (info.offset == std::chrono::seconds(0)) {
      out.write(std::format("\"{:%FT%TZ} ({})\"", val, tz->name())); // Zulu time
    } else {
      out.write(std::format("\"{:%FT%T%Ez} ({})\"", val, tz->name())); // Time with UTC offset
    }
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    if (val.empty()) {
      out.write("null");
      return;
    }

    out.write('[');
    const auto optA = optionalOf(val.a);
    if (not optA) {
      out.write("null");
    } else {
      CborConsumerImpl<ADataType, A>().consume(val.a, out, config);
    }
    out.write(", ");
    const auto optB = optionalOf(val.b);
    if (not optB) {
      out.write("null");
    } else {
      CborConsumerImpl<BDataType, B>().consume(val.b, out, config);
    }
    out.write(']');
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    CborConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val);
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    // Here we have to pass an additional argument, the instance, to the tuple consumer. The tuple consumer
    // will pass it on to the member-reference consumer
    CborConsumerImpl<ElemDataType, Elem>().consume(refs, out, config, val.get());
  }
};

template<typename T>
struct CborConsumerImpl<DataType::MemberRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  template<typename C>
  void
  consume(const T& val, nio::Sink& out, CONFIG__, const C& instance) const {
    out.print("\"{}\": ", val.name());
    CborConsumerImpl<ElemDataType, Elem>().consume(val.get(instance), out, config);
  }
};

template<typename T>
struct CborConsumerImpl<DataType::VarRef, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("\"{}\": ", val.name());
    CborConsumerImpl<ElemDataType, Elem>().consume(val.get(), out, config);
  }
};

template<typename T>
struct CborConsumerImpl<DataType::CodePoint, T> {
  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    out.print("\"U+{:0>4X}\"", static_cast<u32>(val));
  }
};

template<typename T>
struct CborConsumerImpl<DataType::Character, T> {
  using Elem = T::View;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  consume(const T& val, nio::Sink& out, CONFIG__) const {
    const auto elem = static_cast<Elem>(val);
    CborConsumerImpl<ElemDataType, Elem>().consume(elem, out, config);
  }
};

#endif

// `CborProducerImpl` ---------------------------------------------------------------------------------------

template<DataType DataType, typename T>
struct CborProducerImpl;

template<>
struct CborProducerImpl<DataType::Bool, bool> {
  void
  produce(CborItem<bool>& val, nio::ContiguousSource& in) const {
    const auto pos = in.tell();
    auto item = loadItem<bool>(in);
    if (not cbor_is_bool(item.ptr())) {
      throw InputFailure(pos, "Expected a boolean value");
    }
    val = item.withValue(cbor_get_bool(item.ptr()));
  }
};

template<typename C>
struct CborProducerImpl<DataType::Char, C> {
  void
  produce(CborItem<C>& val, nio::ContiguousSource& in) const {
    const auto pos = in.tell();
    auto item = loadItem<C>(in);
    if constexpr (std::same_as<C, char>) {
      if (cbor_is_int(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_8) {
        val = item.withValue(static_cast<char>(cbor_get_uint8(item.ptr())));
        return;
      }
    } else {
      if (cbor_is_int(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_32) {
        val = item.withValue(static_cast<char32>(cbor_get_uint32(item.ptr())));
        return;
      }
    }
    throw InputFailure(pos, "Expected a character");
  }
};

template<typename E>
struct CborProducerImpl<DataType::Enum, E> {
  using Underlying = std::underlying_type_t<E>;
  static constexpr auto UnderlyingDataType = DataTypes<Underlying>::Value;

  void
  produce(CborItem<E>& val, nio::ContiguousSource& in) const {
    CborItem<Underlying> underlying;
    CborProducerImpl<UnderlyingDataType, Underlying>().produce(underlying, in);
    val = underlying.template cast<E>();
  }
};

template<typename I>
struct CborProducerImpl<DataType::Integer, I> {
  void
  produce(CborItem<I>& val, nio::ContiguousSource& in) const {
    const auto pos = in.tell();
    auto item = loadItem<I>(in);
    if constexpr (sizeof(I) == 1) {
      if (cbor_is_int(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_8) {
        val = item.withValue(static_cast<I>(cbor_get_uint8(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 2) {
      if (cbor_is_int(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_16) {
        val = item.withValue(static_cast<I>(cbor_get_uint16(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 4) {
      if (cbor_is_int(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_32) {
        val = item.withValue(static_cast<I>(cbor_get_uint32(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 8) {
      if (cbor_is_int(item.ptr()) && cbor_int_get_width(item.ptr()) == CBOR_INT_64) {
        val = item.withValue(static_cast<I>(cbor_get_uint64(item.ptr())));
        return;
      }
    } else if constexpr (sizeof(I) == 16) {
      if (cbor_isa_array(item.ptr()) && cbor_array_size(item.ptr()) == 2) {
        CborItem<u64> hi = cbor_array_get(item.ptr(), 0);
        CborItem<u64> lo = cbor_array_get(item.ptr(), 1);
        if (cbor_is_int(hi.ptr()) && cbor_int_get_width(hi.ptr()) == CBOR_INT_64 &&
            cbor_is_int(lo.ptr()) && cbor_int_get_width(lo.ptr()) == CBOR_INT_64) {
          u128 val128 = cbor_get_uint64(hi.ptr());
          val128 <<= 64;
          val128 |= cbor_get_uint64(lo.ptr());
          val = item.withValue(static_cast<I>(val128));
          return;
        }
      }
    } else {
      ROCKET_TERMINATE("Cannot produce integer of type `{}`", typeid(I));
    }
    throw InputFailure(pos, "Expected an integer value");
  }
};

#if 0 // NOLINT

template<typename I>
struct CborProducerImpl<DataType::Integer, I> {
  void
  produce(I& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    const auto result = scanInteger<I>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected an integer value");
  }
};

template<typename F>
struct CborProducerImpl<DataType::Float, F> {
  using Limits = std::numeric_limits<F>;

  void
  produce(F& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "-Infinity")) {
      val = -Limits::infinity();
      return;
    }
    if (readChoice(in, { "Infinity", "+Infinity" })) {
      val = Limits::infinity();
      return;
    }
    if (readString(in, "NaN")) {
      val = Limits::quiet_NaN();
      return;
    }

    const auto result = scan<F>(in);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(pos, "Expected a floating-point value");
  }
};

template<typename P>
struct CborProducerImpl<DataType::Pointer, P> {
  void
  produce(P& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "null")) {
      val = nullptr;
      return;
    }

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    const auto result = scan<P>(inner);
    if (result) {
      val = *result;
      return;
    }
    throw InputFailure(inPos(0), "Expected a pointer value");
  }
};

template<typename T>
struct CborProducerImpl<DataType::String, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a string");
    }
    auto input = readUntilUnescapedChar(in, '"');
    if (not input) {
      throw InputFailure(pos, "Unterminated string literal");
    }

    const std::string unescaped = str::escape::unescapeCString(*input);
    using C = T::value_type;
    std::basic_string<C> str(unicode::ConvertTo<C>::apply(unescaped));
    if constexpr (std::same_as<T, std::basic_string_view<C>>) {
      // Because both unescaping and Unicode-converting produce intermediate strings local to this function,
      // decoding to `std::basic_string_view` is a bit more involved. To make this possible, we store the
      // intermediate string in the source so the decoded string view is valid for the lifetime of the source
      const auto& ref = in.store(std::move(str));
      val = ref;
    } else {
      // `std::basic_string`
      val = std::move(str);
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Optional, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);

    if (readString(in, "null")) {
      val = std::nullopt;
      return;
    }

    val = Elem();
    CborProducerImpl<ElemDataType, Elem>().produce(*val, in);
  }
};

template<typename T>
struct CborProducerImpl<DataType::Tuple, T> {
  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an array");
    }

    u64 index = 0;
    std::apply([&](auto&&... arg) {
      (produceElem(std::forward<decltype(arg)>(arg), in, index++), ...);
    }, val);

    skip(in);
    if (std::tuple_size_v<T> > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skip(in);
    }
    if (not readChar(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated array");
    }
  }

private:

  template<typename Elem>
  void
  produceElem(Elem& elem, nio::Source& in, u64 index) const {
    skip(in);
    if (index > 0) {
      expectComma(in);
      skip(in);
    }
    constexpr auto ElemDataType = DataTypes<Elem>::Value;
    CborProducerImpl<ElemDataType, Elem>().produce(elem, in);
  }
};

template<typename T>
struct CborProducerImpl<DataType::List, T> {
  static_assert(not IsView<T>, "Cannot decode list view");
  static_assert(not IsForwardList<T>, "Cannot decode forward list");

  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if constexpr (not IsArray<T>) {
      val.clear();
    }

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an array");
    }

    if constexpr (IsArray<T>) {
      // Fixed-size array
      produceArray(val, in, pos);
    } else {
      // Container with `push_back`
      produceContainerWithPushBack(val, in);
    }
  }

private:

  void
  produceArray(T& val, nio::Source& in, u64 pos) const {
    const auto size = val.size();
    for (u64 index = 0; index < size; ++index) {
      skip(in);
      if (index > 0) {
        expectComma(in);
        skip(in);
      }
      CborProducerImpl<ElemDataType, Elem>().produce(val[index], in);
    }

    skip(in);
    if (size > 0 && readChar(in, ',')) { // Allow trailing comma if nonempty
      skip(in);
    }
    if (not readChar(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, fmt::format("Unterminated array of size {}", size));
    }
  }

  void
  produceContainerWithPushBack(T& val, nio::Source& in) const {
    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, ']')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, ']')) { // Allow trailing comma if nonempty
          return;
        }
      }
      val.push_back(Elem());
      CborProducerImpl<ElemDataType, Elem>().produce(val.back(), in);
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Set, T> {
  using Elem = T::value_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    val.clear();
    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an array");
    }

    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, ']')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, ']')) { // Allow trailing comma if nonempty
          return;
        }
      }
      Elem elem;
      CborProducerImpl<ElemDataType, Elem>().produce(elem, in);
      val.insert(std::move(elem));
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Map, T> {
  using Key = T::key_type;
  using Elem = T::mapped_type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "Expected an object");
    }

    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }

      Key key;
      readJsonKey(key, in);
      skip(in);

      expectColon(in);
      skip(in);

      Elem elem;
      CborProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win
      val.insert_or_assign(std::move(key), std::move(elem));
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Bimap, T> {
  using Key = Purge<typename T::left_value_type::first_type>;
  using Elem = Purge<typename T::left_value_type::second_type>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    val.clear();

    if (not readChar(in, '{')) {
      throw InputFailure(pos, "Expected an object");
    }

    u64 index = 0;
    while (true) {
      skip(in);
      if (readChar(in, '}')) {
        return;
      }
      if (index++ > 0) {
        expectComma(in);
        skip(in);
        if (readChar(in, '}')) { // Allow trailing comma if nonempty
          return;
        }
      }

      Key key;
      readJsonKey(key, in);
      skip(in);

      expectColon(in);
      skip(in);

      Elem elem;
      CborProducerImpl<ElemDataType, Elem>().produce(elem, in);
      // Make the last one win
      insertOrReplace(val, key, elem);
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Duration, T> {
  using Rep = T::rep;
  static constexpr auto RepDataType = DataTypes<Rep>::Value;

  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    Rep count;
    try {
      CborProducerImpl<RepDataType, Rep>().produce(count, inner);
    } catch (const InputFailure&) {
      throw InputFailure(inPos(0), "Expected a duration");
    }

    const std::set<std::string_view> UNITS = {
      "ns", "µs", "us", "ms", "s", "min", "h", "d", "w", "m", "y"
    };

    if (auto str = readChoice(inner, UNITS, true); str) {
      if (*str == "ns") {
        val = duration_cast<T>(nanoseconds(count));
      } else if (*str == "µs" || *str == "us") {
        val = duration_cast<T>(microseconds(count));
      } else if (*str == "ms") {
        val = duration_cast<T>(milliseconds(count));
      } else if (*str == "s") {
        val = duration_cast<T>(seconds(count));
      } else if (*str == "min") {
        val = duration_cast<T>(minutes(count));
      } else if (*str == "h") {
        val = duration_cast<T>(hours(count));
      } else if (*str == "d") {
        val = duration_cast<T>(days(count));
      } else if (*str == "w") {
        val = duration_cast<T>(weeks(count));
      } else if (*str == "m") {
        val = duration_cast<T>(months(count));
      } else if (*str == "y") {
        val = duration_cast<T>(years(count));
      } else {
        ROCKET_TERMINATE_UNREACHABLE_CODE();
      }
      return;
    }

    throw InputFailure(inPos(inner.tell()), "Expected a time unit");
  }
};

template<typename T>
struct CborProducerImpl<DataType::Date, T> {
  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    auto& is = inner.istream();
    auto result = scn::scan<std_int, std_unsigned, std_unsigned>(is, "{}-{}-{}");
    if (not result) {
      throw InputFailure(inPos(0), "Expected a date");
    }
    inner.seek(io::tellg(is), nio::SeekMode::beg);

    const auto& [y, m, d] = result->values();
    val = { year(y), month(m), day(d) };
  }
};

template<typename T>
struct CborProducerImpl<DataType::HourMinuteSecond, T> {
  using Precision = T::precision;

  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it
    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    // Read hour, minute, and second

    auto& is = inner.istream();
    auto result = scn::scan<hours::rep, minutes::rep, seconds::rep>(is, "{}:{}:{}");
    if (not result) {
      throw InputFailure(inPos(0), "Expected an hour, minute, and second");
    }
    inner.seek(io::tellg(is), nio::SeekMode::beg);

    auto [h, m, s] = result->values();
    bool neg = false;
    if (h < 0) {
      h = -h;
      neg = true;
    }

    try
    {
      // Read subseconds

      const nanoseconds subseconds = readSubseconds(inner);

      // Finally, construct `hh_mm_ss`

      Precision duration = duration_cast<Precision>(hours(h) + minutes(m) + seconds(s) + subseconds);
      if (neg) {
        duration = -duration;
      }
      val = T(duration);
    } catch (const InputFailure& ex) {
      throw InputFailure(inPos(ex.position()), ex.message());
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::TimeZone, T> {
  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string (no inner source needed here because we use `input` directly)
    Positions positions;
    const auto input = readJsonString(in, positions);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    try {
      // This throws if the time zone is not found
      val = locate_zone(input);
    } catch (const std::exception& ex) {
      throw InputFailure(inPos(0), ex.what());
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Time, T> {
  using Clock = T::clock;
  static_assert(std::same_as<Clock, std::chrono::system_clock>, "Clock must be `system_clock`");
  using Duration = T::duration;

  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it

    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    // The environment variable `TZ` interferes somehow ...

    const auto* current = current_zone();
    const auto TZ = system::env::get<std::string>("TZ");
    if (TZ) {
      ROCKET_EXPECT(*TZ == current->name(), "If defined, the environment variable `TZ` must match the current time zone, which is {}", current->name());
    }

    // Scan the time point, using scnlib

    auto& is = inner.istream();
    const auto result = scn::scan<T>(is, "{:%FT%T}");
    if (not result) {
      throw InputFailure(inPos(0), "Expected a time point");
    }
    inner.seek(io::tellg(is), nio::SeekMode::beg);
    val = result->value();

    try {
      // Read subseconds

      const nanoseconds subseconds = readSubseconds(inner);
      if (subseconds.count() > 0) {
        val += duration_cast<Duration>(subseconds);
      }

      // Read 'Z'

      expectChar(inner, 'Z');

      // Work around a bug in scnlib where the time point is not parsed as UTC but dependent from the current
      // time zone

      const auto info = current->get_info(val);
      val += info.offset;
    } catch (const InputFailure& ex) {
      throw InputFailure(inPos(ex.position()), ex.message());
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::ZonedTime, T> {
  using Duration = T::duration;
  static constexpr auto TimeZoneDataType = DataTypes<TimeZone>::Value;
  static_assert(TimeZoneDataType == DataType::TimeZone);

  void
  produce(T& val, nio::Source& in) const {
    using namespace std::chrono;

    skip(in);
    const auto pos = in.tell();

    // Read JSON string and create an inner source for it

    Positions positions;
    const auto input = readJsonString(in, positions);
    nio::StringSource inner(input);
    const auto inPos = [pos, &positions](u64 innnerPos) -> u64 {
      return jsonStringOffset(pos, innnerPos, positions);
    };

    // The environment variable `TZ` interferes somehow ...

    const auto* current = current_zone();
    const auto TZ = system::env::get<std::string>("TZ");
    if (TZ) {
      ROCKET_EXPECT(*TZ == current->name(), "If defined, the environment variable `TZ` must match the current time zone, which is {}", current->name());
    }

    // Scan the time point, using scnlib

    sys_time<Duration> tp;
    {
      auto& is = inner.istream();
      const auto result = scn::scan<sys_time<Duration>>(is, "{:%FT%T}");
      if (not result) {
        throw InputFailure(inPos(0), "Expected a time point");
      }
      inner.seek(io::tellg(is), nio::SeekMode::beg);
      tp = result->value();
    }

    try {
      // Read subseconds

      const nanoseconds subseconds = readSubseconds(inner);
      if (subseconds.count() > 0) {
        tp += duration_cast<Duration>(subseconds);
      }

      // Read "Z" or UTC offset

      const auto offsetPos = inner.tell();
      seconds offset;

      if (not readChar(inner, 'Z')) {
        auto sign = readChoice(inner, { "+", "-" }, false, false);
        if (not sign) {
          throw InputFailure(offsetPos, "Expected UTC offset");
        }

        auto& is = inner.istream();
        const auto result = scn::scan<std_unsigned, std_unsigned>(is, "{}:{}");
        if (not result) {
          throw InputFailure(offsetPos, "Expected UTC offset");
        }
        inner.seek(io::tellg(is), nio::SeekMode::beg);
        const auto& [h, m] = result->values();
        offset = hours(h) + minutes(m);
        if (*sign == "-") {
          offset = -offset;
        }
      }

      // Apply UTC offset; convert time point to UTC

      tp -= offset;

      // Work around a bug in scnlib where the time point is not parsed as UTC but dependent from the current
      // time zone

      const auto info = current->get_info(tp);
      tp += info.offset;

      // Read time zone. We need to do this by hand because the time-zone producer reads in a JSON string.
      // Here, however, the time zone is surrounded by parentheses

      expectChar(inner, ' ');
      const u64 tzPos = inner.tell();
      expectChar(inner, '(');
      const auto name = readUntilChar(inner, ')');
      if (not name) {
        throw InputFailure(tzPos, "Unterminated time-zone name");
      }
      TimeZone tz; // NOLINT
      try {
        // This throws if the time zone is not found
        tz = locate_zone(*name);
      } catch (const std::exception& ex) {
        throw InputFailure(tzPos, ex.what());
      }

      // Finally, construct the `zoned_time`

      val = T(tz, tp);
    } catch (const InputFailure& ex) {
      throw InputFailure(inPos(ex.position()), ex.message());
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Interval, T> {
  using A = T::A;
  static constexpr auto ADataType = DataTypes<A>::Value;
  using B = T::B;
  static constexpr auto BDataType = DataTypes<B>::Value;

  void
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (readString(in, "null")) {
      val = T();
      return;
    }

    if (not readChar(in, '[')) {
      throw InputFailure(pos, "Expected an interval");
    }
    skip(in);

    A a = A();
    if constexpr (IsOptional<A>) {
      if (not readString(in, "null")) {
        CborProducerImpl<ADataType, A>().produce(a, in);
      }
    } else {
      CborProducerImpl<ADataType, A>().produce(a, in);
    }
    val.a = a;

    skip(in);
    expectComma(in);
    skip(in);

    B b = B();
    if constexpr (IsOptional<B>) {
      if (not readString(in, "null")) {
        CborProducerImpl<BDataType, B>().produce(b, in);
      }
    } else {
      CborProducerImpl<BDataType, B>().produce(b, in);
    }
    val.b = b;

    skip(in);
    if (readChar(in, ',')) { // Allow trailing comma
      skip(in);
    }
    if (not readChar(in, ']')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated interval");
    }
  }
};

template<typename T>
struct CborProducerImpl<DataType::Declared, T> {
  static constexpr auto& refs = rocket::reflect::Declared<T>::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in) const {
    produceMembers(refs, val, in);
  }
};

template<typename T>
struct CborProducerImpl<DataType::Instance, T> {
  static constexpr auto& refs = T::InnerType::refs;
  using Elem = Purge<decltype(refs)>;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;
  static_assert(ElemDataType == DataType::Tuple);

  void
  produce(T& val, nio::Source& in) const {
    produceMembers(refs, val.get(), in);
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
  produce(T& val, nio::Source& in) const {
    skip(in);
    const auto pos = in.tell();

    if (not readChar(in, '"')) {
      throw InputFailure(pos, "Expected a code point");
    }

    if (not readString(in, "U+", false, false)) {
      // Read `char32`
      in.seek(-1, nio::SeekMode::cur);
      Elem elem = Elem();
      CborProducerImpl<ElemDataType, Elem>().produce(elem, in);
      val = T(elem);
      return;
    }

    // Read `U+...`
    in.seek(-2, nio::SeekMode::cur);
    auto result = scanCodePoint<u32>(in);
    if (not result) {
      throw InputFailure(pos, "Expected a code point");
    }
    if (not readChar(in, '"')) {
      throw InputFailure(in.tell(), { pos, in.tell() }, "Unterminated code point");
    }
    val = static_cast<Elem>(*result);
  }
};

template<typename T>
struct CborProducerImpl<DataType::Character, T> {
  using Elem = T::Type;
  static constexpr auto ElemDataType = DataTypes<Elem>::Value;

  void
  produce(T& val, nio::Source& in) const {
    Elem elem;
    CborProducerImpl<ElemDataType, Elem>().produce(elem, in);
    val = T(std::move(elem));
  }
};

#endif

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
 * XXX
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
    unsigned char* buf;
    u64 bufSize;
    cbor_serialize_alloc(item.ptr(), &buf, &bufSize);
    ROCKET_EXPECT(bufSize > 0, "Failed to allocate memory");
    ROCKET_GUARD([&buf] { free(buf); });
    // @improve Avoid copying and use the buffer directly. Ideally, we would return some sort of an immutable
    // view that calls `free` on destruction
    return std::vector<u8>(buf, buf + bufSize);
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

    // Create a source from the byte sequence
    nio::SpanSource in(cbor);

    // Delegate to the producer, and we're done
    DecodeResult<CborItem<T>> result = Base::decode<T, CborItem<T>>(in);
    if (not result) {
      return std::unexpected(result.error());
    }
    return result.value().value();
  }
};

} // namespace rocket::codec

// EOF
