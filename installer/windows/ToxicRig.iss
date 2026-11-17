; ToxicRig - installer per Windows (Inno Setup 6).
; Compilato automaticamente da GitHub Actions:
;   ISCC.exe /DAppVersion=0.3.0 /DSourceDir=<cartella con ToxicRig.vst3 e ToxicRig.exe> ToxicRig.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\dist"
#endif

[Setup]
AppId={{6F1B7C1E-3A0D-4C52-9E2B-7A55D1C0B7E1}
AppName=ToxicRig
AppVersion={#AppVersion}
AppVerName=ToxicRig {#AppVersion}
AppPublisher=ToxicZone
AppPublisherURL=https://toxiczone.it
AppSupportURL=https://toxiczone.it
DefaultDirName={autopf}\ToxicZone\ToxicRig
DefaultGroupName=ToxicZone
DisableProgramGroupPage=yes
LicenseFile=license_it.txt
OutputBaseFilename=ToxicRig-Setup-{#AppVersion}
OutputDir=..\..\dist-installer
SetupIconFile=ToxicRig.ico
UninstallDisplayIcon={app}\ToxicRig.ico
UninstallDisplayName=ToxicRig (ToxicZone)
WizardStyle=modern
WizardImageFile=wizard.bmp,wizard@2x.bmp
WizardSmallImageFile=wizard_small.bmp,wizard_small@2x.bmp
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
VersionInfoCompany=ToxicZone
VersionInfoProductName=ToxicRig
VersionInfoVersion={#AppVersion}

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Installazione completa"
Name: "custom"; Description: "Personalizzata"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3 (FL Studio, Ableton, Reaper, Cubase, Studio One...)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Applicazione standalone (suonare senza DAW)"; Types: full
Name: "stage"; Description: "Palco 3D (interfaccia 3D del plugin)"; Types: full custom; Flags: fixed

[Tasks]
Name: "desktopicon"; Description: "Crea un'icona sul desktop per la versione standalone"; Components: standalone; Flags: unchecked

[Files]
; Il plugin VST3 e' una cartella: va copiata intera nella cartella VST3 comune,
; quella dove FL Studio e tutte le DAW cercano i plugin.
Source: "{#SourceDir}\ToxicRig.vst3\*"; DestDir: "{commoncf64}\VST3\ToxicRig.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\ToxicRig.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
; Il palco 3D: un solo archivio in ProgramData, dove il plugin lo cerca (vedi plugin/WebEditor.cpp).
Source: "{#SourceDir}\ToxicRig-Stage.zip"; DestDir: "{commonappdata}\ToxicZone\ToxicRig"; Components: stage; Flags: ignoreversion
Source: "ToxicRig.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "LEGGIMI.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\ToxicRig"; Filename: "{app}\ToxicRig.exe"; Components: standalone
Name: "{group}\Leggimi ToxicRig"; Filename: "{app}\LEGGIMI.txt"
Name: "{group}\Disinstalla ToxicRig"; Filename: "{uninstallexe}"
Name: "{autodesktop}\ToxicRig"; Filename: "{app}\ToxicRig.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\ToxicRig.exe"; Description: "Avvia ToxicRig standalone"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\ToxicRig.vst3"
Type: files; Name: "{commonappdata}\ToxicZone\ToxicRig\ToxicRig-Stage.zip"
Type: dirifempty; Name: "{commonappdata}\ToxicZone\ToxicRig"

[Messages]
italian.FinishedLabel=ToxicRig e' installato.%n%nIn FL Studio: Options > Manage plugins > Find more plugins, poi cerca "ToxicRig" e mettilo tra i preferiti (stellina).
english.FinishedLabel=ToxicRig is installed.%n%nIn your DAW, rescan plugins and look for "ToxicRig".
