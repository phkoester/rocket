# Rocket

Rocket is an all-purpose library written in C++.

It is pre-alpha, a hobby project of mine, and possibly dangerous. **Do not use it**.

## Third-Party Components

Rocket stands on the shoulders of these giants:

- [128-bit-literals](https://github.com/jbapple/128-bit-literals) ([license](license/128-bit-literals/LICENSE.txt))
- [benchmark](https://github.com/google/benchmark) ([license](license/benchmark/LICENSE))
- [Boost](https://github.com/boostorg/boost) ([license](license/Boost/LICENSE_1_0.txt))
- [{fmt}](https://github.com/fmtlib/fmt) ([license](license/fmt/LICENSE))
- [GoogleTest](https://github.com/google/googletest) ([license](license/GTest/LICENSE))
- [ICU](https://github.com/unicode-org/icu)  ([license](license/ICU/LICENSE))
- [scnlib](https://github.com/eliaskosunen/scnlib) ([license](license/scnlib/LICENSE))

## CMake Variables:

| Name                              | Type     | Default                           | Description
| :-------------------------------- | :------- | :-------------------------------- | :----------
| `BUILD_SHARED_LIBS`               | `BOOL`   | `OFF`                             | Build shared libraries
| `BUILD_TESTING`                   | `BOOL`   | `ON`                              | Enable testing and build tests
| `CMAKE_BUILD_TYPE`                | `STRING` | `Release` if single configuration | The build type (`Debug` or `Release`)
| `ROCKET_BUILD_BENCH`              | `BOOL`   | `ON` if master project            | Enable benchmarking and build benchmarks
| `ROCKET_BUILD_TEST`               | `BOOL`   | `ON` if master project            | Enable testing and build tests
| `ROCKET_NIO_LOG`                  | `BOOL`   | `OFF`                             | Enable logging of `rocket::nio`
| `ROCKET_NIO_NO_CONTIGUOUS_SOURCE` | `BOOL`   | `OFF`                             | Disable contiguous-source optimization for `rocket::nio`

## Environment Variables

| Name                   | Type     | Stage   | Description
| :--------------------- | :------- | :------ | :----------
| `ROCKET_EXIT`          | `bool`   | Runtime | If `true`, `std::exit` is called rather than `std::quick_exit`.
| `ROCKET_LOG_FMT`       | `string` | Runtime | Default log format.
| `ROCKET_QUICK_EXIT`    | `bool`   | Runtime | If `true`, `std::quick_exit` is called rather than `std::exit`.
| `ROCKET_TEST_TERMINAL` | `bool`   | Test    | If `true`, tests are run that require standard devices to be connected to a terminal.
