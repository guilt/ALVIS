; ----------------------------------------------------------------
Name "ALVIS - Allegro Visualization Plugin"
Caption "ALVIS - Allegro Visualization Plugin"
Icon "SRC\ALVIS.ICO"
UninstallIcon "SRC\ALVIS.ICO"
OutFile "DIST\alvis.exe"
; ----------------------------------------------------------------

InstallDir $PROGRAMFILES\Winamp
InstallDirRegKey HKEY_LOCAL_MACHINE \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

Function .onInit
  StrCpy $INSTDIR $PROGRAMFILES\Winamp
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" "UninstallString"
  IfErrors 0 CheckWacup
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WACUP" "UninstallString"
  IfErrors 0 GotWacup
  Return

  CheckWacup:
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WACUP" "UninstallString"
  IfErrors Done

  GotWacup:
  StrCpy $INSTDIR $PROGRAMFILES\WACUP
  Goto Done

  Done:
FunctionEnd

Function QueryWinampVisPath ; sets $1 with vis path
  StrCpy $1 $INSTDIR\Plugins
  ReadINIStr $9 $INSTDIR\winamp.ini Winamp VisDir
  StrCmp $9 "" End
  IfFileExists $9 0 End
    StrCpy $1 $9
  End:
FunctionEnd

Function .onVerifyInstDir
  IfFileExists $INSTDIR\Winamp.exe NoAbort
  IfFileExists $INSTDIR\WACUP.exe NoAbort
    Abort
  NoAbort:
FunctionEnd

Section ""

  Call QueryWinampVisPath

  SetOutPath $1

  File "DIST\vis_alvis.dll"
!ifndef STATIC
  File "DIST\alleg42.dll"
!endif
  File "support\boom.bmp"
  File "support\alvis.htm"
  File "support\alvis.css"

WriteUninstaller "$1\Uninstall ALVIS.exe"

  ; Add/Remove Programs registration
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "DisplayName" "ALVIS - Allegro Visualization Plugin"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "UninstallString" '"$1\Uninstall ALVIS.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "Publisher" "Karthik Kumar Viswanathan and Nikhil Karnad"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "DisplayIcon" "$1\Uninstall ALVIS.exe,0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "DisplayVersion" "2.0.0.0"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS" \
    "NoRepair" 1

SectionEnd

Function .onInstSuccess
  ExecShell "open" '"$1\alvis.htm"'
  FindWindow $R0 "winamp v1.x"
  StrCmp $R0 0 "" RunWinamp
  FindWindow $R0 "WACUP"
  StrCmp $R0 0 "" RunWacup
  MessageBox MB_YESNO|MB_ICONQUESTION \
        "The plugin was installed successfully. Would you like to run Winamp now with this plugin set as the default plug-in?" \
        IDNO End
  RunWinamp:
    WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "visplugin_name" "vis_alvis.dll"
    WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "visplugin_num" "0"
    Exec '"$INSTDIR\Winamp.exe"'
    Goto End
  RunWacup:
    WriteINIStr "$INSTDIR\winamp.ini" "Winamp" "visplugin_name" "vis_alvis.dll"
    WriteINIStr "$INSTDIR\winamp.ini" "Winamp" "visplugin_num" "0"
    Exec '"$INSTDIR\WACUP.exe"'
  End:
FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\Plugins\vis_alvis.dll"
  Delete "$INSTDIR\Plugins\alleg42.dll"
  Delete "$INSTDIR\boom.bmp"
  Delete "$INSTDIR\alvis.ini"
  Delete "$INSTDIR\alvis.htm"
  Delete "$INSTDIR\alvis.css"
  Delete "$INSTDIR\Plugins\Uninstall ALVIS.exe"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\ALVIS"

SectionEnd

; eof
