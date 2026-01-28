::
:: make.cmd
::
:: ONLY EDIT THE ORIGINAL FILE, WHICH IS `gaia-make.cmd`.
::
:: Usage:
::   make                (calls `configure` and `build`)
::   make configure
::   make build [TARGET]
::   make test
::   make test-terminal  (for Rocket only)
::
:: To build a specific target:
::
::   > cmake --build --preset windows-release --target TARGET
::
:: To run specific tests:
::
::   > ctest --preset windows-release -R PATTERN -V
::   > build\src\test\Release\test-NAME.exe
::

@echo off

setlocal

if "%1" == "configure" (
  call :configure
  goto :eof
)
if "%1" == "build" (
  call :build
  goto :eof
)
if "%1" == "test" (
  call :test
  goto :eof
)
if "%1" == "test-terminal" (
  call :test-terminal
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

if "%2" == "" (
  cmake --build --preset windows-release
) else (
  echo Building target %2
  cmake --build --preset windows-release --target %2
)
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test -----------------------------------------------------------------------------------------------------

:test

set TEST=%2
if "%TEST%" == "" set TEST=test

if "%TEST%" == "all" (
   echo ALL
   ctest --preset windows-release
) else if "%TEST%" == "test" (
   echo TEST
   ctest --test-dir build\src\test --preset windows-release
) else if "%TEST%" == "bench" (
   echo BENCH
   ctest --test-dir build\src\bench --preset windows-release
) else (
   echo PATTERN
   ctest --preset windows-release -R %2
)
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test-terminal --------------------------------------------------------------------------------------------

:test-terminal

set ROCKET_TEST_TERMINAL=1

build\src\test\Release\test-rocket-system-terminal.exe
build\src\test\Release\test-rocket-unicode-Character.exe

goto :eof

:: EOF
