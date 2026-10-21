/*
 * codec.cc
 */

#include "rocket/codec/codec.h"

namespace rocket::codec::internal {

// Variables ------------------------------------------------------------------------------------------------

thread_local DecoderStore* currentStore = nullptr;

// `DecoderStore` -------------------------------------------------------------------------------------------

DecoderStore*
DecoderStore::get() {
  return currentStore;
}

void
DecoderStore::set(DecoderStore* val) {
  currentStore = val;
}

} // namespace rocket::codec::internal

// EOF
