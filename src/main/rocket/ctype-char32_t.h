/**
 * @file ctype-char32_t.h
 *
 * A `std::ctype<char32_t>` specialization that adds `char32_t` support to standard I/O streams.
 */

#pragma once

#include <locale>

namespace std {

// `ctype<char32_t>` ----------------------------------------------------------------------------------------

/**
 * This implementation is based on `ctype<wchar_t>`, found in `locale_facets.h`.
 */
template<>
struct ctype<char32_t> : public __ctype_abstract_base<char32_t>
{
  /// The character type.
  using char_type = char32_t;
  /// `__wmask_type`.
  using __wmask_type = wctype_t;

  /// The facet ID for `ctype<char32_t>`.
  static locale::id id;

  /**
   * Constructor performs initialization.
   *
   * This is the constructor provided by the standard.
   *
   * @param __refs passed to the base facet class
   */
  explicit ctype(size_t __refs = 0);

  /**
   * Constructor performs static initialization.
   *
   * This constructor is used to construct the initial C locale facet.
   *
   * @param __cloc handle to C locale data
   * @param __refs passed to the base facet class
   */
  explicit ctype(__c_locale __cloc, size_t __refs = 0);

  /// @dtor
  ~ctype() noexcept override;

protected:

  /// @cond undocumented

  __c_locale _M_c_locale_ctype;

  // Pre-computed narrowed and widened chars
  bool _M_narrow_ok;
  char _M_narrow[128];
  wint_t _M_widen[1 + static_cast<unsigned char>(-1)];

  // Pre-computed elements for do_is
  mask _M_bit[16];
  __wmask_type _M_wmask[16];

  __wmask_type _M_convert_to_wmask(const mask __m) const;

  // For use at construction time only
  void _M_initialize_ctype();

  /// @endcond

  /**
   * Tests `char32_t` classification.
   *
   * This function finds a mask M for @p __c and compares it to mask @p __m.
   *
   * #do_is is a hook for a derived facet to change the behavior of classifying. #do_is must always return
   * the same result for the same input.
   *
   * @param __c the char32_t to find the mask of
   * @param __m the mask to compare against
   * @return (M & __m) != 0
   */
  bool do_is(mask __m, char_type __c) const override;

  /**
   * Returns a mask array.
   *
   * This function finds the mask for each char32_t in the range [`__lo`,`__hi`) and successively writes it
   * to @p __vec. @p __vec must have as many elements as the input.
   *
   * #do_is is a hook for a derived facet to change the behavior of classifying. #do_is must always return
   * the same result for the same input.
   *
   * @param __lo pointer to start of range
   * @param __hi pointer to end of range
   * @param __vec pointer to an array of mask storage
   * @return @p __hi
   */
  const char_type* do_is(const char_type* __lo, const char_type* __hi, mask* __vec) const override;

  /**
   * Finds `char32_t` matching mask.
   *
   * This function searches for and returns the first `char32_t` `c` in [`__lo`,`__hi`) for which
   * `is(__m, c)` is `true`.
   *
   * #do_scan_is is a hook for a derived facet to change the behavior of match searching. #do_is must always
   * return the same result for the same input.
   *
   * @param __m the mask to compare against
   * @param __lo pointer to start of range
   * @param __hi pointer to end of range
   * @return pointer to a matching `char32_t` if found, else @p __hi.
   */
  const char_type* do_scan_is(mask __m, const char_type* __lo, const char_type* __hi) const override;

  /**
   * Finds `char32_t` not-matching mask.
   *
   * This function searches for and returns a pointer to the first `char32_t` `c` of [`__lo`,`__hi`) for
   * which `is(__m, c)` is `false`.
   *
   * #do_scan_is is a hook for a derived facet to change the behavior of match searching. #do_is must always
   * return the same result for the same input.
   *
   * @param __m the mask to compare against
   * @param __lo pointer to start of range
   * @param __hi pointer to end of range
   * @return pointer to a nonmatching `char32_t` if found, else @p __hi
   */
  const char_type* do_scan_not(mask __m, const char_type* __lo, const char_type* __hi) const override;

  /**
   * Converts to uppercase.
   *
   * This virtual function converts the `char32_t` argument to uppercase if possible. If not possible (for
   * example, <code>'2'</code>), returns the argument.
   *
   * #do_toupper is a hook for a derived facet to change the behavior of uppercasing. #do_toupper must always
   * return the same result for the same input.
   *
   * @param __c the `char32_t` to convert
   * @return the uppercase `char32_t` if convertible, else @p __c
   */
  char_type do_toupper(char_type __c) const override;

