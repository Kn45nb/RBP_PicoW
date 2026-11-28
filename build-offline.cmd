@echo off
setlocal
cd /d "%~dp0" || exit /b 1
set "PATH=%CD%\.tool\cmake 4.5.1\bin;%CD%\.tool\ninja v1.13.2;%CD%\.tool\arm-gnu 15.3.rel2\bin;%CD%\.tool\python;%PATH%"

cmake -S . -B build-offline -G Ninja -DPICO_BOARD=pico_w -DPICO_NO_PICOTOOL=ON -DFETCHCONTENT_FULLY_DISCONNECTED=ON
if errorlevel 1 exit /b 1

cmake --build build-offline --target main
if errorlevel 1 exit /b 1

if /i "%~1"=="check" (
    echo Build OK: build-offline\main.elf
    exit /b 0
)

set "PICOTOOL=%CD%\.tool\picotool\picotool.exe"
if not exist "%PICOTOOL%" (
    echo Missing: "%PICOTOOL%"
    exit /b 1
)

"%PICOTOOL%" uf2 convert "build-offline\main.elf" "build-offline\main-new.uf2" --family rp2040 --platform rp2040
if errorlevel 1 exit /b 1
move /y "build-offline\main-new.uf2" "build-offline\main.uf2" >nul || exit /b 1
for %%F in ("build-offline\main.uf2") do echo UF2: %%~fF [%%~zF bytes]
