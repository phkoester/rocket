::
:: make.cmd
::
:: ONLY EDIT THE ORIGINAL FILE, WHICH IS `gaia-make.cmd`.
::
:: Usage:
::   make                                      (calls `configure` and `build`)
::   make info
::   make configure
::   make build [TARGET]
::   make test  [all | bench | test | PATTERN] (default: test)
::   make test-terminal                        (for Rocket only)
::   make clean
::
:: Parameters:
::
:: - BUILD_TYPE
::     The build type: `debug` or `release` (default)
:: - CXX_TOOLCHAIN
::     The C++ toolchain: `llvm` or `msvc` (default)
:: - VERBOSE
::     Produce verbose output
::

@echo off

setlocal enableextensions
SET NAME=%~n0

:: Configure build type -------------------------------------------------------------------------------------

if not defined BUILD_TYPE set BUILD_TYPE=release
if %BUILD_TYPE% neq debug if %BUILD_TYPE% neq release (
  echo %NAME%: `BUILD_TYPE`: Invalid value `%BUILD_TYPE%`; expected `debug` or `release` 1>&2
  exit /b 2
)

if %BUILD_TYPE% == debug set CONFIG=Debug
if %BUILD_TYPE% == release set CONFIG=Release

:: Configure C++ toolchain ----------------------------------------------------------------------------------

if not defined CXX_TOOLCHAIN set CXX_TOOLCHAIN=msvc
if %CXX_TOOLCHAIN% neq llvm if %CXX_TOOLCHAIN% neq msvc (
  echo %NAME%: `CXX_TOOLCHAIN`: Invalid value `%CXX_TOOLCHAIN%`; expected `llvm` or `msvc` 1>&2
  exit /b 2
)

set CMAKE_TOOLCHAIN_FLAG=
if %CXX_TOOLCHAIN% == llvm set CMAKE_TOOLCHAIN_FLAG=-T ClangCL

:: Configure verbose output ---------------------------------------------------------------------------------

set CMAKE_TRAILING_FLAGS=
if defined VERBOSE (
  set CMAKE_TRAILING_FLAGS=-v
)

:: Print info -----------------------------------------------------------------------------------------------

echo ########################################
echo #
echo # BUILD_TYPE   : %BUILD_TYPE%
echo # CXX_TOOLCHAIN: %CXX_TOOLCHAIN%
echo # VERBOSE      : %VERBOSE%
echo #
echo ########################################

:: Parse command --------------------------------------------------------------------------------------------

if "%~1" == "" (
  call :configure
  call :build
  goto :eof
) else if "%1" == "info" (
  goto :eof
) else if "%1" == "configure" (
  call :configure
  goto :eof
) else if "%1" == "build" (
  call :build %2
  goto :eof
) else if "%1" == "test" (
  call :test %2
  goto :eof
) else if "%1" == "test-terminal" (
  call :test-terminal
  goto :eof
) else if "%1" == "clean" (
  call :clean
  goto :eof
) else (
  echo %NAME%: Invalid command `%1` 1>&2
  exit /b 2
)

goto :eof

:: configure ------------------------------------------------------------------------------------------------

:configure

cmake %CMAKE_TOOLCHAIN_FLAG% --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: build ----------------------------------------------------------------------------------------------------

:build

if [%1] == [] (
  cmake --build --preset windows-%BUILD_TYPE% %CMAKE_TRAILING_FLAGS%
) else (
  cmake --build --preset windows-%BUILD_TYPE% --target %1 %CMAKE_TRAILING_FLAGS%
)
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test -----------------------------------------------------------------------------------------------------

:test

set TEST=%1
if not defined TEST set TEST=test

if %TEST% == all (
  ctest --preset windows-%BUILD_TYPE%
) else if %TEST% == bench (
  ctest --test-dir build\src\bench --preset windows-%BUILD_TYPE% -V
) else if %TEST% == test (
  ctest --test-dir build\src\test --preset windows-%BUILD_TYPE%
) else (
  ctest --preset windows-%BUILD_TYPE% -R %TEST% -V
)
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test-terminal --------------------------------------------------------------------------------------------

:test-terminal

set ROCKET_TEST_TERMINAL=1

build\src\test\%CONFIG%\test-rocket-system-terminal.exe
build\src\test\%CONFIG%\test-rocket-unicode-Character.exe

goto :eof

:: clean ----------------------------------------------------------------------------------------------------

:clean

if exist build\ (
  echo Removing build directory. This may take a while ...
  rmdir /q /s build
  if %errorlevel% neq 0 exit /b %errorlevel%
)

goto :eof

:: EOF
