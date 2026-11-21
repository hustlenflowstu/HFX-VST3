#define MyAppName "HFX - Hustle N Flow Effect"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Hustle N Flow"
#define MyAppURL "https://payhip.com/HFCommunity"
#define PluginSource "build\HFX_artefacts\Release\VST3\HFX.vst3"

[Setup]
AppId={{8F3B0A64-8A1B-4F9D-9D65-7C5F0D7A1B10}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={commoncf}\VST3\HFX.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
OutputDir=installer_output
OutputBaseFilename=HFX_Setup_1.0.0_x64
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes
CloseApplications=no

[Files]
Source: "{#PluginSource}\\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    MsgBox('HFX has been installed to the system VST3 folder.' + #13#10 + #13#10 +
      'Restart Ableton Live 11, open Preferences > Plug-Ins, and rescan VST3 plug-ins if HFX does not appear.',
      mbInformation, MB_OK);
end;
