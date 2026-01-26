/**
 * @file nio-fwd.h
 *
 * `nio` forward declations.
 */

#pragma once

#include "rocket/rocket.h"

namespace rocket::nio {

struct Io;
struct Sink;
struct Source;

ROCKET_PUBLIC_SYMBOL extern Source& in;
ROCKET_PUBLIC_SYMBOL extern Sink& out;
ROCKET_PUBLIC_SYMBOL extern Sink& err;

} // namespace rocket::nio

// EOF
