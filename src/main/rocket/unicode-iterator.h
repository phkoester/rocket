/**
 * @file unicode-iterator.h
 *
 * Iterators for Unicode code points and graphemes.
 */

#pragma once

#include "Type.h"
#include "assert.h"
#include "unicode.h"

namespace rocket::unicode {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

extern thread_local Grapheme gr;

std::string
iteratorAt(const Type& type, size_t pos, std::string_view msg);

template<typename It>
std::string
iteratorAt(const It& it, size_t pos, std::string_view msg) {
  return iteratorAt(Type::of(it), pos, msg);
}

template<typename It>
std::string
outOfBounds(const It& it, size_t pos) {
  return iteratorAt(it, pos, "is out of bounds");
}

} // namespace internal

// `CodePointIterator` --------------------------------------------------------------------------------------

template<typename C> requires Character<C>
struct CodePointIterator;

template<typename C> requires Character<C>
struct GraphemeIterator;

// `CodePointIterator<char>` ................................................................................

/**
 * A #rocket::unicode::CodePoint iterator for UTF-8 strings.
 *
 * This class conforms to the `std::contiguous_iterator` concept.
 *
 * @attention If this iterator is constructed with an initial position of 0 or copy-constructed from another
 * iterator with a known code-point position, it always keeps track of its current code-point position. In
 * this case, #codePointPosition immediately returns a precalculated value. Otherwise, calling
 * #codePointPosition requires scanning the input string up to the iterator's current position. After
 * scanning, the iterator has a known code-point position.
 */
template<>
struct CodePointIterator<char> {
  /// The character type.
  using CharType = char;

  /// A type needed for `std::contiguous_iterator`.
  using iterator_category = std::contiguous_iterator_tag;
  /// A type needed for `std::contiguous_iterator`.
  using difference_type = std::ptrdiff_t;
  /// A type needed for `std::contiguous_iterator`.
  using value_type = CodePoint;
  /// A type needed for `std::contiguous_iterator`.
  using pointer = value_type*;
  /// A type needed for `std::contiguous_iterator`.
  using reference = value_type&;

  /// @ctor_default
  // No need  to initialize `cpSize_` here because `end() == true`
  // cppcheck-suppress uninitMemberVar
  constexpr CodePointIterator() : size_(0), pos_(0), cpPos_(0) {}

  /**
   * @ctor
   *
   * @param input a string
   * @param position a position
   */
  explicit CodePointIterator(std::string_view input, size_t position = 0);

  /**
   * Returns the current code point as a UTF-8 string.
   *
   * @return a UTF-8 string
   */
  operator std::string_view() const;

  /**
   * @member_op_dereference
   *
   * @return a reference to the current code point
   */
  const CodePoint& operator*() const;

  /**
   * @member_op_arrow
   *
   * @return a pointer to the current code point
   */
  const CodePoint* operator->() const;

  /**
   * @member_op_subscript
   *
   * @return a reference to the code point at index @p index
   */
  const CodePoint& operator[](difference_type index) const;

  /// @member_op_eq
  inline bool
  operator==(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ == rhs.pos_;
  }

  /// @member_op_ne
  inline bool
  operator!=(const CodePointIterator& rhs) const {
    return not operator==(rhs);
  }

  /// @member_op_lt
  inline bool
  operator<(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ < rhs.pos_;
  }

  /// @member_op_le
  inline bool
  operator<=(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ <= rhs.pos_;
  }

  /// @member_op_gt
  inline bool
  operator>(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ > rhs.pos_;
  }

  /// @member_op_ge
  inline bool
  operator>=(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ >= rhs.pos_;
  }

  /// @member_op_inc
  CodePointIterator& operator++();

  /// @member_op_inc_post
  CodePointIterator operator++(int);

  /// @member_op_dec
  CodePointIterator& operator--();

  /// @member_op_dec_post
  CodePointIterator operator--(int);

  /// @member_op_add_assign
  CodePointIterator& operator+=(difference_type rhs);

  /// @member_op_sub_assign
  CodePointIterator& operator-=(difference_type rhs);

  /**
   * Returns `true` if this iterator points to the beginning of the input.
   *
   * @return `true` if this iterator points to the beginning of the input.
   */
  inline bool begin() const { return pos_ == 0; }

  /**
   * Returns the current byte offset in the input string.
   *
   * For `CodePointIterator<char>`, this is is the same as #position.
   *
   * @return the byte offset in the input string
   */
  inline size_t bytePosition() const { return pos_; }

