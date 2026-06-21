; ============================================================================
;  AI Synth – Windows installer (Inno Setup)
;  Build this script with Inno Setup 6.x on a Windows machine.
;  It will package the VST3 plugin AND standalone app, install them to
;  the correct Windows locations, and create an uninstaller.
; ============================================================================
#define MyAppName          "AI Synth"
#define MyAppVersion       "1.0.0"
#define MyAppPublisher     "AI Synth Labs"
#define MyAppExeName       "AI Synth.exe"
#define MyVst3Name         "AI Synth.vst3"

[Setup]
AppId={{B5F8A2C1-9D3E-4F2A-B1C7-1A2B3C4D5E6F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://example.com/aisynth
AppSupportURL=https://example.com/aisynth/support
AppUpdatesURL=https://example.com/aisynth/updates
DefaultDirName={commonpf}\AI Synth Labs\AI Synth
DefaultGroupName=AI Synth Labs
DisableProgramGroupPage=yes
OutputDir=..\Installer\Output
OutputBaseFilename=AI Synth Setup
SetupIconFile=..\Assets\icon.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64
UninstallDisplayIcon={app}\{#MyAppExeName}
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "vst3";        Description: "Install VST3 plugin (FL Studio, Cubase, etc.)"; GroupDescription: "Components:"; Flags: checkablealone
Name: "standalone";  Description: "Install standalone application";                 GroupDescription: "Components:"; Flags: checkablealone
Name: "desktopicon"; Description: "Create a &desktop icon";                          GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
; VST3 plugin
Source: "..\build\Release\VST3\AI Synth.vst3"; DestDir: "{commoncf64}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone application
Source: "..\build\Release\Standalone\AI Synth.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
; Supporting files
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\BUILD.md";   DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\AI Synth"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{group}\Uninstall AI Synth"; Filename: "{uninstallexe}"
Name: "{commondesktop}\AI Synth"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; Components: standalone

[Run]
; Offer to launch the standalone app after install
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,AI Synth}"; Flags: nowait postinstall skipifsilent; Components: standalone

[UninstallDelete]
; Clean up user preset folder on uninstall
Type: filesandordirs; Name: "{userappdata}\AI Synth"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
end;
