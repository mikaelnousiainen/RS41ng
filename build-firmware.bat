@echo off
REM Build the RS41ng firmware using the Docker build environment.
REM
REM Usage:
REM   build-firmware.bat [--vaisala-cal [SERIAL]] [config-file.yaml] [extra cmake flags...]
REM
REM Examples:
REM   build-firmware.bat                     - use config.yaml (default)
REM   build-firmware.bat my-tracker.yaml     - use a different config file
REM   build-firmware.bat -DRS41=1            - no config file; pass target flag for the built-in config
REM   build-firmware.bat my-tracker.yaml -DRS41=1
REM   build-firmware.bat --vaisala-cal V1221335 -DRS41=1
REM
REM --vaisala-cal fetches the sonde's factory PTU calibration from SondeHub (by
REM the serial printed on the sticker; prompts for it when omitted) and generates
REM src\vaisala_boom_cal.h before building - see tools\fetch_vaisala_boom_cal.py.
REM Use together with SENSOR_VAISALA_BOOM_ENABLE and SENSOR_VAISALA_BOOM_CAL_MODE 2.
REM
REM The first argument, if it does not start with "-", selects the configuration
REM YAML file (relative to this script's directory). When that file exists, the
REM hardware target is derived from its hardware.type field, so no -D flag is needed.
REM Any remaining arguments are forwarded to cmake. Requires the rs41ng_compiler
REM Docker image (build it once with: docker build -t rs41ng_compiler .).

setlocal enabledelayedexpansion

REM Run from the repository root (where this script lives).
cd /d "%~dp0"

REM Optional: fetch the sonde's factory calibration from SondeHub before building.
if /I not "%~1"=="--vaisala-cal" goto skip_vaisala_cal
shift
set "CAL_SERIAL=%~1"
if defined CAL_SERIAL (
    set "CAL_LEAD=!CAL_SERIAL:~0,1!"
    if "!CAL_LEAD!"=="-" (
        set "CAL_SERIAL="
    ) else (
        shift
    )
)
python tools\fetch_vaisala_boom_cal.py !CAL_SERIAL!
if errorlevel 1 exit /b 1
:skip_vaisala_cal

set "CONFIG_FILE=config.yaml"
set "CONFIG_EXPLICIT=0"

set "FIRST=%~1"
if defined FIRST (
    set "LEAD=!FIRST:~0,1!"
    if not "!LEAD!"=="-" (
        set "CONFIG_FILE=%~1"
        set "CONFIG_EXPLICIT=1"
        shift
    )
)

REM Collect any remaining arguments to forward to cmake.
set "EXTRA_ARGS="
:collect
if not "%~1"=="" (
    set "EXTRA_ARGS=!EXTRA_ARGS! %~1"
    shift
    goto collect
)

docker image inspect rs41ng_compiler >nul 2>&1
if errorlevel 1 (
    echo ERROR: Docker image "rs41ng_compiler" not found. 1>&2
    echo Build it once with: docker build -t rs41ng_compiler . 1>&2
    exit /b 1
)

if not exist "%CONFIG_FILE%" (
    if "%CONFIG_EXPLICIT%"=="1" (
        echo ERROR: config file "%CONFIG_FILE%" not found in %cd% 1>&2
        exit /b 1
    )
    echo No "%CONFIG_FILE%" found - building with the built-in manual configuration ^(config.h / config.c^).
    echo You must pass a target flag, e.g.: build-firmware.bat -DRS41=1
) else (
    echo Using configuration file: %CONFIG_FILE%
)

docker run --rm -it -v "%cd%:/usr/local/src/RS41ng" -e "CONFIG_FILE=%CONFIG_FILE%" rs41ng_compiler%EXTRA_ARGS%
