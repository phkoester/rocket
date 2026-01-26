::
:: build.cmd
::
:: To run a single test with detailed output:
::
::   ctest --preset windows-release -R PATTERN --output-on-failure
::

@echo off

set BUILD_DIR=build\release

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

ctest --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

:: EOF
