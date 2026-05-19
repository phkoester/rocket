/*
 * Message.cc
 */

#include "Message.h"

#include "rocket/assert.h"

#include <scn/scan.h>

using namespace std;

namespace rocket::comm {

// #Message -------------------------------------------------------------------------------------------------

Message::Message(u64 size) :
  payload_(size, ' ') {
  ROCKET_CHECK(size, size == 0 || size >= 2);

  if (size == 0) {
    return;
  }

  for (u64 i = 0; i < size; ++i) {
    payload_[i] = '0' + (i % 10);
  }
  payload_[0] = '[';
  payload_[size - 1] = ']';
}

string
Message::display() const {
  if (size() <= 32) {
    return payload_;
  }
  return payload_.substr(0, 8) + "..." + payload_.substr(size() - 8);
}

u64
Message::parseSize(std::string_view input) {
  auto result = scn::scan<u64, char>(input, "{:i}{}"); // NOLINT
  ROCKET_EXPECT(result, "Invalid size input: {:?}", input);
  const auto [size, unit] = result->values();
  switch (unit) {
    case 'b':
      return size;
    case 'k':
      return size * 1024;
    case 'm':
      return size * 1024 * 1024;
    case 'g':
      return size * 1024 * 1024 * 1024;
    default:
      ROCKET_FAIL("Invalid unit: {}", unit);
  }
}

} // namespace rocket::comm
