::
:: make.cmd
::
:: ONLY EDIT THE ORIGINAL FILE, WHICH IS `gaia-make.cmd`.
::
:: Usage:
::   make                                      (calls `configure` and `build`)
::   make configure
::   make build [TARGET]
::   make test  [all | bench | test | PATTERN] (default: test)
::   make test-terminal                        (for Rocket only)
::   make clean
::
:: Parameters:
::
:: - BUILD_TYPE
::     The build type: `debug` or `release`

@echo off

setlocal

:: Configure build type -------------------------------------------------------------------------------------

if not defined BUILD_TYPE set BUILD_TYPE=release
if %BUILD_TYPE% neq debug if %BUILD_TYPE% neq release (
 echo make.cmd: Invalid value `%BUILD_TYPE%` for BUILD_TYPE; expected `debug` or `release` 1>&2
 exit /b 2
)

if %BUILD_TYPE% == debug set CONFIG=Debug
if %BUILD_TYPE% == release set CONFIG=Release

echo ################################################################################
echo #
echo # BUILD_TYPE: %BUILD_TYPE%
echo #
echo ################################################################################

:: Parse command --------------------------------------------------------------------------------------------

if "%1" == "configure" (
  call :configure
  goto :eof
)
if "%1" == "build" (
  call :build %2
  goto :eof
)
if "%1" == "test" (
  call :test %2
  goto :eof
)
if "%1" == "test-terminal" (
  call :test-terminal
  goto :eof
)
if "%1" == "clean" (
  call :clean
  goto :eof
)

:: main ----------------------------------------------------------------------------------------------------

call :configure
call :build

goto :eof

:: configure ------------------------------------------------------------------------------------------------

:configure

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: build ----------------------------------------------------------------------------------------------------

:build

if [%1] == [] (
  cmake --build --preset windows-%BUILD_TYPE%
) else (
  cmake --build --preset windows-%BUILD_TYPE% --target %1
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