  /**
   * Returns the current code-point position.
   *
   * @attention If this iterator was constructed with an initial position of 0 or copy-constructed from
   * another iterator with a known code-point position, it always keeps track of its current code-point
   * position. In this case, #codePointPosition immediately returns a precalculated value. Otherwise,
   * calling #codePointPosition requires scanning the input string up to the iterator's current position.
   * After scanning, the iterator has a known code-point position.
   *
   * @return the current code-point position
   */
  size_t codePointPosition() const;

  /**
   * Returns the size of the UTF-8 byte sequence of the current code point.
   *
   * @return the size of the UTF-8 byte sequence of the current code point in the range [1, 4]
   */
  uint8_t codePointSize() const;

  /**
   * Returns `true` if this iterator can decrement by @p n code-point positions, and if so, performs the
   * decrement operation.
   *
   * @param n the number of code-point positions to decrement
   * @return `true` if this iterator can decrement by @p n code-point positions. If this function returns
   *     `true`, then the decrement operation is performed, otherwise the iterator remains unchanged
   */
  bool decrement(difference_type n = 1);

  /**
   * Returns `true` if this iterator points to the end of the input.
   *
   * @return `true` if this iterator points to the end of the input.
   */
  inline bool end() const { return pos_ == size_; }

  /**
   * Returns `true` if this iterator points to a grapheme boundary.
   *
   * @return `true` if this iterator points to a grapheme boundary
   */
  bool graphemeBoundary() const;

  /**
   * Returns `true` if this iterator can increment by @p n code-point positions, and if so, performs the increment
   * operation.
   *
   * @param n the number of code-point positions to increment
   * @return `true` if this iterator can increment by @p n code-point positions. If this function returns
   *     `true`, then the increment operation is performed, otherwise the iterator remains unchanged
   */
  bool increment(difference_type n = 1);

  /**
   * Returns the input string this iterator was constructed with.
   *
   * @return the input string
   */
  std::string_view input() const { return input_; }

  /**
   * Returns the current `char` offset in the input string.
   *
   * @return the current `char` offset in the input string
   */
  inline size_t position() const { return pos_; }

  /**
   * Returns `true` if this iterator points to a word boundary.
   *
   * @return `true` if this iterator points to a word boundary
   */
  bool wordBoundary() const;

private:

  std::string_view input_; // The input string
  size_t size_; // The size of `input_`
  size_t pos_; // The current `char` offset in the string
  mutable size_t cpPos_; // Lazy value: the current code-point position
  /**
   * The byte size of the current code point.
   *
   * @attention If #end returns `true`, then this value is undefined and may not be used at all.
   */
  uint8_t cpSize_;

  void go(size_t newPos);

  friend struct GraphemeIterator<CharType>;

  friend CodePointIterator operator+(const CodePointIterator& lhs, difference_type rhs);

  friend CodePointIterator operator+(difference_type lhs, const CodePointIterator& rhs);

  friend CodePointIterator operator-(const CodePointIterator& lhs, difference_type rhs);

  friend difference_type operator-(const CodePointIterator& lhs, const CodePointIterator& rhs);
};

static_assert(std::contiguous_iterator<CodePointIterator<char>>);

// `CodePointIterator<char32_t>` ............................................................................

/**
 * A #rocket::unicode::CodePoint iterator for UTF-32 strings.
 *
 * This class conforms to the `std::contiguous_iterator` concept.
 */
template<>
struct CodePointIterator<char32_t> {
  /// The character type.
  using CharType = char32_t;

  /// A type needed for `std::contiguous_iterator`.
  using iterator_category = std::contiguous_iterator_tag;
  /// A type needed for `std::contiguous_iterator`.
  using difference_type = std::ptrdiff_t;
  /// A type needed for `std::contiguous_iterator`.
  using value_type = CodePoint;
  /// A type needed for `std::contiguous_iterator`.
  using pointer = value_type*;
  /// A type needed for `std::contiguous_iterator`.
  using reference = value_type&;

  /// @ctor_default
  inline CodePointIterator() : size_(0), pos_(0) {}

  /**
   * @ctor
   *
   * @param input a string
   * @param position a position
   */
  explicit CodePointIterator(std::u32string_view input, size_t position = 0);

  /**
   * Returns the current code point as a UTF-32 string.
   *
   * @return a UTF-32 string
   */
  operator std::u32string_view() const;

