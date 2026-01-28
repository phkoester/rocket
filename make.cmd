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

@echo off

setlocal

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
  cmake --build --preset windows-release
) else (
  cmake --build --preset windows-release --target %1
)
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test -----------------------------------------------------------------------------------------------------

:test

set TEST=%1
if not defined TEST set TEST=test

if %TEST% == all (
   ctest --preset windows-release
) else if %TEST% == test (
   ctest --test-dir build\src\test --preset windows-release
) else if %TEST% == bench (
   ctest --test-dir build\src\bench --preset windows-release
) else (
   ctest --preset windows-release -R %TEST% -V
)
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test-terminal --------------------------------------------------------------------------------------------

:test-terminal

set ROCKET_TEST_TERMINAL=1

build\src\test\Release\test-rocket-system-terminal.exe
build\src\test\Release\test-rocket-unicode-Character.exe

goto :eof

:: clean ----------------------------------------------------------------------------------------------------

:clean

echo Removing build directory. This may take a while ...
rmdir /q /s build
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: EOF
