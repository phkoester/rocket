# Rocket

Rocket is a general-purpose library written in C++, helping you to create cross-platform CLI and GUI
applications.

CMake Target               | Content
:------------------------- | :------
`Rocket::rocket`           | The basic Rocket library for CLI applications
`Rocket::rocket-bench`     | Rocket.Bench, a library for benchmarks, using Rocket and Google benchmark
`Rocket::rocket-gui`       | Rocket.Gui, a library for GUI applications, using Rocket and WxWidgets
`Rocket::rocket-test`      | Rocket.Test, a library for unit tests, using Rocket and GoogleTest
`Rocket::rocket-test-main` | The `main` function for unit tests, using Rocket and GoogleTest

## CMake Requirements

* CMake 4.2 or higher.
* Python 3.8 or higher. On Windows, both a native Windows Python and an MSYS Python will work.
* The Python package `semver`.
* On Windows, a Developer Command Prompt from Microsoft Visual Studio is required.

## Third-Party Components

Rocket stands on the shoulders of these giants:

- [128-bit-literals](https://github.com/jbapple/128-bit-literals) ([license](license/128-bit-literals/LICENSE.txt))
- [benchmark](https://github.com/google/benchmark) ([license](license/benchmark/LICENSE))
- [Boost](https://github.com/boostorg/boost) ([license](license/Boost/LICENSE_1_0.txt))
- [{fmt}](https://github.com/fmtlib/fmt) ([license](license/fmt/LICENSE))
- [GoogleTest](https://github.com/google/googletest) ([license](license/GTest/LICENSE))
- [ICU](https://github.com/unicode-org/icu)  ([license](license/ICU/LICENSE))
- [libcbor](https://github.com/PJK/libcbor) ([license](license/libcbor/LICENSE.md))
- [scnlib](https://github.com/eliaskosunen/scnlib) ([license](license/scnlib/LICENSE))
- [wxWidgets](https://github.com/wxWidgets/wxWidgets) ([license](license/wxWidgets/licence.txt))

ICU is made available through CMake by [icu-cmake](https://github.com/phkoester/icu-cmake).

## CMake Options

All of the following options may be supplied either as defined CMake variables or as environment variables:

| Name                              | Type     | Default                           | Description
| :-------------------------------- | :------- | :-------------------------------- | :----------
| `BUILD_SHARED_LIBS`               | `BOOL`   | `OFF`                             | Build shared libraries (`.so` on Linux, DLL on Windows) instead of static libraries
| `BUILD_TESTING`                   | `BOOL`   | `ON`                              | Enable testing and build tests
| `CMAKE_BUILD_TYPE`                | `STRING` | `Release` if single configuration | The build type (`Debug` or `Release`)
| `COVERAGE`                        | `BOOL`   | `OFF`                             | Enable code coverage
| `ROCKET_BUILD_BENCH`              | `BOOL`   | `ON` if master project            | Enable benchmarking and build benchmarks
| `ROCKET_BUILD_TEST`               | `BOOL`   | `ON` if master project            | Build the unit tests
| `ROCKET_COMPONENTS`               | `STRING` | &mdash;                           | Additional components to enable: `gui`
| `ROCKET_IO_LOG`                   | `BOOL`   | `OFF`                             | Enable logging of `rocket::io`
| `ROCKET_NIO_LOG`                  | `BOOL`   | `OFF`                             | Enable logging of `rocket::nio`
| `ROCKET_NIO_NO_CONTIGUOUS_SOURCE` | `BOOL`   | `OFF`                             | Disable contiguous-source optimization for `rocket::nio`

## Environment Variables

| Name                   | Type     | Stage   | Description
| :--------------------- | :------- | :------ | :----------
| `ROCKET_EXIT`          | `bool`   | Runtime | If true, `std::exit` is called rather than `std::quick_exit`.
| `ROCKET_LOG`           | `string` | Runtime | Default log options, e.g. `--log all=trace`
| `ROCKET_QUICK_EXIT`    | `bool`   | Runtime | If true, `std::quick_exit` is called rather than `std::exit`.
| `ROCKET_TEST_TERMINAL` | `bool`   | Test    | If true, tests are run that require standard devices to be connected to a terminal.

## License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or
  http://www.apache.org/licenses/LICENSE-2.0)
- MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.
