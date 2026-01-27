::
:: build.cmd
::
:: To build a specific target:
::
::   > cmake --preset windows
::   > cmake --build --preset windows-release --target TARGET
::
:: To run a single test:
::
::   > ctest --preset windows-release -R PATTERN --output-on-failure
::   > ctest --preset windows-release -R PATTERN -v
::   > build\release\debug\src\test\test-NAME.exe
::

@echo off

set BUILD_DIR=build\release

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

if exist src\test (
   ctest --preset windows-release
   if %errorlevel% neq 0 exit /b %errorlevel%
)

:: EOF
