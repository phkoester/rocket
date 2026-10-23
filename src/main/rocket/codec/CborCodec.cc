/*
 * CborCodec.cc
 */

#include "rocket/codec/CborCodec.h"

using namespace rocket;
using namespace std;

namespace rocket::codec::internal::cbor {

// Functions ------------------------------------------------------------------------------------------------

string_view
messageOf(cbor_error_code errorCode) {
  switch (errorCode) {
    case CBOR_ERR_NOTENOUGHDATA: return "Not enough data";
    case CBOR_ERR_NODATA: return "No data";
    case CBOR_ERR_MALFORMATED: return "Malformed data";
    case CBOR_ERR_MEMERROR: return "Memory error";
    case CBOR_ERR_SYNTAXERROR: return "Syntax error";
    default: ROCKET_FAIL("Invalid CBOR error code {}", static_cast<i32>(errorCode));
  }
}

}; // namespace rocket::codec::internal::cbor

// EOF
