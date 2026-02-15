/*
 * codec.cc
 */

#include "rocket/codec/codec.h"

using namespace rocket;
using namespace std;

// Test the #rocket::IsArray concept for codec-relevant types
static_assert(IsArray<array<int, 1>>);
static_assert(not IsArray<span<int>>);
static_assert(not IsArray<vector<int>>);

// Test the #rocket::IsHashed concept for codec-relevant types
static_assert(not IsHashed<Bimap<int, int>>);
static_assert(IsHashed<UnorderedBimap<int, int>>);
static_assert(not IsHashed<map<int, int>>);
static_assert(not IsHashed<set<int>>);
static_assert(IsHashed<unordered_map<int, int>>);
static_assert(IsHashed<unordered_set<int>>);

// Test the #rocket::IsVector concept for codec-relevant types
static_assert(not IsVector<array<int, 1>>);
static_assert(not IsVector<span<int>>);
static_assert(IsVector<vector<int>>);

// Test the #rocket::IsView concept for codec-relevant types
static_assert(not IsView<array<int, 1>>);
static_assert(not IsView<basic_string<char>>);
static_assert(IsView<basic_string_view<char>>);
static_assert(IsView<span<int>>);
static_assert(not IsView<vector<int>>);

// EOF
