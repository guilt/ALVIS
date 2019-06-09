; ----------------------------------------------------------------
Name "ALVIS - Allegro Visualization Plugin"
Caption "ALVIS - Allegro Visualization Plugin"
OutFile "alvis.exe"
; ----------------------------------------------------------------

InstallDir $PROGRAMFILES\Winamp
InstallDirRegKey HKEY_LOCAL_MACHINE \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

DirText "The installer has determined that this is the path to Winamp. If it is not correct, please change (You will not be able to proceed until a valid path is found)."

Function QueryWinampVisPath ; sets $1 with vis path
  StrCpy $1 $INSTDIR\Plugins
  ReadINIStr $9 $INSTDIR\winamp.ini Winamp VisDir
  StrCmp $9 "" End
  IfFileExists $9 0 End
    StrCpy $1 $9 ; update dir
  End: 
FunctionEnd

Function .onVerifyInstDir
  IfFileExists $INSTDIR\Winamp.exe NoAbort
    Abort
  NoAbort:
FunctionEnd

; The stuff to install
Section ""

  ; this function sets $1 to point to the 'winamp\plugins' folder:
  ; (see function definition above)
  Call QueryWinampVisPath

; ----------------------------------------------------------------
  ; Set the current output path (the folder to which files will be 
  ; decompressed) to '$1', which is the path to the winamp plugins 
  ; folder.
  SetOutPath $1

  ; Now list all the files you want to go into that folder on the 
  ; user's computer.  Use local paths to point to the files; 
  ;   these paths will not be preserved for the decompression.
  File "src\vis_alvis.dll"
  File "support\boom.bmp"
  File "support\alvis.htm"
  File "support\alvis.css"

; ----------------------------------------------------------------
  
; ----------------------------------------------------------------
  ; Note: If you want to create a subdirectory called 'XYZ' underneath 
  ; the winamp plugins directory, to store more files for your plugin,
  ; you would do something like this:
  ;    SetOutPath $1\XYZ
  ;    File "C:\Program Files\winamp\plugins\XYZ\*.*"
; ----------------------------------------------------------------

;Create uninstaller
WriteUninstaller "$INSTDIR\PLUGINS\Uninstall ALVIS.exe"
 
SectionEnd

Function .onInstSuccess
  ExecShell "open" '"$INSTDIR\PLUGINS\alvis.htm"'
  FindWindow $R0 "winamp v1.x"
  StrCmp $R0 0 "" NoOpen2
  MessageBox MB_YESNO|MB_ICONQUESTION \
        "The plugin was installed successfully. Would you like to run Winamp now with this plugin set as the default plug-in?" \
        IDNO End
    WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "visplugin_name" "vis_alvis.dll"
    WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "visplugin_num" "0"
    Exec '"$INSTDIR\Winamp.exe"'
  Goto end
  NoOpen2:
  MessageBox MB_OK "The plugin was successfully installed.  Press CTRL+P while in Winamp to select, configure, and run it."
End:
FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN STUFF HERE!

  Delete "$INSTDIR\vis_alvis.dll"
  Delete "$INSTDIR\boom.bmp"
  Delete "$INSTDIR\alvis.ini"
  Delete "$INSTDIR\alvis.htm"
  Delete "$INSTDIR\alvis.css"
  Delete "$INSTDIR\Uninstall ALVIS.exe"

SectionEnd

; eof
