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

ROCKET_EXPORT extern Source& in;
ROCKET_EXPORT extern Sink& out;
ROCKET_EXPORT extern Sink& err;

} // namespace rocket::nio

// EOF
