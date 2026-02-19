/*
 * type-traits.cc
 */

#include "rocket/type-traits.h"

#include "rocket/Bimap.h"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

using namespace rocket;
using namespace std;

// #Ordering, #CommonOrdering -------------------------------------------------------------------------------

static_assert(std::is_same_v<Ordering<int>, std::strong_ordering>);
static_assert(std::is_same_v<Ordering<std::tuple<int, float>>, std::partial_ordering>);
static_assert(std::is_same_v<Ordering<std::tuple<int, int, string>>, std::strong_ordering>);

static_assert(std::is_same_v<CommonOrdering<>, std::strong_ordering>);
static_assert(std::is_same_v<CommonOrdering<int>, std::strong_ordering>);
static_assert(std::is_same_v<CommonOrdering<int, float>, std::partial_ordering>);
static_assert(std::is_same_v<CommonOrdering<int, int, string>, std::strong_ordering>);

// #IsArray -------------------------------------------------------------------------------------------------

static_assert(IsArray<array<i32, 1>>);
static_assert(not IsArray<span<i32>>);
static_assert(not IsArray<vector<i32>>);

// #IsUnordered ---------------------------------------------------------------------------------------------

static_assert(not IsUnordered<Bimap<i32, i32>>);
static_assert(IsUnordered<UnorderedBimap<i32, i32>>);
static_assert(not IsUnordered<map<i32, i32>>);
static_assert(not IsUnordered<set<i32>>);
static_assert(IsUnordered<unordered_map<i32, i32>>);
static_assert(IsUnordered<unordered_set<i32>>);

// #IsVector ------------------------------------------------------------------------------------------------

static_assert(not IsVector<array<i32, 1>>);
static_assert(not IsVector<span<i32>>);
static_assert(IsVector<vector<i32>>);

// #IsView --------------------------------------------------------------------------------------------------

static_assert(not IsView<array<i32, 1>>);
static_assert(not IsView<basic_string<char>>);
static_assert(not IsView<basic_string<char32>>);
static_assert(IsView<basic_string_view<char>>);
static_assert(IsView<basic_string_view<char32>>);
static_assert(IsView<span<i32>>);
static_assert(not IsView<vector<i32>>);

// #View ----------------------------------------------------------------------------------------------------

static_assert(std::is_same_v<View<i32>, i32>);
static_assert(std::is_same_v<View<array<i32, 1>>, span<const i32>>);
static_assert(std::is_same_v<View<basic_string<char>>, basic_string_view<char>>);
static_assert(std::is_same_v<View<basic_string<char32>>, basic_string_view<char32>>);
static_assert(std::is_same_v<View<basic_string_view<char>>, basic_string_view<char>>);
static_assert(std::is_same_v<View<basic_string_view<char32>>, basic_string_view<char32>>);
static_assert(std::is_same_v<View<span<i32>>, span<const i32>>);
static_assert(std::is_same_v<View<span<const i32>>, span<const i32>>);
static_assert(std::is_same_v<View<vector<i32>>, span<const i32>>);

// EOF