  /**
   * Converts array to uppercase.
   *
   * This virtual function converts each `char32_t` in the range [`__lo`,`__hi`) to uppercase if possible.
   * Other elements remain untouched.
   *
   * #do_toupper is a hook for a derived facet to change the behavior of uppercasing. #do_toupper must always
   ' return the same result for the same input.
   *
   * @param __lo pointer to start of range
   * @param __hi pointer to end of range
   * @return @p __hi
   */
  const char_type* do_toupper(char_type* __lo, const char_type* __hi) const override;

  /**
   * Converts to lowercase.
   *
   * This virtual function converts the argument to lowercase if possible. If not possible (for example,
   * <code>'2'</code>), returns the argument.
   *
   * #do_tolower is a hook for a derived facet to change the behavior of lowercasing. #do_tolower must always
   * return the same result for the same input.
   *
   * @param __c the `char32_t` to convert
   * @return the lowercase `char32_t` if convertible, else @p __c
   */
  char_type do_tolower(char_type __c) const override;

  /**
   * Converts array to lowercase.
   *
   * This virtual function converts each `char32_t` in the range [`__lo`,`__hi`) to lowercase if possible.
   * Other elements remain untouched.
   *
   * #do_tolower is a hook for a derived facet to change the behavior of lowercasing. #do_tolower must always
   * return the same result for the same input.
   *
   * @param __lo pointer to start of range
   * @param __hi pointer to end of range
   * @return @p __hi
   */
  const char_type* do_tolower(char_type* __lo, const char_type* __hi) const override;

  /**
   * Widens `char` to `char32_t`.
   *
   * This virtual function converts the `char` to `char32_t` using the simplest reasonable transformation.
   * For an underived `ctype<char32_t>` facet, the argument will be cast to `char32_t`.
   *
   * #do_widen is a hook for a derived facet to change the behavior of widening. #do_widen must always return
   * the same result for the same input.
   *
   * @note This is not what you want for code-page conversions. See `codecvt` for that.
   *
   * @param __c the char to convert
   * @return the converted `char32_t`
   */
  char_type do_widen(char __c) const override;

  /**
   * Widens `char` array to `char32_t` array.
   *
   * This function converts each `char` in the input to `char32_t` using the simplest reasonable
   * transformation. For an underived `ctype<char32_t>` facet, the argument will be copied, casting each
   * element to `char32_t`.
   *
   * #do_widen is a hook for a derived facet to change the behavior of widening. #do_widen must always return
   * the same result for the same input.
   *
   * @note This is not what you want for code-page conversions. See `codecvt` for that.
   *
   * @param __lo pointer to start range
   * @param __hi pointer to end of range
   * @param __to pointer to the destination array
   * @return @p __hi
   */
  const char* do_widen(const char* __lo, const char* __hi, char_type* __to) const override;

  /**
   * Narrows `char32_t` to `char`.
   *
   * This virtual function converts the argument to `char` using the simplest reasonable transformation. If
   * the conversion fails, @p __dfault is returned instead. For an underived `ctype<char32_t>` facet,
   * @p __c will be cast to `char` and returned.
   *
   * #do_narrow is a hook for a derived facet to change the behavior of narrowing. #do_narrow must always
   * return the same result for the same input.
   *
   * @note This is not what you want for code-page conversions. See `codecvt` for that.
   *
   * @param __c the char32_t to convert
   * @param __dfault `char` to return if conversion fails
   * @return the converted `char`
   */
  char do_narrow(char_type __c, char __dfault) const override;

  /**
   * Narrows `char32_t` array to `char` array.
   *
   * This virtual function converts each `char32_t` in the range [`__lo`,`__hi`) to `char` using the simplest
   * reasonable transformation and writes the results to the destination array. For any `char32_t` in the
   * input that cannot be converted, @p __dfault is used instead. For an underived `ctype<char32_t>` facet,
   * the argument will be copied, casting each element to `char`.
   *
   * #do_narrow is a hook for a derived facet to change the behavior of narrowing. #do_narrow must always
   * return the same result for the same input.
   *
   * @note This is not what you want for code-page conversions. See `codecvt` for that.
   *
   * @param __lo pointer to start of range
   * @param __hi pointer to end of range
   * @param __dfault `char` to use if conversion fails
   * @param __to pointer to the destination array
   * @return @p __hi
   */
  const char_type* do_narrow(
      const char_type* __lo, const char_type* __hi, char __dfault, char* __to) const override;
};

} // namespace std

// EOF
