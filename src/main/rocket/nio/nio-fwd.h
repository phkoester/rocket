/**
 * @file nio-fwd.h
 *
 * `nio` forward declarations.
 */

#pragma once

#include "rocket/rocket.h"

namespace rocket::nio {

struct Device;

struct Sink;
struct BufferedSink;
struct NullSink;
struct SpanSink;
struct StringSink;

struct Source;
struct ContiguousSource;
struct BufferedSource;
struct NullSource;
struct SpanSource;
struct StringSource;

ROCKET_PUBLIC extern Source& in; // NOLINT
ROCKET_PUBLIC extern Sink& out; // NOLINT
ROCKET_PUBLIC extern Sink& err; // NOLINT

} // namespace rocket::nio

// EOF
