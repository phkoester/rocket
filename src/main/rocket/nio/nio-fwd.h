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

extern Source& stdin;
extern Sink& stdout;
extern Sink& stderr;

} // namespace rocket::nio

// EOF
