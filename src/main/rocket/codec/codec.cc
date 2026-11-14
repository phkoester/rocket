/*
 * codec.cc
 */

#include "rocket/codec/codec.h"

#include "rocket/unicode/Iterator.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace std;

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

// Functions ------------------------------------------------------------------------------------------------

void
beginContainer(nio::Sink& out, bool indent, u64& level, char c) {
  if (indent) {
    ++level;
  }
  out.write(c);
}

void
endContainer(nio::Sink& out, bool indent, u64& level, u64 size, char c) {
  if (indent) {
    --level;
    if (size > 0) {
      out.print("\n{: <{}}", "", 2 * level);
    }
  }
  out.write(c);
}

void
nextElem(nio::Sink& out, bool indent, u64 level, u64 index) {
  if (index > 0) {
    if (indent) {
      out.write(',');
    } else {
      out.write(", ");
    }
  }
  if (indent) {
    out.print("\n{: <{}}", "", 2 * level);
  }
}

void
snapToClusterStarts(DecodeError& error, nio::Source& in) {
  const auto oldPos = in.tell();
  if (not oldPos || not in.seek(0, nio::SeekMode::beg)) {
    return;
  }
  const string text = in.readString();
  in.seek(static_cast<i64>(*oldPos), nio::SeekMode::beg);

  vector<u64> starts { 0 };
  try {
    unicode::Iterator<char> iter(unicode::IteratorType::Character, text);
    for (u64 pos = iter.next(); pos != NPOS; pos = iter.next()) {
      starts.push_back(pos);
    }
  } catch (const InvalidArgument&) {
    return; // Invalid UTF-8 has no clusters to speak of
  }

  // `starts` begins with 0, so the element in front of the upper bound always exists
  const auto snap = [&starts](u64 pos) { return *(ranges::upper_bound(starts, pos) - 1); };

  if (error.position) {
    error.position = snap(*error.position);
  }
  for (auto& range : error.ranges) {
    range.a = snap(range.a);
    if (range.b) {
      *range.b = snap(*range.b);
    }
  }
}

} // namespace rocket::codec::internal

// EOF
