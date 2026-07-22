#define ProjectName GetEnv('PROJECT_NAME')
#define ProductName GetEnv('PRODUCT_NAME')
#define ProductVersion GetEnv('VERSION')
#define Publisher GetEnv('COMPANY_NAME')
#define Year GetDateTimeString("yyyy","","")

; ArtefactsPath is normally passed in via `iscc /DArtefactsPath=<path>` (see CMake/packaging.cmake,
; which resolves it to wherever the current build actually put AppInDAW_artefacts - "build/" for
; the default preset, "release-build/" for the release preset, etc.) - this fallback only applies
; if someone compiles the script directly without that define.
#ifndef ArtefactsPath
  #define ArtefactsPath "..\..\build\AppInDAW_artefacts\Release"
#endif

[Setup]
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
AppName={#ProductName}
OutputBaseFilename={#ProductName}-{#ProductVersion}-Windows
AppCopyright=Copyright (C) {#Year} {#Publisher}
AppPublisher={#Publisher}
AppVersion={#ProductVersion}
DefaultDirName="{commoncf64}\VST3\{#ProductName}.vst3"
DisableDirPage=yes
LicenseFile="resources\EULA"
UninstallFilesDir="{commonappdata}\{#ProductName}\uninstall"

; MSVC adds a .ilk when building the plugin - excluded so it doesn't ship in the installer.
[Files]
Source: "{#ArtefactsPath}\VST3\{#ProductName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#ProductName}.vst3\"; Excludes: *.ilk; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{autoprograms}\Uninstall {#ProductName}"; Filename: "{uninstallexe}"
