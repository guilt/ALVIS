#ifndef VIS_MOD_H
#define VIS_MOD_H

#include "allegro.h"
#include "winalleg.h"
#include "vis.h"
#include "decl.h"
#include "beat.h"

void endFrame(int r, int g, int b);
void renderMonoLabel(int r, int g, int b);
void doDraw(HDC hdc);
void updateSongData(HWND hWnd);
void updateUnformattedSongData(HWND hWnd);
void makeScrollText(char source[SZLEN], char destination[SZLEN], int scrollPosition, int windowSize);
void stripDirectory(char path[MAX_PATH]);

int getBoomBoxEnabled();
int getBoomBoxMode();
void setBoomBoxEnabled(int v);
void setBoomBoxMode(int v);

extern int fullyExited;
extern int allegroInitialized, allegroError;
extern BITMAP *usedBuffer;

extern char originalSongTitle[SZLEN], currentSongTitle[SZLEN];
extern char positionText[SZLEN], lengthText[SZLEN], displayString[3*SZLEN];
extern char separatorFormat[SZLEN];
extern float songPositionSeconds, songLengthSeconds;
extern int scrollLength;

extern int windowWidth, windowHeight;

extern char titleText[];
extern int showStats;
extern int frameCounter[MAX_DISPLAY_CH];
extern HWND mainWindowHandle;

typedef int (*visRenderFunc)(struct winampVisModule *thisMod);

enum {
    chNone = 0,
    chStereo = 2,
};

enum {
    bppPaletted = 0,
    bpp16 = 16,
    bpp32 = 32,
};

typedef void (*visConfigReadFn)(struct winampVisModule *m, const char *file);
typedef void (*visConfigWriteFn)(struct winampVisModule *m, const char *file);
typedef void (*visInitFn)(struct winampVisModule *m);
typedef void (*visQuitFn)();
typedef void (*visInputFn)(struct winampVisModule *m, int key);

typedef struct {
    char *name;
    int latencyMs;
    int delayMs;
    int spectrumNumCh;
    int waveformNumCh;
    int width;
    int height;
    int bitsPerPixel;
    int scrollLength;
    visRenderFunc render;
    visInitFn initModule;
    visQuitFn quitModule;
    visInputFn inputHandler;
    visConfigReadFn readConfig;
    visConfigWriteFn writeConfig;
} VisModuleDef;

void registerModule(const VisModuleDef *def);

#endif
