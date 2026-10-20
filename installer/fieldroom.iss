; FIELDROOM Windows VST3 Installer Script (Inno Setup)
#define MyAppName "FIELDROOM"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "AcousticLab"
#define MyAppURL "https://github.com/Rafthouse/FIELDROOM-vst3-"

[Setup]
AppId={{D37D584C-76B1-4C5C-9A8E-9529F74A6A81}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=FIELDROOM-Windows-Setup-x64
OutputDir=..\dist-windows
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Copy 64-bit VST3 bundle into standard Windows VST3 directory (C:\Program Files\Common Files\VST3\FIELDROOM.vst3)
Source: "..\build\FIELDROOM_artefacts\Release\VST3\FIELDROOM.vst3\*"; DestDir: "{commoncf64}\VST3\FIELDROOM.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[Messages]
WelcomeLabel2=This will install [name/ver] on your computer.%n%nFIELDROOM is a professional binaural virtual loudspeaker monitoring and headphone calibration VST3 plugin with zero-latency 64-bit floating-point DSP.
