@echo off
setlocal
cd /d "%~dp0"
echo ==========================================
echo HFX - Build Windows Installer
echo ==========================================
where ISCC.exe >nul 2>nul
if errorlevel 1 (
  echo ERROR: Inno Setup 6 is not installed or ISCC.exe is not in PATH.
  echo Install Inno Setup 6 from https://jrsoftware.org/isinfo.php
  echo Then run this file again. You can also compile HFX_Installer.iss from Inno Setup Compiler.
  pause
  exit /b 1
)
if not exist "build\HFX_artefacts\Release\VST3\HFX.vst3" (
  echo HFX VST3 binary not found. Building it now...
  call BUILD_HFX_WINDOWS.bat
  if errorlevel 1 exit /b 1
)
ISCC.exe HFX_Installer.iss
if errorlevel 1 (
  echo Installer build failed. Check the error above.
  pause
  exit /b 1
)
echo.
echo SUCCESS. Your installer is here:
echo installer_output\HFX_Setup_1.0.0_x64.exe
pause
