/**
 * @file util.h
 *
 * Additional `nio` utilities.
 */

#pragma once

#include "nio.h"

namespace rocket::nio {

size_t getChar(nio::Source& in, char& out);

size_t getChar(nio::Source& in, char& out, char expected);

} // namespace rocket::nio
