@echo off
setlocal

:: Crash Team Racing Expanded Build Script (Windows)
:: Requires MSYS2 MinGW32 tools and UCRT64 Ninja in PATH

set "CRASH_TEAM_RACING_EXPANDED_MSYS2_PACKAGES=git mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-ucrt-x86_64-ninja"

where i686-w64-mingw32-gcc >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: i686-w64-mingw32-gcc not found in PATH
    echo.
    echo Install MSYS2, then run:
    echo   pacman -S --needed %CRASH_TEAM_RACING_EXPANDED_MSYS2_PACKAGES%
    echo Then add C:\msys64\mingw32\bin and C:\msys64\ucrt64\bin to your PATH
    exit /b 1
)

where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: cmake not found in PATH
    echo.
    echo Install MSYS2, then run:
    echo   pacman -S --needed %CRASH_TEAM_RACING_EXPANDED_MSYS2_PACKAGES%
    echo Then add C:\msys64\mingw32\bin and C:\msys64\ucrt64\bin to your PATH
    exit /b 1
)

where ninja >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: ninja not found in PATH
    echo.
    echo Install MSYS2, then run:
    echo   pacman -S --needed %CRASH_TEAM_RACING_EXPANDED_MSYS2_PACKAGES%
    echo Then add C:\msys64\mingw32\bin and C:\msys64\ucrt64\bin to your PATH
    exit /b 1
)

cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)

:: Bound parallel jobs: bare -j starts an unlimited number of compilers.
if not defined CMAKE_BUILD_PARALLEL_LEVEL set "CMAKE_BUILD_PARALLEL_LEVEL=%NUMBER_OF_PROCESSORS%"
if not defined CMAKE_BUILD_PARALLEL_LEVEL set "CMAKE_BUILD_PARALLEL_LEVEL=2"
cmake --build build
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo.
echo Build succeeded: build\crash_team_racing_expanded.exe
