rem
rem build.cmd
rem

@echo off

cmake --preset windows
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --build --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%
ctest --preset windows-release
if %errorlevel% neq 0 exit /b %errorlevel%
cmake --install build --config Release --prefix install
if %errorlevel% neq 0 exit /b %errorlevel%

rem EOF
