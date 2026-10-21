/*
 * codec.cc
 */

#include "rocket/codec/codec.h"

namespace rocket::codec::internal {

ROCKET_PUBLIC thread_local DecoderStore* store = nullptr;

} // namespace rocket::codec::internal

// EOF
