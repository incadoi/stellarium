; Stellarium installer
; Run "make install" first to generate the executable and translation files.
; @ISS_AUTOGENERATED_WARNING@

[Setup]
@ISS_ARCHITECTURE_SPECIFIC@
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVersion=@PACKAGE_VERSION@
AppVerName=Stellarium @PACKAGE_VERSION@
AppPublisher=Stellarium team
AppPublisherURL=http://www.stellarium.org/
OutputBaseFilename=stellarium-@PACKAGE_VERSION@-@ISS_PACKAGE_PLATFORM@
OutputDir=installers
; In 64-bit mode, {pf} is equivalent to {pf64},
; see http://www.jrsoftware.org/ishelp/index.php?topic=32vs64bitinstalls
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\data\stellarium.ico
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "@CMAKE_INSTALL_PREFIX@\bin\stellarium.exe"; DestDir: "{app}"
@STELMAINLIB@
@REDIST@
Source: "stellarium.url"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}"; DestName: "ChangeLog.rtf"
@ZLIB@
Source: "@QtCore_location@"; DestDir: "{app}";
Source: "@QtGui_location@"; DestDir: "{app}";
Source: "@QtOpenGL_location@"; DestDir: "{app}";
Source: "@QtNetwork_location@"; DestDir: "{app}";
Source: "@QtWidgets_location@"; DestDir: "{app}";
Source: "@QtDeclarative_location@"; DestDir: "{app}";
Source: "@QtSql_location@"; DestDir: "{app}";
Source: "@QtXmlPatterns_location@"; DestDir: "{app}";
Source: "@QtConcurrent_location@"; DestDir: "{app}";
@ISS_QT_SCRIPT@
@ISS_QT_MULTIMEDIA@
@ISS_ANGLE_LIBS@
@ISS_ICU_LIBS@
@ISS_WINDOWS_PLUGIN@
@ISS_QML_DIR@
@ISS_QML_PLUGINS@
@ISS_QML_SHADERS@
Source: "@CMAKE_INSTALL_PREFIX@\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs

[Tasks]
Name: desktopicon; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: desktopicon\common; Description: "{cm:ForAllUsers}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: exclusive
Name: desktopicon\user; Description: "{cm:ForCurrentUserOnly}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: exclusive unchecked
Name: removeconfig; Description: "{cm:RemoveMainConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
Name: removeplugins; Description: "{cm:RemovePluginsConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
Name: removesolar; Description: "{cm:RemoveSolarConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
Name: removelandscapes; Description: "{cm:RemoveUILandscapes}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
;Name: removeshortcuts; Description: "{cm:RemoveShortcutsConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked

[Run]
;An option to start Stellarium after setup has finished
Filename: "{app}\stellarium.exe"; Description: "{cm:LaunchProgram,Stellarium}"; Flags: postinstall nowait skipifsilent unchecked

[InstallDelete]
;The old log file in all cases
Type: files; Name: "{userappdata}\Stellarium\log.txt"
Type: files; Name: "{userappdata}\Stellarium\config.ini"; Tasks: removeconfig
Type: files; Name: "{userappdata}\Stellarium\data\ssystem.ini"; Tasks: removesolar
Type: filesandordirs; Name: "{userappdata}\Stellarium\modules"; Tasks: removeplugins
Type: filesandordirs; Name: "{userappdata}\Stellarium\landscapes"; Tasks: removelandscapes
;Type: files; Name: "{userappdata}\Stellarium\data\shortcuts.json"; Tasks: removeshortcuts

[UninstallDelete]

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,Stellarium}"; Filename: "{app}\stellarium.url"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium {cm:FallbackMode}"; Filename: "{app}\stellarium.exe"; Parameters: "--safe-mode"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\{cm:UninstallProgram,Stellarium}"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{userappdata}\Stellarium\config.ini"
Name: "{group}\{cm:LastRunLog}"; Filename: "{userappdata}\Stellarium\log.txt"
Name: "{group}\{cm:ChangeLog}"; Filename: "{app}\ChangeLog.rtf"
Name: "{commondesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\common
Name: "{userdesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\user

; Recommended use Inno Setup 5.5.3+
[Languages]
; Official translations of GUI of Inno Setup + translation Stellarium specific lines
Name: "en"; MessagesFile: "compiler:Default.isl,util\ISL\EnglishCM.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl,util\ISL\CatalanCM.isl"
Name: "co"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "da"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "fi"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl,util\ISL\FrenchCM.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "el"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "he"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hu"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "no"; MessagesFile: "compiler:Languages\Norwegian.isl,util\ISL\NorwegianCM.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl,util\ISL\BrazilianPortugueseCM.isl"
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl,util\ISL\RussianCM.isl"
Name: "sr"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "uk"; MessagesFile: "compiler:Languages\Ukrainian.isl,util\ISL\UkrainianCM.isl"
; Unofficial translations of GUI of Inno Setup
Name: "bg"; MessagesFile: "util\ISL\Bulgarian.isl,util\ISL\BulgarianCM.isl"
Name: "bs"; MessagesFile: "util\ISL\Bosnian.isl,util\ISL\BosnianCM.isl"
Name: "gla"; MessagesFile: "util\ISL\ScotsGaelic.isl"

