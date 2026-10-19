Unicode true
RequestExecutionLevel user
!include "MUI2.nsh"
!include "LogicLib.nsh"
Name "Coneic Player ${PRODUCT_VERSION}"
OutFile "${OUTFILE}"
InstallDir "$LOCALAPPDATA\Programs\Coneic"
InstallDirRegKey HKCU "Software\BlueCloud\Coneic" "InstallLocation"
VIProductVersion "${FILE_VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "Coneic Player"
VIAddVersionKey /LANG=1033 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "FileVersion" "${FILE_VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName" "BLUECLOUD"
VIAddVersionKey /LANG=1033 "FileDescription" "Coneic Player Windows installer"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Coneic contributors; VLC Authors and VideoLAN"
Icon "${APPDIR}\Coneic.ico"
UninstallIcon "${APPDIR}\Coneic.ico"
SetCompressor /SOLID lzma
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${APPDIR}\COPYING.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Coneic Player"
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    File /r "${APPDIR}\*"
    WriteUninstaller "$INSTDIR\Uninstall Coneic.exe"
    CreateDirectory "$SMPROGRAMS\Coneic"
    CreateShortcut "$SMPROGRAMS\Coneic\Coneic Player.lnk" "$INSTDIR\Coneic.exe" "" "$INSTDIR\Coneic.ico"
    CreateShortcut "$SMPROGRAMS\Coneic\Set Coneic as default.lnk" "$INSTDIR\Set Coneic as default.cmd" "" "$INSTDIR\Coneic.ico"
    WriteRegStr HKCU "Software\BlueCloud\Coneic" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "DisplayName" "Coneic Player"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "Publisher" "BLUECLOUD"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "UninstallString" '"$INSTDIR\Uninstall Coneic.exe"'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "QuietUninstallString" '"$INSTDIR\Uninstall Coneic.exe" /S'
    WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "DisplayIcon" "$INSTDIR\Coneic.ico"
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "NoModify" 1
    WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic" "NoRepair" 1
    nsExec::ExecToLog '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$INSTDIR\Coneic-shell\register-file-associations.ps1" -AppDirectory "$INSTDIR"'
    Pop $0
    ${If} $0 != 0
        DetailPrint "File associations could not be registered. Run Set Coneic as default.cmd to retry."
    ${EndIf}
SectionEnd

Section "Uninstall"
    SetShellVarContext current
    nsExec::ExecToLog '"$SYSDIR\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$INSTDIR\Coneic-shell\register-file-associations.ps1" -AppDirectory "$INSTDIR" -Action Unregister'
    Pop $0
    Delete "$SMPROGRAMS\Coneic\Coneic Player.lnk"
    Delete "$SMPROGRAMS\Coneic\Set Coneic as default.lnk"
    RMDir "$SMPROGRAMS\Coneic"
    # Delete only packaged files. Extra user-created files and all user profiles remain.
    !include "${DELETE_INCLUDE}"
    Delete "$INSTDIR\Uninstall Coneic.exe"
    RMDir "$INSTDIR"
    DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\BlueCloud.Coneic"
    DeleteRegKey HKCU "Software\BlueCloud\Coneic"
SectionEnd
