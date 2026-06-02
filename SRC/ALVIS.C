// ALVIS - Allegro Visualization Plugin
//
// With thanks to :
// Justin Frankel, Ryan Geiss and Francis Gastellu / Nullsoft.
// all Allegro programmers.
//
// Programmed by Karthik Kumar Viswanathan (@guilt) for use with Allegro
// Co-Programmed by Nikhil Karnad (scribbleink)
//
// Added support for WA5. WA5 Rocks.

#include <math.h>
#include <stdio.h>
#include "allegro.h"
#include "winalleg.h"
#include "wa_ipc.h"
#include "vis.h"
#include "decl.h"
#include "ver.h"

#define BOOM_BRUSH BLACK_BRUSH

#if defined(ALLEGRO_STATICLINK) && !defined(ALLEGRO_MSVC)
BEGIN_GFX_DRIVER_LIST
END_GFX_DRIVER_LIST
BEGIN_JOYSTICK_DRIVER_LIST
END_JOYSTICK_DRIVER_LIST
BEGIN_MIDI_DRIVER_LIST
END_MIDI_DRIVER_LIST
BEGIN_DIGI_DRIVER_LIST
END_DIGI_DRIVER_LIST
#endif

int fullyExited=1;
int allegroInitialized=0,allegroError=0;
static int bitsPerPixel=0;
int frameCounter[MAX_DISPLAY_CH];

BITMAP *usedBuffer=NULL;

char originalSongTitle[SZLEN],currentSongTitle[SZLEN],
     positionText[SZLEN],lengthText[SZLEN],displayString[3*SZLEN];
char separatorFormat[SZLEN]={" *** "};
float songPositionSeconds,songLengthSeconds;
int scrollLength=0;

int windowWidth=0,windowHeight=0;

static char iniFileName[]="alvis.ini";


char titleText[] = VProductName;
static char notAvailableText[] = "Feature not available";
static char aboutText[] =
"Copyright \xA9 " VVersionOldYear "-" VVersionYear " " VAuthor1 " & " VAuthor2 "\n"
"  By " VAuthor1 " (" VAuthor1_GitHub ") & " VAuthor2 " (" VAuthor2_GitHub ")\n"
"\n"
"  Based on:\n"
"    Vis_MiniSDK v1.01  - Justin Frankel\n"
"    Vis_MegaSDK v1.03  - Ryan Geiss\n"
"    wa502_sdk          - Francis Gastellu\n"
"    Allegro v" ALLEGRO_VERSION_STR "\n"
#ifndef ALLEGRO_STATICLINK
"    (requires Allegro v" ALLEGRO_VERSION_STR " runtime)\n"
#endif
;
static char appName[] = "AlVis";

static int configX=0, configY=0;
int showStats=1;


static winampVisModule *getModule(int which);

static embedWindowState embedState;
static int isWA5;

void config(struct winampVisModule *thisMod);
int  init(struct winampVisModule *thisMod);
void quit(struct winampVisModule *thisMod);

int visWaveRender(struct winampVisModule*);
int visSpectrumRender(struct winampVisModule*);
int visVuRender(struct winampVisModule*);
int visGrindRender(struct winampVisModule*);
int visBoomRender(struct winampVisModule*);
void visBoomReadConfig(struct winampVisModule*, const char*);
void visBoomWriteConfig(struct winampVisModule*, const char*);
void visBoomInit(struct winampVisModule*);
void visBoomQuit();
int getBoomBoxMode();
int getBoomBoxEnabled();
void setBoomBoxMode(int);
void setBoomBoxEnabled(int);
void visBoomInput(struct winampVisModule*, int);
void stripDirectory(char path[MAX_PATH]);
int waitUntilPluginFinished();

static void clampConfiguration();
static void resetFrameCounters();
static void allegroQuit();
static int allegroInit(int, int, int);
static void getIniFilePath(struct winampVisModule*, char[MAX_PATH]);
static void readConfiguration(struct winampVisModule*);
static void writeConfiguration(struct winampVisModule*);

int frameCounter[MAX_DISPLAY_CH];

#include "vis_mod.h"
#include "beat.h"

