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

u64
positionOf(span<const u8> cbor, u64 index) {
  unsigned char* data = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(cbor.data()));
  u64 itemIndex = 0;
  u64 pos = 0;
  cbor_decoder_result result; // NOLINT

  while (pos < cbor.size()) {
    if (itemIndex == index) {
      return pos;
    }

    result = cbor_stream_decode(data + pos, cbor.size() - pos, nullptr, nullptr);
    if (result.status == CBOR_DECODER_FINISHED) {
      ++itemIndex;
      pos += result.read;
    } else {
      throw InputFailure(pos, result.status == CBOR_DECODER_NEDATA ? "Not enough data" : "Malformed data");
    }
  }

  return NPOS;
}

}; // namespace rocket::codec::internal::cbor

// EOF
