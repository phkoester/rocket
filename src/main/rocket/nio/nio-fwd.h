/**
 * @file nio-fwd.h
 *
 * `nio` forward declarations.
 */

#pragma once

#include "rocket/rocket.h"

namespace rocket::nio {

struct Io;
struct Sink;
struct Source;

ROCKET_PUBLIC extern Source& in;
ROCKET_PUBLIC extern Sink& out;
ROCKET_PUBLIC extern Sink& err;

} // namespace rocket::nio

// EOF
