; ENCORE. Basement Studio installer for Windows (Inno Setup 6)
;
;   ISCC.exe /DAppVersion=0.4.0 /DBuildDir=..\build\EncoreStudio_artefacts\Release installer\EncoreStudio.iss
;
; Makes dist\Encore-Studio-<version>-windows-setup.exe: Start menu and (optional)
; desktop shortcuts, opens .encore songs (and .basement ones from before the
; rename) on double-click, and a normal uninstaller. Songs and settings are
; never installed or removed by it: they live in the user's own folders.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\build\EncoreStudio_artefacts\Release"
#endif
; ENCORE. VST's own installer, offered as an extra (CI downloads the newest one
; from github.com/jimmyrlondo/encore-vst; without it the option isn't shown)
#ifndef VstSetup
  #define VstSetup "..uildst\ENCORE-VST-setup.exe"
#endif
#ifndef BridgeDir
  #define BridgeDir "..\build32\EncoreStudioBridge32_artefacts\Release"
#endif

[Setup]
AppId={{85CBF2F3-407D-4790-8345-BA701885E638}
AppName=ENCORE. Basement Studio
AppVersion={#AppVersion}
AppVerName=ENCORE. Basement Studio {#AppVersion}
AppPublisher=ENCORE. Music
AppPublisherURL=https://encoremusic.studio
AppSupportURL=https://github.com/jimmyrlondo/encore-studio/issues
AppUpdatesURL=https://github.com/jimmyrlondo/encore-studio/releases
DefaultDirName={autopf}\Encore Studio
DefaultGroupName=Encore Studio
DisableProgramGroupPage=yes
; installs for everyone (asks for admin) or just this user, the person's choice
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
CloseApplications=yes
WizardStyle=modern
SetupIconFile=encore.ico
UninstallDisplayIcon={app}\Encore Studio.exe
UninstallDisplayName=ENCORE. Basement Studio
OutputDir=..\dist
OutputBaseFilename=Encore-Studio-{#AppVersion}-windows-setup
Compression=lzma2/max
SolidCompression=yes
InfoBeforeFile=welcome.txt

[Tasks]
Name: "desktopicon"; Description: "Put ENCORE. Basement Studio on the desktop"; GroupDescription: "Shortcuts:"
Name: "associate"; Description: "Open .encore songs with ENCORE. Basement Studio"; GroupDescription: "Songs:"
#if FileExists(VstSetup)
Name: "vst"; Description: "Also install ENCORE. VST: the same plugins for your other DAWs (Cubase, Reaper, FL Studio and more). You pick which ones next."; GroupDescription: "Extras:"; Flags: unchecked
#endif

[Files]
Source: "{#BuildDir}\Encore Studio.exe"; DestDir: "{app}"; Flags: ignoreversion
#if FileExists(AddBackslash(BridgeDir) + "Encore Studio Bridge32.exe")
Source: "{#BridgeDir}\Encore Studio Bridge32.exe"; DestDir: "{app}"; Flags: ignoreversion
#endif
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"
Source: "..\README.md"; DestDir: "{app}"
#if FileExists(VstSetup)
Source: "{#VstSetup}"; DestDir: "{tmp}"; DestName: "ENCORE VST Setup.exe"; Flags: deleteafterinstall; Tasks: vst
#endif

[InstallDelete]
; shortcuts from before the renames (Encore Studio, Encore Basement, Encore Basement Studio)
Type: files; Name: "{autoprograms}\Encore Studio.lnk"
Type: files; Name: "{autodesktop}\Encore Studio.lnk"
Type: files; Name: "{autoprograms}\Encore Basement.lnk"
Type: files; Name: "{autodesktop}\Encore Basement.lnk"
Type: files; Name: "{autoprograms}\Encore Basement Studio.lnk"
Type: files; Name: "{autodesktop}\Encore Basement Studio.lnk"

[Icons]
Name: "{autoprograms}\ENCORE. Basement Studio"; Filename: "{app}\Encore Studio.exe"
Name: "{autodesktop}\ENCORE. Basement Studio"; Filename: "{app}\Encore Studio.exe"; Tasks: desktopicon

[Registry]
; .encore and .basement songs open in ENCORE. Basement Studio (per user or machine, whichever this install is)
Root: HKA; Subkey: "Software\Classes\.encore"; ValueType: string; ValueName: ""; ValueData: "EncoreStudio.Song"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.basement"; ValueType: string; ValueName: ""; ValueData: "EncoreStudio.Song"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\EncoreStudio.Song"; ValueType: string; ValueName: ""; ValueData: "ENCORE. Basement Studio song"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\EncoreStudio.Song\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\Encore Studio.exe,0"; Tasks: associate
Root: HKA; Subkey: "Software\Classes\EncoreStudio.Song\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Encore Studio.exe"" ""%1"""; Tasks: associate

[Run]
#if FileExists(VstSetup)
; its own window, with a checkbox for each plugin; Basement Studio has them all built in either way
Filename: "{tmp}\ENCORE VST Setup.exe"; StatusMsg: "Opening the ENCORE. VST installer..."; Flags: shellexec waituntilterminated skipifsilent; Tasks: vst
#endif
Filename: "{app}\Encore Studio.exe"; Description: "Open ENCORE. Basement Studio now"; Flags: nowait postinstall skipifsilent
