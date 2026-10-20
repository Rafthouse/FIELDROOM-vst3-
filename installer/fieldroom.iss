; FIELDROOM Windows installer (Inno Setup)
#define MyAppName "FIELDROOM"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "AcousticLab"

[Setup]
AppId={{8F3B2C1A-7C4D-4E2B-9A61-FIELDROOM0001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={commoncf}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=dist-windows
OutputBaseFilename=FIELDROOM-Windows-Setup-x64
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Files]
Source: "build\FIELDROOM_artefacts\Release\VST3\FIELDROOM.vst3\*"; DestDir: "{commoncf}\VST3\FIELDROOM.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
Filename: "https://github.com/Rafthouse/FIELDROOM-vst3-"; Flags: shellexec postinstall skipifsilent unchecked; Description: "Open project page"
