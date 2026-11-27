@echo off
setlocal
cd /d "%~dp0" || exit /b 1

set "PATH=%CD%\.tool\cmake 4.5.1\bin;%CD%\.tool\ninja v1.13.2;%CD%\.tool\arm-gnu 15.3.rel2\bin;%PATH%"

cmake --build build-manual --target main
if errorlevel 1 exit /b 1

if not exist "build-manual\main.uf2" (
    echo Khong tim thay file UF2.
    exit /b 1
)

for %%F in ("build-manual\main.uf2") do echo UF2: %%~fF [%%~zF bytes]