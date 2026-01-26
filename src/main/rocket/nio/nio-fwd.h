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

ROCKET_IMPORT extern Source& in;
ROCKET_IMPORT extern Sink& out;
ROCKET_IMPORT extern Sink& err;

} // namespace rocket::nio

// EOF
