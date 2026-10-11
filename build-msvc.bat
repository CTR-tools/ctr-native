@echo off
setlocal

:: CTR Native MSVC Build Script (Windows x86)
:: Requires Visual Studio 2022 or Build Tools with the Desktop C++ workload.

where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: cmake not found in PATH
    echo Install CMake 3.20 or newer, or the Visual Studio C++ CMake tools component.
    exit /b 1
)

cmake --preset windows-msvc-x86
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)

cmake --build --preset windows-msvc-x86-release --parallel
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo Build succeeded: build-msvc-x86\Release\ctr_native.exe
