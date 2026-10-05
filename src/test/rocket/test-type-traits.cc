/*
 * test-type-traits.cc
 */

#include "rocket-test/rocket-test.h"

#include "rocket/type-traits.h"

// `TEST` ---------------------------------------------------------------------------------------------------

TEST(typeTraits, Ordering) {
  static_assert(std::same_as<Ordering<int>, std::strong_ordering>);
  static_assert(std::same_as<Ordering<std::tuple<int, float>>, std::partial_ordering>);
  static_assert(std::same_as<Ordering<std::tuple<int, int, string>>, std::strong_ordering>);

  static_assert(std::same_as<CommonOrdering<>, std::strong_ordering>);
  static_assert(std::same_as<CommonOrdering<int>, std::strong_ordering>);
  static_assert(std::same_as<CommonOrdering<int, float>, std::partial_ordering>);
  static_assert(std::same_as<CommonOrdering<int, int, string>, std::strong_ordering>);
}

TEST(typeTraits, Largest) {
  static_assert(std::same_as<Largest<i32, i64>, i64>);
  static_assert(std::same_as<Largest<i32, f64>, f64>);
  static_assert(std::same_as<Largest<bool, u16>, u16>);
}

TEST(typeTraits, View) {
  static_assert(std::same_as<View<i32>, i32>);
  static_assert(std::same_as<View<array<i32, 1>>, span<const i32>>);
  static_assert(std::same_as<View<basic_string<char>>, basic_string_view<char>>);
  static_assert(std::same_as<View<basic_string<char32>>, basic_string_view<char32>>);
  static_assert(std::same_as<View<basic_string_view<char>>, basic_string_view<char>>);
  static_assert(std::same_as<View<basic_string_view<char32>>, basic_string_view<char32>>);
  static_assert(std::same_as<View<span<i32>>, span<const i32>>);
  static_assert(std::same_as<View<span<const i32>>, span<const i32>>);
  static_assert(std::same_as<View<vector<i32>>, span<const i32>>);
}

TEST(typeTraits, Purge) {
  static_assert(std::same_as<Purge<const volatile i32>, i32>);
  static_assert(std::same_as<Purge<const std::true_type&>, std::true_type>);
}

TEST(typeTraits, IsArray) {
  static_assert(IsArray<array<i32, 1>>);
  static_assert(not IsArray<span<i32>>);
  static_assert(not IsArray<vector<i32>>);
}

TEST(typeTraits, IsForwardList) {
  static_assert(IsForwardList<forward_list<i32>>);
  static_assert(not IsForwardList<list<i32>>);
}

TEST(typeTraits, IsUnordered) {
  static_assert(not IsUnordered<Bimap<i32, i32>>);
  static_assert(IsUnordered<UnorderedBimap<i32, i32>>);
  static_assert(not IsUnordered<map<i32, i32>>);
  static_assert(not IsUnordered<set<i32>>);
  static_assert(IsUnordered<unordered_map<i32, i32>>);
  static_assert(IsUnordered<unordered_set<i32>>);
}

TEST(typeTraits, IsVector) {
  static_assert(not IsVector<array<i32, 1>>);
  static_assert(not IsVector<span<i32>>);
  static_assert(IsVector<vector<i32>>);
}

TEST(typeTraits, IsView) {
  static_assert(not IsView<array<i32, 1>>);
  static_assert(not IsView<basic_string<char>>);
  static_assert(not IsView<basic_string<char32>>);
  static_assert(IsView<basic_string_view<char>>);
  static_assert(IsView<basic_string_view<char32>>);
  static_assert(IsView<span<i32>>);
  static_assert(not IsView<vector<i32>>);
}

// EOF
