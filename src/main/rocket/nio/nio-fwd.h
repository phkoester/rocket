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
struct BufferedSink;
struct StringSink;

struct Source;
struct BufferedSource;
struct StringSource;

ROCKET_PUBLIC extern Source& in; // NOLINT
ROCKET_PUBLIC extern Sink& out; // NOLINT
ROCKET_PUBLIC extern Sink& err; // NOLINT

} // namespace rocket::nio

// EOF
