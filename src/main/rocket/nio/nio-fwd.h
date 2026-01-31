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

ROCKET_PUBLIC extern Source& in; // NOLINT
ROCKET_PUBLIC extern Sink& out; // NOLINT
ROCKET_PUBLIC extern Sink& err; // NOLINT

} // namespace rocket::nio

// EOF