#define MAX_VIS_MODULES 16
static const VisModuleDef *visDefs[MAX_VIS_MODULES];
static winampVisModule dynMods[MAX_VIS_MODULES];
static int modCount = 0;

void registerModule(const VisModuleDef *def) {
    if (!def || !def->name || !def->render) return;
    if (modCount >= MAX_VIS_MODULES) return;
    int i = modCount++;
    visDefs[i] = def;
    visDefs[i] = def;
    dynMods[i].description = def->name;
    dynMods[i].hwndParent = NULL;
    dynMods[i].hDllInstance = NULL;
    dynMods[i].sRate = 0;
    dynMods[i].nCh = 0;
    dynMods[i].latencyMs = def->latencyMs;
    dynMods[i].delayMs = def->delayMs;
    dynMods[i].spectrumNch = def->spectrumNumCh;
    dynMods[i].waveformNch = def->waveformNumCh;
    dynMods[i].Config = config;
    dynMods[i].Init = init;
    dynMods[i].Render = def->render;
    dynMods[i].Quit = quit;
    dynMods[i].userData = (void*)(intptr_t)i;
}

static winampVisModule *getModule(int which) {
    if (which < 0 || which >= modCount) return NULL;
    return &dynMods[which];
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND mainWindowHandle;

static winampVisHeader moduleHeader = { VIS_HDRVER, titleText, getModule };

#ifdef __cplusplus
extern "C" {
#endif
__declspec( dllexport ) winampVisHeader *winampVisGetHeader() {
    return &moduleHeader;
}
#ifdef __cplusplus
}
#endif

static void getIniFilePath(struct winampVisModule *thisMod, char iniFile[MAX_PATH]) {
    GetModuleFileName(thisMod->hDllInstance,iniFile,MAX_PATH);
    stripDirectory(iniFile);
    strcat(iniFile,iniFileName);
}

static void readConfiguration(struct winampVisModule *thisMod) {
    char configFile[MAX_PATH];
    getIniFilePath(thisMod,configFile);
    configX = GetPrivateProfileInt(thisMod->description,"Screen_x",configX,configFile);
    configY = GetPrivateProfileInt(thisMod->description,"Screen_y",configY,configFile);
    showStats = GetPrivateProfileInt(thisMod->description,"OSD",showStats,configFile);
    int i; for (i = 0; i < modCount; i++)
        if (thisMod == &dynMods[i] && visDefs[i]->readConfig)
            visDefs[i]->readConfig(thisMod, configFile);
    clampConfiguration();
}

static void writeIntConfig(struct winampVisModule *m, const char *key, int value, const char *file) {
    char buf[32];
    wsprintf(buf, "%d", value);
    WritePrivateProfileString(m->description, key, buf, file);
}

static void writeConfiguration(struct winampVisModule *thisMod) {
    char configFile[MAX_PATH];
    getIniFilePath(thisMod,configFile);
    clampConfiguration();

    writeIntConfig(thisMod, "Screen_x", configX, configFile);
    writeIntConfig(thisMod, "Screen_y", configY, configFile);
    writeIntConfig(thisMod, "OSD", showStats, configFile);
    int i; for (i = 0; i < modCount; i++)
        if (thisMod == &dynMods[i] && visDefs[i]->writeConfig)
            visDefs[i]->writeConfig(thisMod, configFile);
}


// configuration dialog - shared by all 5 modules
void config(struct winampVisModule *thisMod) {
    if (!waitUntilPluginFinished())return;
    fullyExited = 0;
    MessageBox(thisMod->hwndParent,aboutText,titleText,MB_OK);
    fullyExited = 1;
}

static void showFeatureNotAvailable() {
    MessageBox(NULL,notAvailableText,titleText,MB_OK);
}

void endFrame(int r, int g, int b) {
    if(*displayString && usedBuffer && showStats)
        textout_ex(usedBuffer, font, displayString, 0, 0, makecol(r,g,b),-1);
    if(mainWindowHandle) {
        HDC hdc = GetDC(mainWindowHandle);
        doDraw(hdc);
        ReleaseDC(mainWindowHandle,hdc);
    }
}

void renderMonoLabel(int r, int g, int b) {
    if(usedBuffer && showStats)
        textout_centre_ex(usedBuffer, font, "Mono", windowWidth/2, windowHeight/2, makecol(r,g,b),-1);
}

void stripDirectory(char path[MAX_PATH]) {
    char *p = path + strlen(path);
    while (p > path && *p != '\\') p--;
    if (*p == '\\') p++;
    *p = 0;
}

static void clampConfiguration() {
    if(configX<0||configX>MAX_CONFIG_X) configX=0;
    if(configY<0||configY>MAX_CONFIG_Y) configY=0;
    if(showStats<0||showStats>1) showStats=1;
    int bm = getBoomBoxMode(), be = getBoomBoxEnabled();
    if (bm < 0 || bm > 1) setBoomBoxMode(1);
    if (be < 0 || be > 1) setBoomBoxEnabled(1);
}

static void resetFrameCounters() {
    int i;
    for (i = 0; i < MAX_DISPLAY_CH; i++) frameCounter[i] = 0;
}


static BITMAP *letterboxBuffer = NULL;
static int letterboxWidth = 0, letterboxHeight = 0;

void doDraw(HDC hdc) {
    if (allegroError || !allegroInitialized || !usedBuffer) return;
    if (!hdc) return;

    RECT clientRect;
    GetClientRect(mainWindowHandle, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    if (clientWidth < 1 || clientHeight < 1) return;

    if (!letterboxBuffer || letterboxWidth != clientWidth || letterboxHeight != clientHeight) {
        if (letterboxBuffer) destroy_bitmap(letterboxBuffer);
        letterboxBuffer = create_bitmap(clientWidth, clientHeight);
        letterboxWidth = clientWidth;
        letterboxHeight = clientHeight;
    }

    clear_bitmap(letterboxBuffer);

    float srcAspect = (float)windowWidth / (float)windowHeight;
    float dstAspect = (float)clientWidth / (float)clientHeight;
    int destX, destY, destW, destH;

    if (dstAspect > srcAspect) {
        destH = clientHeight;
        destW = (int)(clientHeight * srcAspect);
        destX = (clientWidth - destW) / 2;
        destY = 0;
    }
    else
    {
        destW = clientWidth;
        destH = (int)(clientWidth / srcAspect);
        destX = 0;
        destY = (clientHeight - destH) / 2;
    }

    if (destW > 0 && destH > 0)
        stretch_blit(usedBuffer, letterboxBuffer, 0, 0, windowWidth, windowHeight, destX, destY, destW, destH);

    set_palette_to_hdc(hdc, _current_palette);
    draw_to_hdc(hdc, letterboxBuffer, 0, 0);
}

static void allegroQuit() {
    if (letterboxBuffer) { destroy_bitmap(letterboxBuffer); letterboxBuffer = NULL; }
    if(allegroInitialized) {
    if(!allegroError) {
        if(usedBuffer) { destroy_bitmap(usedBuffer); usedBuffer = NULL; }
        unselect_palette();
        allegro_exit();
        allegroInitialized=0;
    }
    else
    {
        allegroError=0;
    }
    allegroInitialized=0;
    }
}

// Allegro initialization.
static int allegroInit(int width, int height,int bpp) {

    // quit .
    if(allegroInitialized) {
        allegroQuit();
        allegroInitialized=0; 
    }

    allegroError=0;

    #ifdef ALLEGRO_STATICLINK
    if(install_allegro(SYSTEM_NONE,&errno,NULL))
    #else
    if(install_allegro(SYSTEM_AUTODETECT,&errno,NULL))
    #endif
    {
        allegroError=1;
        return 1;
    }

    allegroInitialized=1;
    
    if(bpp>0) {
        set_color_depth(bpp);   
    }

    set_gdi_color_format();

    if(bpp<=0) select_palette(default_palette);

    set_window_title(titleText);

    usedBuffer=create_bitmap(width,height);

    if(usedBuffer) {
        clear_bitmap(usedBuffer);
    }
    else 
    {
        allegroQuit();
        return 1;
    }

    return 0;
}

void quit(struct winampVisModule *thisMod) {
    clampConfiguration();
    writeConfiguration(thisMod);
    if(isWA5) SendMessage(thisMod->hwndParent, WM_WA_IPC, 0, IPC_SETVISWND);
    resetFrameCounters();
    { int i; for (i = 0; i < modCount; i++) if (visDefs[i]->quitModule) visDefs[i]->quitModule(); }
    beatDetectionQuit();
    allegroQuit();
    if (isWA5&&embedState.me) {
    if(thisMod&&thisMod->hwndParent) SetForegroundWindow(thisMod->hwndParent);
    if(embedState.me) DestroyWindow(embedState.me);
    }
    else
    {
    if(mainWindowHandle) DestroyWindow(mainWindowHandle);
    }
    UnregisterClass(appName,thisMod->hDllInstance);
    fullyExited = 1;
    isWA5=0;
}

int init(struct winampVisModule *thisMod) {
    int width;
    int height;
    int bpp;
    int style=WS_VISIBLE|WS_CHILDWINDOW|
        WS_OVERLAPPED|WS_CLIPCHILDREN|
        WS_CLIPSIBLINGS,
        windowExStyle;
    HWND (*e)(embedWindowState *v);
    HWND parent=NULL;

    if (!waitUntilPluginFinished()) return 1;

    beatDetectionQuit();
    allegroQuit();
    
    {
        int idx = (int)(intptr_t)thisMod->userData;
        if (idx >= 0 && idx < modCount) {
            scrollLength = visDefs[idx]->scrollLength;
            windowWidth = visDefs[idx]->width;
            windowHeight = visDefs[idx]->height;
            bitsPerPixel = visDefs[idx]->bitsPerPixel;
            if (idx == 2) srand(time(NULL)); // VU uses random colors
        }
        else
        {
            scrollLength = windowWidth = windowHeight = bitsPerPixel = 0;
        }
    }

    width = windowWidth;
    height = windowHeight;
    bpp=bitsPerPixel;

    readConfiguration(thisMod);
    clampConfiguration();
    resetFrameCounters();
    fullyExited = 0;
    {
        WNDCLASS wc;
        isWA5=1;

        if (isWA5&&embedState.me) {
          if(thisMod&&thisMod->hwndParent) SetForegroundWindow(thisMod->hwndParent);
          if(embedState.me) DestroyWindow(embedState.me);
        }
        else
        {
        if(mainWindowHandle) DestroyWindow(mainWindowHandle);
        }

        UnregisterClass(appName,thisMod->hDllInstance);

        embedState.r.left = configX;
        embedState.r.top = configY;
        embedState.r.right = configX + width;
        embedState.r.bottom = configY + height;

        *(void**)&e = (void *)SendMessage(thisMod->hwndParent,WM_WA_IPC,(LPARAM)0,IPC_GET_EMBEDIF);
        if (!e) {
        isWA5=0;
        parent = thisMod->hwndParent;
        style=WS_VISIBLE|WS_SYSMENU;
        }
        else
        {
            parent = e(&embedState);
        }

        if(isWA5)
            SetWindowText(embedState.me, thisMod->description);

        memset(&wc,0,sizeof(wc));
        wc.lpfnWndProc = WndProc;
        wc.hInstance = thisMod->hDllInstance;
        wc.lpszClassName = appName;

        if (!RegisterClass(&wc)) {
            MessageBox(thisMod->hwndParent,"Error registering Window Class",titleText,MB_OK);
            fullyExited = 1;
            return 1;
        }
    }

    if(isWA5)
    windowExStyle=0;
    else 
    windowExStyle=WS_EX_TOOLWINDOW|WS_EX_APPWINDOW;

    mainWindowHandle = CreateWindowEx(
        windowExStyle, appName, thisMod->description,
        style, configX, configY, width, height,
        parent, NULL, thisMod->hDllInstance, 0);

    if (!mainWindowHandle) {
        MessageBox(thisMod->hwndParent,"Error creating Window",titleText,MB_OK);
        fullyExited = 1;
        return 1;
    }

    SetWindowLong(mainWindowHandle,GWL_USERDATA,(LONG)thisMod);

    if(!isWA5) {
        RECT r;
        int widthPatch,heightPatch;
        GetClientRect(mainWindowHandle,&r);
        widthPatch=(width*2)-r.right;if(widthPatch<0) {widthPatch=r.left;}if(widthPatch<0) {widthPatch=0;}
        heightPatch=(height*2)-r.bottom;if(heightPatch<0) {heightPatch=r.top;}if(heightPatch<0) {heightPatch=0;}
        SetWindowPos(mainWindowHandle,0,0,0,widthPatch,heightPatch,SWP_NOMOVE|SWP_NOZORDER);
    }

    if(allegroInit(width,height,bpp)) {
        MessageBox(thisMod->hwndParent,"Error Initializing Allegro",titleText,MB_OK);
        if (isWA5&&embedState.me) DestroyWindow(embedState.me);
        else if(mainWindowHandle) DestroyWindow(mainWindowHandle);
        UnregisterClass(appName,thisMod->hDllInstance);
        fullyExited = 1;
        return 1;
    }

    { int i; for (i = 0; i < modCount; i++)
        if (thisMod == &dynMods[i] && visDefs[i]->initModule)
            visDefs[i]->initModule(thisMod); }

    if(isWA5)
    SendMessage(thisMod->hwndParent, WM_WA_IPC, (int)mainWindowHandle, IPC_SETVISWND);

    ShowWindow(parent, SW_SHOWNORMAL);
    return 0;
}

// window procedure for our window
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:     return 0;
        case WM_ERASEBKGND: return 0;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                if(hwnd) {
                    HDC hdc = BeginPaint(hwnd,&ps);
                    doDraw(hdc);
                    if(isWA5) {
                    RECT r;
                    GetClientRect(hwnd,&r);
                    {
                    RECT x={r.left+windowWidth, r.top, r.right, r.bottom};
                    RECT y={r.left, r.top+windowHeight, r.right, r.bottom};
                    FillRect(hdc, &x, GetStockObject(BOOM_BRUSH));
                    FillRect(hdc, &y, GetStockObject(BOOM_BRUSH));
                    }
                    }
                    EndPaint(hwnd,&ps);
                }
            }
        return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_CHAR:
            {
                winampVisModule *activeMod = (winampVisModule *)GetWindowLong(hwnd, GWL_USERDATA);
                int idx = activeMod ? (int)(intptr_t)activeMod->userData : -1;
                if (idx >= 0 && idx < modCount && visDefs[idx]->inputHandler)
                    visDefs[idx]->inputHandler(activeMod, (int)wParam);
                switch (wParam) {
                case 'y':
                case 'Y': showStats=1-showStats; break;
                }
            }
        case WM_KEYDOWN:
        case WM_KEYUP:
            {
                winampVisModule *thisMod = (winampVisModule *) GetWindowLong(hwnd,GWL_USERDATA);
                PostMessage(thisMod->hwndParent,message,wParam,lParam);
            }
        return 0;
        case WM_MOVE:
        case WM_WINDOWPOSCHANGING:
            {
                RECT r;
                GetWindowRect(mainWindowHandle,&r);

                if(r.left<0) {
                    if((configX=(r.right-(2*windowWidth)))<0)
                    configX=0;
                }
                else {configX=r.left;}

                if(r.top<0) {
                    if((configY=(r.bottom-(2*windowHeight)))<0)
                    configY=0;
                }
                else {configY=r.top;}

                if(configX>MAX_CONFIG_X) {configX=0;}
                if(configY>MAX_CONFIG_Y) {configY=0;}

                if(showStats<0||showStats>1) {showStats=1;}
            }
            return 0;
            case WM_COMMAND:
            {
                int id = LOWORD(wParam);
                if (isWA5) {
                    switch (id) {
                    case ID_VIS_NEXT:
                    case ID_VIS_PREV:
                    case ID_VIS_RANDOM:
                        return 0;
                    case ID_VIS_FS:
                    case ID_VIS_CFG:
                    case ID_VIS_MENU:
                        showFeatureNotAvailable(); return 0;
                    }
                }
            }
     }
    return DefWindowProc(hwnd,message,wParam,lParam);
}
