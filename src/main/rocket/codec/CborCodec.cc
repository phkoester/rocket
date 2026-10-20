/*
 * CborCodec.cc
 */

#include "rocket/codec/CborCodec.h"

using namespace rocket;
using namespace std;

namespace {

// Local functions ------------------------------------------------------------------------------------------

void
fail(nio::Source& in, const cbor_load_result& result) {
  string_view message;
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

} // namespace

namespace rocket::codec::internal::cbor {

// Functions ------------------------------------------------------------------------------------------------

cbor_item_t*
loadItem(nio::ContiguousSource& in) {
  auto buf = in.bytes();
  cbor_load_result result;
  cbor_item_t* ret = cbor_load(buf.data(), buf.size(), &result);
  if (result.error.code != CBOR_ERR_NONE) {
    ROCKET_ASSERT(ret == nullptr);
    fail(in, result);
  }
  in.seek(result.read, nio::SeekMode::cur);
  return ret;
}

}; // namespace rocket::codec::internal::cbor

// EOF
