/**
 * @file nio-fwd.h
 *
 * `nio` forward declations.
 */

#pragma once

namespace rocket::nio {

struct Io;
struct Sink;
struct Source;

extern Source& in;
extern Sink& out;
extern Sink& err;

} // namespace rocket::nio

// EOF
