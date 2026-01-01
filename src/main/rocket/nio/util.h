/**
 * @file util.h
 *
 * Additional `nio` utilities.
 */

#pragma once

#include "rocket/assert.h"
#include "rocket/nio/nio.h"
#include "rocket/unicode/unicode.h"

namespace rocket::nio {

char getChar(Source& in, char expected);

char getChar(Source& in, const char* expected, const char* what);

unicode::Grapheme getGrapheme(Source& in);

uint32_t getHex(Source& in, size_t n);

std::optional<char> getOptionalChar(Source& in);

std::optional<unicode::Grapheme> getOptionalGrapheme(Source& in);

} // namespace rocket::nio
