/**
 * @file matcher.h
 *
 * A collection of GoogleTest matchers.
 */

#pragma once

#include "rocket/io/io.h"
#include "rocket/text/text.h"

#include <gmock/gmock.h>

#include <regex>

namespace rocket::gtest::matcher {

// Internal -------------------------------------------------------------------------------------------------

namespace internal {

struct Regex {
  const std::string pattern;
  const std::regex regex;

  explicit Regex(const std::string& pattern) :
      pattern(pattern),
      regex(pattern) {}
};

/**
 * Implements polymorphic matchers for #matchesRegex and #containsRegex, which can be used as a `Matcher<T>`
 * as long as @p T can be converted to `std::string`.
 */
struct MatchesRegexMatcher {
  MatchesRegexMatcher(std::shared_ptr<const Regex> regex, bool fullMatch) :
      regex_(regex),
      fullMatch_(fullMatch) {}

  void
  DescribeTo(std::ostream* os) const {
    *os << (fullMatch_ ? "matches" : "contains") << " regular expression ";
    testing::internal::UniversalPrinter<std::string>::Print(regex_->pattern, os);
  }

  void
  DescribeNegationTo(std::ostream* os) const {
    *os << "doesn't " << (fullMatch_ ? "match" : "contain") << " regular expression ";
    testing::internal::UniversalPrinter<std::string>::Print(regex_->pattern, os);
  }

#if GTEST_INTERNAL_HAS_STRING_VIEW
  bool
  MatchAndExplain(const testing::internal::StringView& s, testing::MatchResultListener* listener) const {
    return MatchAndExplain(std::string(s), listener);
  }
#endif // GTEST_INTERNAL_HAS_STRING_VIEW

  /**
   * Accepts pointer types, particularly:
   *
   * - `char*`
   * - `const char*`
   * - `wchar_t*`
   * - `const wchar_t*`
   *
   * @tparam C the character type
   */
  template<typename C>
  bool
  MatchAndExplain(C* s, testing::MatchResultListener* listener) const {
    return s != nullptr && MatchAndExplain(std::string(s), listener);
  }

  /**
   * Matches anything that can convert to `std::string`.
   *
   * This is a template, not just a plain function with `const std::string&`, because `absl::string_view` has
   * some interfering nonexplicit constructors.
   *
   * @tparam MatcheeStringType the matchee's string type
   */
  template<class MatcheeStringType>
  bool
  MatchAndExplain(const MatcheeStringType& s, testing::MatchResultListener*) const {
    const std::string s2(s);
    return fullMatch_ ? std::regex_match(s2, regex_->regex) : std::regex_search(s2, regex_->regex);
  }

private:

