::
:: build.cmd
::

@echo off

set BUILD_DIR=build\release

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%

cmake --build --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

copy %BUILD_DIR%\compile_commands.json .
if %errorlevel% neq 0 exit /b %errorlevel%

ctest --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%

:: EOF
