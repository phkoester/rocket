/**
 * @file boost-spirit-x3.h
 *
 * Adds `char32_t` support to Boost Spirit X3.
 */

#include <boost/spirit/home/support/char_encoding/unicode.hpp>
#include <boost/spirit/home/x3/char/any_char.hpp>
#include <boost/spirit/home/x3/char/char_class.hpp>
#include <boost/spirit/home/x3/string/literal_string.hpp>

#pragma once

// Namespace `boost::spirit::x3::unicode` -------------------------------------------------------------------

namespace boost::spirit::x3 {

namespace unicode {
  /**
   * @name Character
   *
   * These declarations correspond to `boost/spirit/home/x3/char/char.hpp`.
   */
  ///@{

  /**
   * The character type.
   */
  using char_type = any_char<boost::spirit::char_encoding::unicode>;
  /**
   * The `char_` constant.
   */
  const auto char_ = char_type {};

  /**
   * `lit` overload.
   *
   * @param ch a character
   * @return a literal character
   */
  inline literal_char<boost::spirit::char_encoding::unicode, unused_type>
  lit(char32_t ch) {
    return { ch };
  }
  
  ///@}

  /**
   * @name String
   *
   * These declarations correspond to `boost/spirit/home/x3/string/literal_string.hpp`.
   */
  ///@{
        
  /**
   * `string` overload.
   *
   * @param s a string
   * @return a literal string
   */
  inline literal_string<const char32_t*, char_encoding::unicode>
  string(const char32_t* s) {
    return { s };
  }

  /**
   * `string` overload.
   *
   * @param s a string
   * @return a literal string
   */
  inline literal_string<std::basic_string<char32_t>, char_encoding::unicode>
  string(const std::basic_string<char32_t>& s) {
    return { s };
  }

  /**
   * `lit` overload.
   *
   * @param s a string
   * @return a literal string
   */
  inline literal_string<const char32_t*, char_encoding::unicode, unused_type>
  lit(const char32_t* s) {
    return { s };
  }

  /**
   * `lit` overload.
   *
   * @param s a string
   * @return a literal string
   */
  inline literal_string<std::basic_string<char32_t>, char_encoding::unicode, unused_type>
  lit(const std::basic_string<char32_t>& s) {
    return { s };
  }

  ///@}
}

/**
 * @name Character classes
 *
 * These declarations correspond to `boost/spirit/home/x3/char/char_class.hpp`.
 */
///@{

/// @cond undocumented

#define BOOST_SPIRIT_X3_CHAR_CLASS(encoding, name) \
    using name##_type = char_class<char_encoding::encoding, name##_tag>; \
    const name##_type name = name##_type()

#define BOOST_SPIRIT_X3_CHAR_CLASSES(encoding) \
    namespace encoding { \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, alnum); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, alpha); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, digit); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, xdigit); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, cntrl); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, graph); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, lower); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, print); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, punct); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, space); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, blank); \
      BOOST_SPIRIT_X3_CHAR_CLASS(encoding, upper); \
    }

/// @endcond

/**
 * Declares the Unicode character classes.
 */
BOOST_SPIRIT_X3_CHAR_CLASSES(unicode);

#undef BOOST_SPIRIT_X3_CHAR_CLASS
#undef BOOST_SPIRIT_X3_CHAR_CLASSES

///@}

} // namespace boost::spirit::x3

// EOF