  /**
   * @member_op_dereference
   *
   * @return a reference to the current code point
   */
  const CodePoint& operator*() const;

  /**
   * @member_op_arrow
   *
   * @return a pointer to the current code point
   */
  const CodePoint* operator->() const;

  /**
   * @member_op_subscript
   *
   * @return a reference to the code point at index @p index
   */
  const CodePoint& operator[](difference_type index) const;

  /// @member_op_eq
  inline bool
  operator==(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ == rhs.pos_;
  }

  /// @member_op_ne
  inline bool
  operator!=(const CodePointIterator& rhs) const {
    return not operator==(rhs);
  }

  /// @member_op_lt
  inline bool
  operator<(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ < rhs.pos_;
  }

  /// @member_op_le
  inline bool
  operator<=(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ <= rhs.pos_;
  }

  /// @member_op_gt
  inline bool
  operator>(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ > rhs.pos_;
  }

  /// @member_op_ge
  inline bool
  operator>=(const CodePointIterator& rhs) const {
    return input_.data() == rhs.input_.data() && pos_ >= rhs.pos_;
  }

  /// @member_op_inc
  CodePointIterator& operator++();

  /// @member_op_inc_post
  CodePointIterator operator++(int);

  /// @member_op_dec
  CodePointIterator& operator--();

  /// @member_op_dec_post
  CodePointIterator operator--(int);

  /// @member_op_add_assign
  CodePointIterator& operator+=(difference_type rhs);

  /// @member_op_sub_assign
  CodePointIterator& operator-=(difference_type rhs);

  /**
   * Returns `true` if this iterator points to the beginning of the input.
   *
   * @return `true` if this iterator points to the beginning of the input.
   */
  inline bool begin() const { return pos_ == 0; }

  /**
   * Returns the current byte offset in the input string.
   *
   * For `CodePointIterator<char32_t>`, this is `4 * position()`.
   *
   * @return the byte offset in the input string
   */
  inline size_t bytePosition() const { return sizeof(CharType) * pos_; }

  /**
   * Returns the current code-point position.
   *
   * For `CodePointIterator<char32_t>`, this is the same as #position.
   *
   * @return the byte offset in the input string
   */
  inline size_t codePointPosition() const { return pos_; }

  /**
   * Returns `true` if this iterator can decrement by @p n code-point positions, and if so, performs the
   * decrement operation.
   *
   * @param n the number of code-point positions to decrement
   * @return `true` if this iterator can decrement by @p n code-point positions. If this function returns
   *     `true`, then the decrement operation is performed, otherwise the iterator remains unchanged
   */
  bool decrement(difference_type n = 1);

  /**
   * Returns `true` if this iterator points to the end of the input.
   *
   * @return `true` if this iterator points to the end of the input.
   */
  inline bool end() const { return pos_ == size_; }

  /**
   * Returns `true` if this iterator points to a grapheme boundary.
   *
   * @return `true` if this iterator points to a grapheme boundary
   */
  bool graphemeBoundary() const;

  /**
   * Returns `true` if this iterator can increment by @p n code-point positions, and if so, performs the increment
   * operation.
   *
   * @param n the number of code-point positions to increment
   * @return `true` if this iterator can increment by @p n code-point positions. If this function returns
   *     `true`, then the increment operation is performed, otherwise the iterator remains unchanged
   */
  bool increment(difference_type n = 1);

  /**
   * Returns the input string this iterator was constructed with.
   *
   * @return the input string
   */
  std::u32string_view input() const { return input_; }

  /**
   * Returns the current `char32_t` offset in the input string.
   *
   * @return the current `char32_t` offset in the input string
   */
  inline size_t position() const { return pos_; }

  /**
   * Returns `true` if this iterator points to a word boundary.
   *
   * @return `true` if this iterator points to a word boundary
   */
  bool wordBoundary() const;

private:

  std::u32string_view input_; // The input string
  size_t size_; // The size of `input_`
  size_t pos_; // The current `char32_t` offset in the string

  void go(size_t newPos);

  friend CodePointIterator operator+(const CodePointIterator& lhs, difference_type rhs);

  friend CodePointIterator operator+(difference_type lhs, const CodePointIterator& rhs);

  friend CodePointIterator operator-(const CodePointIterator& lhs, difference_type rhs);

  friend difference_type operator-(const CodePointIterator& lhs, const CodePointIterator& rhs);
};

static_assert(std::contiguous_iterator<CodePointIterator<char32_t>>);

// `GraphemeIterator` ---------------------------------------------------------------------------------------

