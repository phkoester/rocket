/**
 * @file io-decl.h
 *
 * I/O utilities: declarations.
 */

#pragma once

#include "basic.h"

#include <ios>
#include <set>
#include <span>
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

template<typename C> requires Character<C>
void check(std::basic_istream<C>&);

template<typename C> requires Character<C>
C getChar(std::basic_istream<C>&);

template<typename C> requires Character<C>
C getChar(std::basic_istream<C>&, C);

template<typename C> requires Character<C>
C getChar(std::basic_istream<C>&, const std::set<C>&);

template<typename I, typename C> requires Integer<I> && Character<C>
I getHex(std::basic_istream<C>&, size_t, std::basic_string<C>&);

template<typename C> requires Character<C>
std::basic_string<C>
getString(std::basic_istream<C>&, const std::set<std::basic_string_view<C>>&);

template<typename C = char> requires Character<C>
std::basic_ispanstream<C> is(
    std::span<C> = std::span<C>(),
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

template<typename C> requires Character<C>
std::basic_ispanstream<C> is(
    const C*,
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

template<typename C> requires Character<C>
std::basic_ispanstream<C> is(
    const std::basic_string<C>&,
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

template<typename C> requires Character<C>
std::basic_ispanstream<C> is(
    std::basic_string_view<C>,
    std::ios::openmode = std::ios::in,
    std::ios::iostate = std::ios::badbit);

template<typename C> requires Character<C>
std::basic_istream<C>& seekg(std::basic_istream<C>&, size_t);

template<typename C> requires Character<C>
size_t tellg(std::basic_istream<C>&) noexcept;

} // namespace rocket::io

// EOF
