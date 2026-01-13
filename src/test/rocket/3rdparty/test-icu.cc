/*
 * test-icu.cc
 *
 * Tests related to the ICU library.
 */

#include "rocket-gtest/rocket-gtest.h"

#include "rocket/unicode/ConvertTo.h"
#include "rocket/unicode/Iterator.h"

using namespace rocket;
using namespace rocket::unicode;
using namespace std;
using namespace testing;

// Functions ------------------------------------------------------------------------------------------------

template<typename C> requires Character<C>
void
dumpSegments(Iterator<C>& it) {
  cout << "FORWARD " << Type::of<C>().name() << endl;
  cout << "-------\n";

  for( const auto& c : it.nextSegments()) {
    auto c8 = ConvertTo<char>().apply(c);
    cout << "Char: [" << c8 << "], current=" << it.current() << endl;
  }

  cout << "BACKWARD " << Type::of<C>().name() << endl;
  cout << "--------\n";

  it.last();
  for( const auto& c : it.previousSegments()) {
    auto c8 = ConvertTo<char>().apply(c);
    cout << "Char: [" << c8 << "], current=" << it.current() << endl;
  }
}

// TEST -----------------------------------------------------------------------------------------------------

TEST(icu, graphemeClusters) {
  Iterator<char> iter8(IteratorType::Char, "a🧑‍🌾bc");
  dumpSegments<char>(iter8);

  Iterator<char32_t> iter32(IteratorType::Char, U"a🧑‍🌾bc");
  dumpSegments<char32_t>(iter32);
}

// EOF