/**
 * A #rocket::unicode::Grapheme iterator for UTF-8 and UTF-32 strings.
 *
 * This class conforms to the `std::contiguous_iterator` concept.
 *
 * @attention If this iterator is constructed with an initial position of 0 or copy-constructed from another
 * iterator with a known grapheme position, it always keeps track of its current grapheme position. In this
 * case, #graphemePosition immediately returns a precalculated value. Otherwise, calling #graphemePosition
 * requires scanning the input string up to the iterator's current position. After scanning, the iterator has
 * a known grapheme position.
 */
template<typename C> requires Character<C>
struct GraphemeIterator {
  /// The character type.
  using CharType = C;

  /// A type needed for `std::contiguous_iterator`.
  using iterator_category = std::contiguous_iterator_tag;
  /// A type needed for `std::contiguous_iterator`.
  using difference_type = std::ptrdiff_t;
  /// A type needed for `std::contiguous_iterator`.
  using value_type = Grapheme;
  /// A type needed for `std::contiguous_iterator`.
  using pointer = value_type*;
  /// A type needed for `std::contiguous_iterator`.
  using reference = value_type&;

  /// @ctor_default
  // No need  to initialize `grSize_` here because `end() == true`
  // cppcheck-suppress uninitMemberVar
  constexpr GraphemeIterator() : it_(input_, 0), grPos_(0)  {}

  /**
   * @ctor
   *
   * @param input a string
   * @param position a `CharType` offset
   */
  explicit GraphemeIterator(std::basic_string_view<C> input, size_t position = 0) :
      input_(input),
      it_(input_, position),
      grPos_(position == 0 ? 0 : NPOS) {
    go();
  }

  /**
   * @member_op_dereference
   *
   * @return a reference to the current grapheme
   */
  const Grapheme&
  operator*() const {
    ROCKET_EXPECT(not end(), internal::outOfBounds(*this, position()));
    CodePoints cps;
    cps.reserve(grSize_);
    copy(it_, it_ + grSize_, back_inserter(cps));
    internal::gr = Grapheme(cps);
    return internal::gr;
  }

  /**
   * @member_op_arrow
   *
   * @return a pointer to the current grapheme
   */
  const Grapheme*
  operator->() const {
    ROCKET_EXPECT(not end(), internal::outOfBounds(*this, position()));
    CodePoints cps;
    cps.reserve(grSize_);
    copy(it_, it_ + grSize_, back_inserter(cps));
    internal::gr = Grapheme(cps);
    return &internal::gr;
  }

  /**
   * @member_op_subscript
   *
   * @return a reference to the grapheme at index @p index
   */
  const Grapheme&
  operator[](difference_type index) const {
    auto it(*this);
    it += index;
    *it;
    return internal::gr;
  }

  /// @member_op_eq
  inline bool operator==(const GraphemeIterator& rhs) const { return it_ == rhs.it_; }

  /// @member_op_ne
  inline bool operator!=(const GraphemeIterator& rhs) const { return it_ != rhs.it_; }

  /// @member_op_lt
  inline bool operator<(const GraphemeIterator& rhs) const { return it_ < rhs.it_; }

  /// @member_op_le
  inline bool operator<=(const GraphemeIterator& rhs) const { return it_ <= rhs.it_; }

  /// @member_op_gt
  inline bool operator>(const GraphemeIterator& rhs) const { return it_ > rhs.it_; }
  
  /// @member_op_ge
  inline bool operator>=(const GraphemeIterator& rhs) const { return it_ >= rhs.it_; }

  /// @member_op_inc
  GraphemeIterator&
  operator++() {
    ROCKET_EXPECT(not end(), internal::iteratorAt(*this, position(), "cannot increment"));
    it_ += grSize_;
    go();
    if (grPos_ != NPOS)
      ++grPos_;
    return *this;
  }

  /// @member_op_inc_post
  GraphemeIterator
  operator++(int) {
    auto result(*this);
    operator++();
    return result;
  }

  /// @member_op_dec
  GraphemeIterator&
  operator--() {
    ROCKET_EXPECT(not begin(), internal::iteratorAt(*this, position(), "cannot decrement"));
    --it_;
    while (not it_.graphemeBoundary())
      --it_;
    go();
    if (grPos_ != NPOS)
      --grPos_;
    return *this;
  }

  /// @member_op_dec_post
  GraphemeIterator
    operator--(int) {
    auto result(*this);
    operator--();
    return result;
  }

