@echo off
setlocal
cd /d "%~dp0"
set "SRC=%~dp0build\HFX_artefacts\Release\VST3\HFX.vst3"
set "DEST=%COMMONPROGRAMFILES%\VST3\HFX.vst3"
if not exist "%SRC%" (
  echo HFX VST3 was not found.
  echo Run BUILD_HFX_WINDOWS.bat first and check for build errors.
  pause
  exit /b 1
)
if not exist "%COMMONPROGRAMFILES%\VST3" mkdir "%COMMONPROGRAMFILES%\VST3"
if exist "%DEST%" rmdir /s /q "%DEST%"
xcopy "%SRC%" "%DEST%" /E /I /Y
if errorlevel 1 (
  echo Installation failed. Try running this file as Administrator.
  pause
  exit /b 1
)
echo HFX VST3 installed to:
echo %DEST%
echo Restart Ableton Live and rescan VST3 plug-ins.
pause
