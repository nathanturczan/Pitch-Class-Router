; Pitch Class Router Windows Installer Script
; Inno Setup Script - https://jrsoftware.org/isinfo.php

#define MyAppName "Pitch Class Router"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Scale Navigator LLC"
#define MyAppURL "https://scalenavigator.com"

[Setup]
AppId={{B4E9C0D3-8E5F-6A2B-9C0D-3E5F6A2B9C0D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=PitchClassRouter-Setup-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation"
Name: "vst3only"; Description: "VST3 only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 Plugin"; Types: full vst3only custom; Flags: fixed

[Files]
; VST3 Plugin - copies to Common Files VST3 folder
Source: "..\..\plugin\build\PitchClassRouter_artefacts\Release\VST3\Pitch Class Router.vst3\*"; DestDir: "{commoncf64}\VST3\Pitch Class Router.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3

[Icons]
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    MsgBox('Pitch Class Router has been installed successfully!' + #13#10 + #13#10 +
           'VST3 installed to:' + #13#10 +
           ExpandConstant('{commoncf64}\VST3\Pitch Class Router.vst3') + #13#10 + #13#10 +
           'Please restart your DAW and rescan for plugins.',
           mbInformation, MB_OK);
  end;
end;