  /// @member_op_add_assign
  GraphemeIterator&
  operator+=(difference_type rhs) {
    if (rhs < 0)
      return operator-=(-rhs);
    else if (rhs  > 0) {
      for (difference_type i = 0; i < rhs; ++i)
        operator++();
    }
    return *this;  
  }

  /// @member_op_sub_assign
  GraphemeIterator&
  operator-=(difference_type rhs) {
    if (rhs < 0)
      return operator+=(-rhs);
    else if (rhs > 0) {
      for (difference_type i = 0; i < rhs; ++i)
        operator--();
    }
    return *this;  
  }

  /**
   * Returns `true` if this iterator points to the beginning of the input.
   *
   * @return `true` if this iterator points to the beginning of the input.
   */
  inline bool begin() const { return it_.begin(); }

  /**
   * Returns `true` if this iterator points to the end of the input.
   *
   * @return `true` if this iterator points to the end of the input.
   */
  inline bool end() const { return it_.end(); }

  /**
   * Returns the current grapheme position.
   *
   * @attention If this iterator was constructed with an initial position of 0 or copy-constructed from
   * another iterator with a known grapheme position, it always keeps track of its current grapheme position.
   * In this case, #graphemePosition immediately returns a precalculated value. Otherwise, calling
   * #graphemePosition requires scanning the input string up to the iterator's current position. After
   * scanning, the iterator has a known grapheme position.
   *
   * @return the current grapheme position
   */
  size_t
  graphemePosition() const {
    if (grPos_ == NPOS) {
      auto it = GraphemeIterator<C>(input_);
      while (it.position() != position())
        ++it;
      if constexpr (std::is_same_v<CharType, char>) {
        // Copy known code-point position
        it_.cpPos_ = it.it_.cpPos_;
      }
      // Copy known grapheme position
      grPos_ = it.grPos_;
    }
    return grPos_;
  }

  /**
   * Returns the code-point size of the current grapheme.
   *
   * @return the code-point size of the current grapheme
   */
  size_t
  graphemeSize() const {
    ROCKET_EXPECT(not end(), internal::outOfBounds(*this, position()));
    return grSize_;
  }

  /**
   * Returns the input string this iterator was constructed with.
   *
   * @return the input string
   */
  inline std::basic_string_view<C> input() const { return input_; }

  /**
   * Returns the current `CharType` offset in the input string.
   *
   * @return the current `CharType` offset in the input string
   */
  inline size_t position() const { return it_.position(); }

  /**
   * Returns `true` if this iterator points to a word boundary.
   *
   * @return `true` if this iterator points to a word boundary
   */
  bool wordBoundary() const { return it_.wordBoundary(); }

private:

  std::basic_string_view<C> input_; // The input string
  CodePointIterator<C> it_; // The code-point iterator used to implement this grapheme iterator
  mutable size_t grPos_; // Lazy value: the current grapheme position
  /**
   * The code-point size of the current grapheme.
   *
   * @attention If #end returns `true`, then this value is undefined and may not be used at all.
   */
  size_t grSize_;

  void
  go() {
    // NOTE: `grPos_` may not be used inside this function

    if (end())
      return;
    
    // Check grapheme boundary
    ROCKET_EXPECT(it_.graphemeBoundary(), internal::iteratorAt(*this, position(), "does not point to a grapheme boundary"));
    
    // Update grapheme size
    it_.codePointPosition(); // Optimization: enforce known code-point position
    auto it(it_ + 1);
    while (not it.graphemeBoundary())
      ++it;
    grSize_ = it - it_;
  }

  friend GraphemeIterator
  operator+(const GraphemeIterator& lhs, difference_type rhs) {
    auto result(lhs);
    result += rhs;
    return result;
  }

  friend GraphemeIterator
  operator+(difference_type lhs, const GraphemeIterator& rhs) {
    auto result(rhs);
    result += lhs;
    return result;
  }

  friend GraphemeIterator
  operator-(const GraphemeIterator& lhs, difference_type rhs) {
    auto result(lhs);
    result -= rhs;
    return result;
  }

  friend difference_type
  operator-(const GraphemeIterator& lhs, const GraphemeIterator& rhs) {
    return lhs.graphemePosition() - rhs.graphemePosition();
  }
};

static_assert(std::contiguous_iterator<GraphemeIterator<char>>);
static_assert(std::contiguous_iterator<GraphemeIterator<char32_t>>);

} // namespace rocket::unicode

// EOF
