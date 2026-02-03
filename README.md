# Rocket

Rocket is an all-purpose library written in C++.

It is pre-alpha, a hobby project of mine, and possibly dangerous. **Do not use it**.

## Build with CMake

Some notable environment variables respected by CMake:

| Name       | Description
| :--------- | -----------
| `CC`       | Path to the C compiler
| `CFLAGS`   | Additional flags for the C compiler
| `CXX`      | Path to the C++ compiler
| `CXXFLAGS` | Additional flags for the C++ compiler

CMake variables:

| Name                 | Type     | Default                           | Description
| :------------------- | :------- | :-------------------------------- | :----------
| `BUILD_SHARED_LIBS`  | `BOOL`   | `OFF`                             | Build shared libraries
| `BUILD_TESTING`      | `BOOL`   | `ON`                              | Enable testing and build tests
| `CMAKE_BUILD_TYPE`   | `STRING` | `Release` if single configuration | The build type (`Debug` or `Release`)
| `ROCKET_BUILD_BENCH` | `BOOL`   | `ON` if master project            | Enable benchmarking and build benchmarks
| `ROCKET_BUILD_TEST`  | `BOOL`   | `ON` if master project            | Enable testing and build tests

### Linux

```bash
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
cmake --install build/release --prefix install
```

### Windows

```bash
cmake --preset windows
cmake --build --preset windows-release
ctest --preset windows-release
cmake --install build --config Release --prefix install
```

Alternatively, run `make.cmd`. For usage information, look inside the file.

## Environment Variables

| Name                   | Type     | Stage   | Description
| :--------------------- | :------- | :------ | :----------
| `ROCKET_EXIT`          | `bool`   | Runtime | If `true`, `std::exit` is called rather than `std::quick_exit`.
| `ROCKET_LOG_FMT`       | `string` | Runtime | Default log format.
| `ROCKET_QUICK_EXIT`    | `bool`   | Runtime | If `true`, `std::quick_exit` is called rather than `std::exit`.
| `ROCKET_TEST_TERMINAL` | `bool`   | Test    | If `true`, tests are run that require standard devices to be connected to a terminal.
