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

| Name                | Type     | Default                           | Description
| :------------------ | :------- | :-------------------------------- | :----------
| `BUILD_SHARED_LIBS` | `BOOL`   | `OFF`                             | Build shared libraries
| `BUILD_TESTING`     | `BOOL`   | `ON`                              | Enable testing and build tests
| `CMAKE_BUILD_TYPE`  | `STRING` | `Release` if single configuration | The build type (`Debug` or `Release`)
| `ROCKET_BENCH`      | `BOOL`   | `OFF`                             | Enable benchmarking and build benchmarks
| `ROCKET_TEST`       | `BOOL`   | `ON` if master project            | Enable testing and build tests

### Linux

```bash
cmake -B build
cmake --build build
ctest --test-dir build
cmake --install build --prefix install
```

### Windows

```bash
cmake --preset windows
cmake --build --preset windows-release
ctest --test-dir build -C Release
cmake --install build --config Release --prefix install
```

## Environment Variables

| Name                   | Type     | Description
| :--------------------- | :------- | :----------
| `ROCKET_EXIT`          | `bool`   | If truthy, `std::exit` is called rather than `std::quick_exit`.
| `ROCKET_LOG_FMT`       | `string` | Default log format.
| `ROCKET_QUICK_EXIT`    | `bool`   | If truthy, `std::quick_exit` is called rather than `std::exit`.
| `ROCKET_TEST_TERMINAL` | `bool`   | If truthy, some tests and benchmarks may perform additional terminal tests and produce more terminal output.
