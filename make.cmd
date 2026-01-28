::
:: make.cmd
::
:: ONLY EDIT THE ORIGINAL FILE, WHICH IS `gaia-make.cmd`.
::
:: Usage: build [configure | build | test | test-terminal]
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
call :test

goto :eof

:: configure ------------------------------------------------------------------------------------------------

:configure

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: build ----------------------------------------------------------------------------------------------------

:build

cmake --build --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

goto :eof

:: test -----------------------------------------------------------------------------------------------------

:test

if exist src\test (
   ctest --preset windows-release
   if %errorlevel% neq 0 exit /b %errorlevel%
)

goto :eof

:: test-terminal --------------------------------------------------------------------------------------------

:test-terminal

setlocal
set ROCKET_TEST_TERMINAL=1
build\src\test\Release\test-rocket-system-terminal.exe
build\src\test\Release\test-rocket-unicode-Character.exe
endlocal

goto :eof

:: EOF
