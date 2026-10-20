/*
 * CborCodec.cc
 */

#include "rocket/codec/CborCodec.h"

using namespace std;

namespace rocket::codec::internal::cbor {

// Functions ------------------------------------------------------------------------------------------------

void
fail(
  nio::Source& in,
  const cbor_load_result& result,
  const optional<string_view>& message) {
  const u64 inPos = in.tell();
  const u64 errPos = inPos + result.error.position;

  const str::Range range = { inPos, errPos };

  string actualMessage;
  switch (result.error.code) {
    case CBOR_ERR_NONE: {
      ROCKET_ASSERT(message);
      actualMessage = *message;
      break;
    }
    case CBOR_ERR_NOTENOUGHDATA: actualMessage = "Not enough data"; break;
    case CBOR_ERR_NODATA: actualMessage = "No data"; break;
    case CBOR_ERR_MALFORMATED: actualMessage = "Malformed data"; break;
    case CBOR_ERR_MEMERROR: actualMessage = "Out of memory"; break;
    case CBOR_ERR_SYNTAXERROR: actualMessage = "Syntax error"; break;
    default: ROCKET_FAIL("Invalid CBOR error code");
  }

  throw InputFailure(errPos, { range }, actualMessage);
}

}; // namespace rocket::codec::internal::cbor

// EOF
