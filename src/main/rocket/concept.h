/**
 * @file concept.h
 *
 * General C++ concepts.
 */

#pragma once

#include <type_traits>

namespace rocket {

// `Container` ----------------------------------------------------------------------------------------------

/**
 * `IsContainerImpl` template.
 *
 * This template is to be specialized.
 */
template<typename> struct IsContainerImpl: std::false_type {};

/**
 * `IsContainer` template.
 *
 * @tparam T the type to test
 */
template<typename T> struct IsContainer : IsContainerImpl<std::decay_t<T>>::type {};

/**
 * A concept for containers.
 *
 * @tparam T the type to test
 */
template<typename T> concept Container = IsContainer<T>::value;

} // namespace rocket

// EOF
