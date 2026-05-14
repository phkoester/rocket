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
if not defined GAIA_DIR (
  set GAIA_DIR=..\gaia
)
call %GAIA_DIR%\bin\gaia-make.cmd %*

:: EOF
