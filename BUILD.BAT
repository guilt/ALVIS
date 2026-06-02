@ECHO OFF
IF NOT DEFINED EXTDEV ECHO ERROR: EXTDEV environment variable not set. && EXIT /B 1
SET APP=%~dp0
CD /D "%APP%"

IF "%1"=="BOTH" GOTO :BUILDBOTH
IF "%1"=="ALL" GOTO :BUILDBOTH
IF "%1"=="1" SET STATIC=1
IF "%1"=="STATIC" SET STATIC=1
IF "%1"=="2" SET STATIC=1

CALL :BUILDONCE
VER>NUL
EXIT /B %ERRORLEVEL%

:BUILDBOTH
SET STATIC=0
CALL :BUILDONCE
IF %ERRORLEVEL% NEQ 0 EXIT /B 1
SET STATIC=1
CALL :BUILDONCE
IF %ERRORLEVEL% NEQ 0 EXIT /B 1
ECHO BOTH modes completed successfully.
VER>NUL
EXIT /B 0

:BUILDONCE
SET MINGDIR=%EXTDEV%\MinGW32
SET GCC="%MINGDIR%\Bin\Gcc"
SET WINDRES="%MINGDIR%\Bin\WindRes"
SET CFLAGS=-std=gnu99 -Wall -Wno-unused-parameter -m32 -I"SRC" -I"%EXTDEV%\Allegro-MSVC8-MinGW-BCC\Include"
SET LDFLAGS=-static-libgcc -m32 -mwindows -Wl,--allow-multiple-definition
SET CLFLAGS=-x c -m32
SET LIBS=-L"%EXTDEV%\Allegro-MSVC8-MinGW-BCC\Lib\MinGW\x86"
SET DEFINES=-DBUILDING_DLL=1
SET ALLEGRO_LIBS=-lalleg -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lole32 -ldinput -lddraw -ldxguid -lwinmm -ldsound

IF "%STATIC%"=="1" (
    SET DEFINES=%DEFINES% -DALLEGRO_STATICLINK
    SET ALLEGRO_LIBS=-lalleg_s %ALLEGRO_LIBS:-lalleg =%
    SET LDFLAGS=%LDFLAGS%
    ECHO --- STATIC BUILD ---
) ELSE (
    SET LDFLAGS=%LDFLAGS% -shared
    ECHO --- DYNAMIC BUILD ---
)

IF NOT "%DEBUG%"=="1" GOTO :CFLAGSREL
SET CFLAGS=%CFLAGS% -O0 -g -DDEBUG=1
GOTO :CFLAGSDONE
:CFLAGSREL
SET CFLAGS=%CFLAGS% -O1 -Wall -fomit-frame-pointer -ffast-math -funroll-loops
:CFLAGSDONE

MKDIR OBJ 2>NUL
MKDIR DIST 2>NUL

ECHO Compiling...
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\ALVIS.C -o OBJ\ALVIS.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\COMMON.C -o OBJ\COMMON.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\BEAT.C -o OBJ\BEAT.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\FFT.C -o OBJ\FFT.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\VIS_WAVE.C -o OBJ\VIS_WAVE.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\VIS_SPEC.C -o OBJ\VIS_SPEC.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\VIS_VU.C -o OBJ\VIS_VU.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\VIS_GRND.C -o OBJ\VIS_GRND.OBJ
%GCC% %CFLAGS% %CLFLAGS% %DEFINES% -c SRC\VIS_BOOM.C -o OBJ\VIS_BOOM.OBJ
%WINDRES% -I SRC SRC\RES.RC -o OBJ\RES.OBJ
IF %ERRORLEVEL% NEQ 0 ECHO Compilation failed && EXIT /B 1

ECHO Linking...
%GCC% %LDFLAGS% -o DIST\vis_alvis.dll OBJ\ALVIS.OBJ OBJ\COMMON.OBJ OBJ\BEAT.OBJ OBJ\FFT.OBJ OBJ\RES.OBJ OBJ\VIS_WAVE.OBJ OBJ\VIS_SPEC.OBJ OBJ\VIS_VU.OBJ OBJ\VIS_GRND.OBJ OBJ\VIS_BOOM.OBJ %LIBS% %ALLEGRO_LIBS%
IF %ERRORLEVEL% NEQ 0 ECHO DLL linking failed && EXIT /B 1
IF NOT EXIST DIST\vis_alvis.dll EXIT /B 1
ECHO SUCCESS: vis_alvis.dll built.

ECHO Building fuzz tester...
SET FUZZ_DEFINES=-DBUILDING_DLL=1
%GCC% %CFLAGS% %CLFLAGS% %FUZZ_DEFINES% -c SRC\FUZZ.C -o OBJ\FUZZ.OBJ
%GCC% -m32 -static-libgcc -Wl,--allow-multiple-definition -o DIST\fuzz.exe OBJ\FUZZ.OBJ OBJ\ALVIS.OBJ OBJ\COMMON.OBJ OBJ\BEAT.OBJ OBJ\FFT.OBJ OBJ\VIS_WAVE.OBJ OBJ\VIS_SPEC.OBJ OBJ\VIS_VU.OBJ OBJ\VIS_GRND.OBJ OBJ\VIS_BOOM.OBJ %LIBS% -lalleg -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lole32 -ldinput -lddraw -ldxguid -lwinmm -ldsound -lm
IF EXIST DIST\fuzz.exe (
    ECHO Running fuzz tests...
    DIST\fuzz.exe
    IF ERRORLEVEL 1 ECHO Fuzz tests FAILED && EXIT /B 1
    ECHO Fuzz tests passed.
) ELSE (
    ECHO Fuzz tester build failed.
)

SET UPXE="%EXTDEV%\UPX-5.0.1-Win32\upx.exe"
IF NOT "%DEBUG%"=="1" IF EXIST %UPXE% (
    %UPXE% --best DIST\vis_alvis.dll >NUL 2>NUL
    IF ERRORLEVEL 1 ECHO UPX compression failed - skipping. ELSE ECHO UPX compressed vis_alvis.dll
) ELSE (
    IF NOT "%DEBUG%"=="1" ECHO UPX not found - skipping.
)

SET PF86="%ProgramFiles(x86)%"
SET MAKENSIS=makensis
IF EXIST "%PF86%\NSIS\makensis.exe" SET MAKENSIS="%PF86%\NSIS\makensis.exe"
IF EXIST "%ProgramFiles%\NSIS\makensis.exe" SET MAKENSIS="%ProgramFiles%\NSIS\makensis.exe"
IF NOT DEFINED MAKENSIS ECHO NSIS not found, skipping installer. && VER>NUL && EXIT /B 0

IF NOT "%STATIC%"=="1" COPY /Y "%EXTDEV%\Allegro-MSVC8-MinGW-BCC\Bin\x86\alleg42.dll" DIST\alleg42.dll >NUL

IF NOT "%DEBUG%"=="1" IF NOT "%STATIC%"=="1" IF EXIST DIST\alleg42.dll IF EXIST %UPXE% (
    %UPXE% --best DIST\alleg42.dll >NUL 2>NUL
    IF ERRORLEVEL 1 ECHO UPX alleg42.dll failed - skipping. ELSE ECHO UPX compressed alleg42.dll
)

ECHO Building installer...
SET NSISFLAGS=
IF "%STATIC%"=="1" SET NSISFLAGS=/DSTATIC
%MAKENSIS% %NSISFLAGS% INSTALL.NSI >NUL
IF EXIST DIST\alvis.exe ECHO SUCCESS: alvis.exe installer built.

EXIT /B 0
