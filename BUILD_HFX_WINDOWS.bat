@echo off
setlocal
cd /d "%~dp0"
echo ==========================================
echo HFX - Windows VST3 Build
echo ==========================================
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake is not installed or not in PATH.
  echo Install CMake from https://cmake.org/download/
  pause
  exit /b 1
)
cmake -S . -B build -A x64
if errorlevel 1 (
  echo.
  echo Build configuration failed. Make sure Visual Studio 2022 C++ workload is installed.
  pause
  exit /b 1
)
cmake --build build --config Release --target HFX_VST3
if errorlevel 1 (
  echo.
  echo Compilation failed. Copy the complete error output when requesting support.
  pause
  exit /b 1
)
echo.
echo SUCCESS. Plugin output should be in:
echo build\HFX_artefacts\Release\VST3\HFX.vst3
pause