  const std::shared_ptr<const Regex> regex_;
  const bool fullMatch_;
};

} // namespace internal

// Functions ------------------------------------------------------------------------------------------------

/**
 * Matches a string that contains regular expression @p regex.
 *
 * The implementation is backed by `std::regex`.
 *
 * @param regex a pointer to an `internal::Regex`
 * @return a matcher
 */
inline testing::PolymorphicMatcher<internal::MatchesRegexMatcher>
containsRegex(std::shared_ptr<const internal::Regex> regex) {
  return testing::MakePolymorphicMatcher(internal::MatchesRegexMatcher(regex, false));
}

/**
 * Matches a string that contains regular expression @p pattern.
 *
 * The implementation is backed by `std::regex`.
 *
 * @tparam T the pattern's string type
 * @param pattern a regular expression
 * @return a matcher
 */
template <typename T = std::string>
inline testing::PolymorphicMatcher<internal::MatchesRegexMatcher>
containsRegex(const testing::internal::StringLike<T>& pattern) {
  return containsRegex(std::make_shared<const internal::Regex>(std::string(pattern)));
}

/**
 * Matches a string that fully matches regular expression @p regex.
 *
 * The implementation is backed by `std::regex`.
 *
 * @param regex a pointer to an `internal::Regex`
 * @return a matcher
 */
inline testing::PolymorphicMatcher<internal::MatchesRegexMatcher>
matchesRegex(std::shared_ptr<const internal::Regex> regex) {
  return testing::MakePolymorphicMatcher(internal::MatchesRegexMatcher(regex, true));
}

/**
 * Matches a string that fully matches regular expression @p pattern.
 *
 * The implementation is backed by `std::regex`.
 *
 * @tparam T the pattern's string type
 * @param pattern a regular expression
 * @return a matcher
 */
template<typename T = std::string>
inline testing::PolymorphicMatcher<internal::MatchesRegexMatcher>
matchesRegex(const testing::internal::StringLike<T>& pattern) {
  return matchesRegex(std::make_shared<const internal::Regex>(std::string(pattern)));
}

/**
 * Similar to `Throws`, but here, an arbitrary number of matchers can be passed to examine the caught
 * exception.
 *
 * @tparam Exception the exception type
 * @tparam Matchers the matchers' types
 * @param matchers the matchers to be combined with `AllOf`
 * @return a matcher
 */
template<typename Exception, typename... Matchers>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<Exception>>
throws(const Matchers&... matchers) {
  return testing::MakePolymorphicMatcher(
      testing::internal::ExceptionMatcherImpl<Exception>(AllOf(matchers...)));
}

// `rocket::io::InputFailure` ...............................................................................

/**
 * Matches a #rocket::io::InputFailure that matches @p positionMatcher and @p whatMatcher.
 *
 * @tparam C the character type
 * @tparam PositionMatcher the type of @p positionMatcher
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param positionMatcher a matcher for the `.position()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename PositionMatcher, typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::InputFailure>>
throwsInputFailure(PositionMatcher&& positionMatcher, WhatMatcher&& whatMatcher) {
  return throws<io::InputFailure>(
    testing::Property(
        ".position()",
        &io::InputFailure::position,
        std::forward<PositionMatcher>(positionMatcher)),
    testing::internal::WithWhat(MatcherCast<std::string>(std::forward<WhatMatcher>(whatMatcher))));
}

/**
 * Matches a #rocket::io::InputFailure that matches @p position and @p whatMatcher.
 *
 * @tparam C the character type
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param position the expected value of the `.position()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::InputFailure>>
throwsInputFailure(size_t position, WhatMatcher&& whatMatcher) {
  return throwsInputFailure(
      testing::Eq(position),
      std::forward<WhatMatcher>(whatMatcher));
}

// `rocket::io::ParseFailure` ...........................................................................

/**
 * Matches a #rocket::io::ParseFailure that matches @p positionMatcher and @p whatMatcher.
 *
 * @tparam C the character type
 * @tparam PositionMatcher the type of @p positionMatcher
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param positionMatcher a matcher for the `.position()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename PositionMatcher, typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::ParseFailure>>
throwsParseFailure(PositionMatcher&& positionMatcher, WhatMatcher&& whatMatcher) {
  return throws<io::ParseFailure>(
    testing::Property(
        ".position()",
        &io::ParseFailure::position,
        std::forward<PositionMatcher>(positionMatcher)),
    testing::Property(
        ".ranges()",
        &io::ParseFailure::ranges,
        testing::Eq(text::Ranges {})),
    testing::internal::WithWhat(MatcherCast<std::string>(std::forward<WhatMatcher>(whatMatcher))));
}

/**
 * Matches a #rocket::io::ParseFailure that matches @p position and @p whatMatcher.
 *
 * @tparam C the character type
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param position the expected value of the `.position()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::ParseFailure>>
throwsParseFailure(size_t position, WhatMatcher&& whatMatcher) {
  return throwsParseFailure(
      testing::Eq(position),
      std::forward<WhatMatcher>(whatMatcher));
}

/**
 * Matches a #rocket::io::ParseFailure that matches @p positionMatcher, @p rangesMatcher, and
 * @p whatMatcher.
 *
 * @tparam C the character type
 * @tparam PositionMatcher the type of @p positionMatcher
 * @tparam RangesMatcher the type of @p rangesMatcher
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param positionMatcher a matcher for the `.position()` property of the exception
 * @param rangesMatcher a matcher for the `.ranges()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename PositionMatcher, typename RangesMatcher, typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::ParseFailure>>
throwsParseFailure(
    PositionMatcher&& positionMatcher,
    RangesMatcher&& rangesMatcher,
    WhatMatcher&& whatMatcher) {
  return throws<io::ParseFailure>(
    testing::Property(
        ".position()",
        &io::ParseFailure::position,
        std::forward<PositionMatcher>(positionMatcher)),
    testing::Property(
        ".ranges()",
        &io::ParseFailure::ranges,
        std::forward<RangesMatcher>(rangesMatcher)),
    testing::internal::WithWhat(MatcherCast<std::string>(std::forward<WhatMatcher>(whatMatcher))));
}

/**
 * Matches a #rocket::io::ParseFailure that matches @p position and @p range.
 *
 * @tparam C the character type
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param position the expected value of the `.position()` property of the exception
 * @param range the expected one and only element in the `.ranges()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::ParseFailure>>
throwsParseFailure(size_t position, text::Range range, WhatMatcher&& whatMatcher) {
  return throwsParseFailure(
      testing::Eq(position),
      testing::Eq(text::Ranges { range }),
      std::forward<WhatMatcher>(whatMatcher));
}

/**
 * Matches a #rocket::io::ParseFailure that matches @p position and @p range.
 *
 * @tparam C the character type
 * @tparam WhatMatcher the type of @p whatMatcher
 * @param position the expected value of the `.position()` property of the exception
 * @param ranges the expected value of the `.ranges()` property of the exception
 * @param whatMatcher a matcher for the `.what()` property of the exception
 * @return a matcher
 */
template<typename WhatMatcher>
inline testing::PolymorphicMatcher<testing::internal::ExceptionMatcherImpl<io::ParseFailure>>
throwsParseFailure(size_t position, const text::Ranges& ranges, WhatMatcher&& whatMatcher) {
  return throwsParseFailure(
      testing::Eq(position),
      testing::Eq(ranges),
      std::forward<WhatMatcher>(whatMatcher));
}

} // namespace rocket::gtest::matcher

// EOF
