::
:: build.cmd
::
:: Usage: build.cmd
::    or  build.cmd test-terminal
::
:: To build a specific target:
::
::   > cmake --preset windows
::   > cmake --build --preset windows-release --target TARGET
::
:: To run specific tests:
::
::   > ctest --preset windows-release -R PATTERN --output-on-failure
::   > ctest --preset windows-release -R PATTERN -V
::   > build\src\test\Release\test-NAME.exe
::

@echo off

if %1 == test-terminal goto :test-terminal

:: main ----------------------------------------------------------------------------------------------------

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

if exist src\test (
   ctest --preset windows-release
   if %errorlevel% neq 0 exit /b %errorlevel%
)

goto :EOF

:: test-terminal --------------------------------------------------------------------------------------------

:test-terminal

setlocal

setROCKET_TEST_TERMINAL=1

build\src\test\Release\test-rocket-system-terminal.exe
build\src\test\Release\test-rocket-unicode-Character.exe

endlocal

goto :EOF

:: EOF
