/**
 * @file io-decl.h
 *
 * I/O utilities: declarations.
 */

// XXX File ganz weg?

#pragma once

#include "base.h"

#include <ios>
#include <set>
#include <spanstream>
#include <string>

namespace rocket::io {

// Constants ------------------------------------------------------------------------------------------------

/**
 * The default buffer size in bytes.
 */
static constexpr size_t DEFAULT_BUFFER_SIZE = 64 * 1'024; // 64 KiB
/**
  * The minimum buffer size in bytes.
  */
static constexpr size_t MIN_BUFFER_SIZE = 128;

// Functions ------------------------------------------------------------------------------------------------

void check(std::istream&);

char getChar(std::istream&);

char getChar(std::istream&, char);

char getChar(std::istream&, const std::set<char>&);

template<typename I> requires Integer<I>
I getHex(std::istream&, size_t, std::string&);

std::string
getString(std::istream&, const std::set<std::string_view>&);

std::ispanstream is(
    std::span<char> = std::span<char>(),
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

std::ispanstream is(
    const char*,
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

std::ispanstream is(
    const std::string&,
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

std::ispanstream is(
    std::string_view,
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

std::istream& seekg(std::istream&, size_t);

size_t tellg(std::istream&) noexcept;

} // namespace rocket::io

// EOF
