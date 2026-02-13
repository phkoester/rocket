/*
 * FormattedCodec.cc
 */

#include "rocket/codec/FormattedCodec.h"

namespace rocket::codec::internal {

// Utilities for encoding -----------------------------------------------------------------------------------

thread_local u64 level = 0;

void
beginContainer(nio::Sink& out, const FormattedConsumerConfig& config, char c) {
  if (config.indent) {
    ++level;
  }
  out.write(c);
}

void
endContainer(nio::Sink& out, const FormattedConsumerConfig& config, u64 size, char c) {
  if (config.indent) {
    --level;
    if (size > 0) {
      out.print("\n{: <{}}", "", 2 * level);
    }
  }
  out.write(c);
}

void
nextElem(nio::Sink& out, const FormattedConsumerConfig& config, u64 index) {
  if (index > 0) {
    if (config.indent) {
      out.write(',');
    } else {
      out.write(", ");
    }
  }
  if (config.indent) {
    out.print("\n{: <{}}", "", 2 * level);
  }
}

} // namespace rocket::codec::internal

// EOF
