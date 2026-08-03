#ifndef SourceRoot
  #define SourceRoot "..\..\dist\windows\stage"
#endif
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist\windows"
#endif

[Setup]
AppId={{9B092204-EBEC-4EE6-B364-DA15AD32A4DD}
AppName=SonRapTune
AppVersion={#AppVersion}
AppPublisher=MasArray
DefaultDirName={autopf}\MasArray\SonRapTune
DefaultGroupName=SonRapTune
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=SonRapTune-{#AppVersion}-Windows-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayIcon={app}\SonRapTune.exe

[Files]
Source: "{#SourceRoot}\VST3\SonRapTune.vst3\*"; DestDir: "{commoncf64}\VST3\SonRapTune.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceRoot}\Standalone\SonRapTune.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\SonRapTune"; Filename: "{app}\SonRapTune.exe"
Name: "{autodesktop}\SonRapTune"; Filename: "{app}\SonRapTune.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Run]
Filename: "{app}\SonRapTune.exe"; Description: "Launch SonRapTune Standalone"; Flags: nowait postinstall skipifsilent
