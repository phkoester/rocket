::
:: make.cmd
::
:: Usage: See `gaia-make.cmd`.
::
:: Parameters:
::
:: - GAIA_DIR
::     The directory of the Gaia project
::

@echo off

setlocal enableextensions
set NAME=%~n0

if not defined GAIA_DIR (
  echo %NAME%: Environment variable `GAIA_DIR` is not defined. 1>&2
  exit /b 2
)

call %GAIA_DIR%\bin\gaia-make.cmd %*

:: EOF
