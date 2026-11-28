# Rocket

[![Linux](https://github.com/phkoester/rocket/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/phkoester/rocket/actions/workflows/ci-linux.yml?query=branch%3Amain)
[![macOS](https://github.com/phkoester/rocket/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/phkoester/rocket/actions/workflows/ci-macos.yml?query=branch%3Amain)
[![Windows](https://github.com/phkoester/rocket/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/phkoester/rocket/actions/workflows/ci-windows.yml?query=branch%3Amain)

Rocket is a general-purpose library written in C++, helping you create cross-platform CLI and GUI
applications.

| CMake Target               | Content
| :------------------------- | :------
| `Rocket::rocket`           | Rocket, the basic library for CLI applications
| `Rocket::rocket-bench`     | Rocket.Bench, a library for benchmarks, using Rocket and Google Benchmark
| `Rocket::rocket-gui`       | Rocket.Gui, a library for GUI applications, using Rocket and wxWidgets
| `Rocket::rocket-test`      | Rocket.Test, a library for unit tests, using Rocket and GoogleTest

## CMake Requirements

- CMake 4.2 or higher.
- Python 3.10 or higher. On Windows, both a native Windows Python and an MSYS Python will work.
- On Windows, a Developer Command Prompt from Microsoft Visual Studio is required.

## Third-Party Components

Rocket stands on the shoulders of these giants:

- [128-bit-literals](https://github.com/jbapple/128-bit-literals) ([license](license/128-bit-literals/LICENSE.txt))
- [benchmark](https://github.com/google/benchmark) ([license](license/benchmark/LICENSE))
- [Boost](https://github.com/boostorg/boost) ([license](license/Boost/LICENSE_1_0.txt))
- [{fmt}](https://github.com/fmtlib/fmt) ([license](license/fmt/LICENSE))
- [GoogleTest](https://github.com/google/googletest) ([license](license/GTest/LICENSE))
- [ICU](https://github.com/unicode-org/icu) ([license](license/ICU/LICENSE))
- [libcbor](https://github.com/PJK/libcbor) ([license](license/libcbor/LICENSE.md))
- [scnlib](https://github.com/eliaskosunen/scnlib) ([license](license/scnlib/LICENSE))
- [wxWidgets](https://github.com/wxWidgets/wxWidgets) ([license](license/wxWidgets/licence.txt))

## CMake Variables

All of the following may be supplied either as defined CMake variables or as environment variables:

| Name                              | Type     | Default                           | Description
| :-------------------------------- | :------- | :-------------------------------- | :----------
| `BUILD_SHARED_LIBS`               | `BOOL`   | `OFF`                             | build shared libraries
| `BUILD_TESTING`                   | `BOOL`   | `ON`                              | enable testing: benchmarks and tests
| `CMAKE_BUILD_TYPE`                | `STRING` | `Release` if single configuration | the build type (`Debug` or `Release`)
| `ROCKET_BUILD_BENCH`              | `BOOL`   | `ON` if top-level project         | build benchmarks
| `ROCKET_BUILD_TEST`               | `BOOL`   | `ON` if top-level project         | build tests
| `ROCKET_COMPONENTS`               | `STRING` | `gui` if top-level project        | additional components to enable: `gui`. Set this to `none` to disable additional components
| `ROCKET_ICU_CMAKE`                | `BOOL`   | `OFF`                             | use icu-cmake to build ICU
| `ROCKET_WERROR`                   | `BOOL`   | `ON` if top-level project         | enable `-Werror`

## Environment Variables

| Name                   | Type     | Stage   | Description
| :--------------------- | :------- | :------ | :----------
| `ROCKET_EXIT`          | `bool`   | runtime | if true, `std::exit` is called rather than `std::quick_exit`
| `ROCKET_LOG`           | `string` | runtime | default log options, e.g., `--log all=trace`
| `ROCKET_QUICK_EXIT`    | `bool`   | runtime | if true, `std::quick_exit` is called rather than `std::exit`
| `ROCKET_TEST_TERMINAL` | `bool`   | test    | if true, tests are run that require standard devices to be connected to a terminal

## License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or
  http://www.apache.org/licenses/LICENSE-2.0)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.
