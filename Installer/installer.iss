; ============================================================================
;  AI Synth – Windows installer (Inno Setup 6.x)
;  Build with:   iscc installer.iss
;  Or open in Inno Setup Compiler and press Ctrl+F9.
; ============================================================================

#define MyAppName          "AI Synth"
#define MyAppVersion       "1.0.0"
#define MyAppPublisher     "AI Synth Labs"
#define MyAppExeName       "AI Synth.exe"

; Icon is optional – falls back to Inno Setup default if not present
#if FileExists(AddBackslash(ExtractFilePath(SourcePath)) + "..\Assets\icon.ico")
  #define MyAppIcon "..\Assets\icon.ico"
#else
  #define MyAppIcon
#endif

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
#if defined(MyAppIcon) && MyAppIcon != ""
SetupIconFile={#MyAppIcon}
#endif
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
Name: "vst3";        Description: "Install VST3 plugin (FL Studio, Cubase, etc.)"; GroupDescription: "Components:"; Flags: checkablealone checked
Name: "standalone";  Description: "Install standalone application";                 GroupDescription: "Components:"; Flags: checkablealone checked
Name: "desktopicon"; Description: "Create a &desktop icon";                          GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
; VST3 plugin – look for it in the standard CMake output locations
Source: "..\build\AISynth_artefacts\Release\VST3\AI Synth.vst3"; DestDir: "{commoncf64}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs; Check: DirExists(ExpandConstant(..\build\AISynth_artefacts\Release\VST3))
Source: "..\build\VST3\AI Synth.vst3";                         DestDir: "{commoncf64}\VST3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not DirExists(ExpandConstant(..\build\AISynth_artefacts\Release\VST3))
; Standalone application
Source: "..\build\AISynth_artefacts\Release\Standalone\AI Synth.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion; Check: FileExists(ExpandConstant(..\build\AISynth_artefacts\Release\Standalone\AI Synth.exe))
Source: "..\build\Standalone\AI Synth.exe";                          DestDir: "{app}"; Components: standalone; Flags: ignoreversion; Check: not FileExists(ExpandConstant(..\build\AISynth_artefacts\Release\Standalone\AI Synth.exe))
; Supporting files
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\BUILD.md";   DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\AI Synth"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{group}\Uninstall AI Synth"; Filename: "{uninstallexe}"
Name: "{commondesktop}\AI Synth"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; Components: standalone

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,AI Synth}"; Flags: nowait postinstall skipifsilent; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{userappdata}\AI Synth"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
