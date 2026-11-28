HFX — HUSTLE N FLOW EFFECT
Windows 11 / Ableton Live 11 / VST3

WHAT THIS PACKAGE IS
This is the HFX JUCE source project plus Windows build and installer scripts.
It does not contain a precompiled VST3 binary or a ready-made Setup.exe yet.
A Windows MSVC build is required to produce the plugin binary before the
installer can package it.

EASIEST ROUTE TO YOUR HFX INSTALLER
1. Install Visual Studio 2022 Community.
2. In Visual Studio Installer, select "Desktop development with C++".
   Ensure MSVC v143, Windows 10/11 SDK, and C++ CMake tools are selected.
3. Install CMake for Windows.
4. Install Inno Setup 6 (the free installer creator): https://jrsoftware.org/isinfo.php
5. Extract this ZIP, then double-click BUILD_HFX_WINDOWS.bat.
   This downloads JUCE 8.0.6 and compiles the HFX VST3 plugin.
6. When the build succeeds, double-click BUILD_HFX_INSTALLER.bat.
7. The Windows installer will be created at:
   installer_output\HFX_Setup_1.0.0_x64.exe
8. Run HFX_Setup_1.0.0_x64.exe. Approve the Windows administrator prompt.
9. Restart Ableton Live 11. Open Preferences > Plug-Ins, enable VST3 and rescan.
   Find HFX under Plug-ins > VST3.

INTERNET IS REQUIRED for the first build because CMake downloads JUCE 8.0.6.

WHAT IS IMPLEMENTED IN THIS SOURCE PROJECT
- Seven-tab HFX interface with dark/orange custom LookAndFeel; Drum Bus is shown only for percussive beat stems.
- Five-band parametric EQ with preset-driven HP/LP/shelf/bell shaping, compressor,
  soft saturation, M/S width, left/right pan, Utility gain and sample-peak limiting.
- Dedicated Drum Bus tab for Kick, Snare, Percussion, Closed Hat, Open Hat and EFX.
  Drum Bus provides Drive, Damp, Boom and Boom Frequency controls. These six roles
  bypass Compressor, Saturator, Utility and Limiter; 808/Bass and Melody retain
  the full dynamics/saturation/output chain.
- Beat/Vocal track mode.
- Factory starting presets.
- Four-second RMS/peak/crest-factor Auto-Mix recommendations.
- Live FFT spectrum and stereo peak/RMS meters.
- APVTS host automation and session state serialization.

IMPORTANT AUDIO ENGINE NOTES
This is a functional first-release candidate foundation, not a fully certified
commercial mastering processor. Auto-Mix is rule-based, not a trained AI model.
Metering is dBFS peak/RMS, not BS.1770 LUFS. The output limiter is not an
oversampled true-peak lookahead limiter. The EQ is not a full dynamic multi-band
EQ. Always compare processed and bypassed audio, gain-match, and keep backups.


GITHUB BUILD OPTION
See GET_HFX_SETUP_EXE.txt for a click-by-click route to build the Windows installer
on GitHub Actions without installing Visual Studio on your PC.


HFX UPDATE NOTES - UTILITY + PRESET REVISION
- Added a Utility tab with one centered output-gain knob (default 0.00 dB).
- Utility gain is applied immediately before the peak limiter, so the limiter remains the final peak-protection stage.
- Utility gain is saved/automated as a host parameter: utilityGain.
- Vocal presets use the requested 4:1 compressor, 5 ms attack, 60 ms release, -29.8 dB threshold, 2 dB drive, 25% saturation mix, 100% width, +11 dB limiter input gain, -1 dB ceiling, and +6 dB Utility.
- Beat preset categories: Kick, Snare, Percussion, Closed Hat, Open Hat, EFX, 808/Bass, Melody.
- 808/Bass and Melody now have separate preset groups.
- Presets use modest variations while keeping their requested instrument/voice tonal direction.
- Presets are starting points. Source recording level, arrangement, phase, and playback level still require listening and adjustment.


HFX UPDATE NOTES - DRUM BUS MIXING WORKFLOW
- Added Drum Bus as a seventh tab. It is available only in Beat mode for Kick,
  Snare, Percussion, Closed Hat, Open Hat and EFX.
- Drum Bus controls: Drive (%), Damp (low-pass cutoff in Hz), Boom (%) and
  Boom Frequency (Hz).
- For those six percussive roles, the DSP path is EQ -> Drum Bus -> Pan/Width.
  Compressor, Saturator, Utility and Limiter are bypassed by design.
- 808/Bass and Melody do not show Drum Bus; they retain EQ -> Compressor ->
  Saturator -> Utility -> Limiter -> Pan/Width.
- Each of the 60 percussive presets has a more distinct EQ curve and Drum Bus
  setting set. Their frequency anchors follow the supplied Kick, Snare, Percussion,
  Closed Hat and Open Hat guide screenshots; pan/width follows the same references.
- Drum Bus intentionally has no limiter. Watch peak levels and lower the source
  or Drive if the signal clips. No fixed preset can guarantee the same result
  across every recording or sample.
- New APVTS parameter IDs: drumDrive, drumDamp, drumBoom and drumFreq. Existing
  parameter IDs were retained for session compatibility.
