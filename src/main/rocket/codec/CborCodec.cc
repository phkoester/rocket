/*
 * CborCodec.cc
 */

#include "rocket/codec/CborCodec.h"

using namespace rocket;
using namespace std;

namespace rocket::codec::internal::cbor {

// Functions ------------------------------------------------------------------------------------------------

void
loadFailed(nio::Source& in, const cbor_load_result& result) {
  string message;
  switch (result.error.code) {
    case CBOR_ERR_NOTENOUGHDATA: message = "Not enough data"; break;
    case CBOR_ERR_NODATA: message = "No data"; break;
    case CBOR_ERR_MALFORMATED: message = "Malformed data"; break;
    case CBOR_ERR_MEMERROR: message = "Out of memory"; break;
    case CBOR_ERR_SYNTAXERROR: message = "Syntax error"; break;
    default: ROCKET_FAIL("Invalid CBOR error code");
  }

  throw InputFailure(in.tell(), message);
}

}; // namespace rocket::codec::internal::cbor

// EOF
