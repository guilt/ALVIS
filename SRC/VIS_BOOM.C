#include <stdio.h>
#include <math.h>
#include "vis_mod.h"

static int boomBoxEnabled = 1;
static int boomBoxMode = 1;
static BITMAP *boomBoxBitmap = NULL;
static char boomBoxFilePath[MAX_PATH];
static float cosTable[DISPLAY_SAMPLES];
static float sinTable[DISPLAY_SAMPLES];

__attribute__((constructor)) static void initTrigTable() {
    int x;
    for (x = 0; x < DISPLAY_SAMPLES; x++) {
        float angle = (x * 2 * PI) / DISPLAY_SAMPLES;
        cosTable[x] = cosf(angle);
        sinTable[x] = sinf(angle);
    }
}

int getBoomBoxEnabled() { return boomBoxEnabled; }
int getBoomBoxMode() { return boomBoxMode; }
void setBoomBoxEnabled(int v) { boomBoxEnabled = v; }
void setBoomBoxMode(int v) { boomBoxMode = v; }
BITMAP *getBoomBoxBitmap() { return boomBoxBitmap; }

static void drawBoomSpeaker(BITMAP *buf, int cx, int cy, int offset, unsigned char *data, int frameParity) {
    int x;
    for (x = 0; x < DISPLAY_SAMPLES; x++) {
        int px = x > 0 ? x - 1 : DISPLAY_SAMPLES - 1;
        float angleC = cosTable[x], angleS = sinTable[x];
        float prevC = cosTable[px], prevS = sinTable[px];
        int sample0 = data[x] ^ 128;
        int sample1 = (x == 0) ? (sample0 + (int)(data[px] ^ 128)) / 2 : (int)(data[px] ^ 128);

        {
            int ringStep;
            for (ringStep = 1; ringStep < 5; ringStep++) {
                float scale = (ringStep * 0.5f) / 5.0f;
                int r1 = (int)(scale * sample0);
                int r2 = (int)(scale * sample1);
                line(buf,
                    cx + (int)(r1 * angleC), cy + (int)(r1 * angleS),
                    cx + (int)(r2 * prevC), cy + (int)(r2 * prevS),
                    makecol(200 + ringStep * 8, 100 + ringStep * 2, 50 + ringStep));
            }
        }
    }
}

static void drawBoomCircles(BITMAP *buf, int cx, int cy, unsigned char *specData) {
    int x;
    for (x = 0; x < DISPLAY_SAMPLES; x++) {
        int radius = (int)(0.3f * (specData[x] ^ 64));
        int redTint   = 50 + (x * 200) / DISPLAY_SAMPLES;
        int greenTint = 50 + (x * 75) / DISPLAY_SAMPLES;
        circle(buf, cx, cy, radius, makecol(redTint, greenTint, 0));
    }
}

int visBoomRender(struct winampVisModule *thisMod) {
    char scrolledTitle[SZLEN];
    int offset = 10;

    int displayCh = thisMod->nCh;
    if (displayCh < 1) return 1;
    if (displayCh > MAX_DISPLAY_CH) displayCh = MAX_DISPLAY_CH;

    if (allegroError || !allegroInitialized) return 1;
    if (showStats) updateUnformattedSongData(thisMod->hwndParent);

    analyze(displayCh, thisMod->waveformData);
    if (displayCh > 1 && getBeat(1)) frameCounter[1]++;
    if (getBeat(0)) frameCounter[0]++;

    if (usedBuffer) clear_bitmap(usedBuffer);

    if (boomBoxEnabled && boomBoxBitmap && usedBuffer)
        blit(boomBoxBitmap, usedBuffer, 0, 0,
            (usedBuffer->w - boomBoxBitmap->w) / 2,
            (usedBuffer->h - boomBoxBitmap->h) / 2,
            usedBuffer->w, usedBuffer->h);

    if (boomBoxEnabled && usedBuffer)
        textout_ex(usedBuffer, font, boomBoxMode ? "Waveform" : "Spectrum", 200, 10, makecol(200, 200, 0), -1);

    if (boomBoxMode) {
        int ch;
        for (ch = 0; ch < displayCh; ch++) {
            if (!usedBuffer) break;
            int speakerX = ch ? (3 * windowWidth / 4) - offset : windowWidth / 4 + offset;
            drawBoomSpeaker(usedBuffer, speakerX, windowHeight / 3, offset,
                thisMod->waveformData[ch], frameCounter[ch]);
        }
    } else {
        int ch;
        for (ch = 0; ch < displayCh; ch++) {
            if (!usedBuffer) break;
            int speakerX = ch ? (3 * windowWidth / 4) - offset : windowWidth / 4 + offset;
            drawBoomCircles(usedBuffer, speakerX, windowHeight / 3, thisMod->spectrumData[ch]);
        }
    }

    if (boomBoxEnabled && usedBuffer && boomBoxBitmap && displayCh > 1) {
        int ch;
        for (ch = 0; ch < displayCh; ch++) {
            int xPos = ch ? 36 : 34;
            int yPos = ch ? 233 : 210;
        int bright = frameCounter[ch] % 2 ? 255 : (ch ? 64 : 64);
        ellipsefill(usedBuffer, xPos, yPos, 5, 7,
            makecol(ch ? 0 : bright, ch ? bright : 0, 0));
        }
    }

    if (displayCh == 1 && showStats && usedBuffer)
        textout_ex(usedBuffer, font, "MONO", 205, 225, makecol(255, 63, 0), -1);

    if (showStats) {
        if (*currentSongTitle && usedBuffer) {
            makeScrollText(originalSongTitle, scrolledTitle,
                frameCounter[0] + frameCounter[1], scrollLength);
            textout_ex(usedBuffer, font, scrolledTitle, 75, 210, makecol(0, 191, 63), -1);
        }

        if (*positionText) {
            if (*lengthText)
                sprintf(displayString, "[%s/%s]", positionText, lengthText);
            else
                sprintf(displayString, "[%s]", positionText);
        }
        else
        {
            *displayString = 0;
        }

        if (*displayString && usedBuffer)
            textout_ex(usedBuffer, font, displayString, 92, 225, makecol(0, 191, 63), -1);
    }

    if (mainWindowHandle) {
        HDC hdc = GetDC(mainWindowHandle);
        doDraw(hdc);
        ReleaseDC(mainWindowHandle, hdc);
    }
    return 0;
}

void visBoomInput(struct winampVisModule *m, int key) {
    switch (key) {
        case 'w': case 'W': setBoomBoxMode(1 - getBoomBoxMode()); break;
        case 't': case 'T': setBoomBoxEnabled(1 - getBoomBoxEnabled()); break;
    }
}

void visBoomReadConfig(struct winampVisModule *m, const char *file) {
    boomBoxMode = GetPrivateProfileInt(m->description, "BBMode", boomBoxMode, file);
    boomBoxEnabled = GetPrivateProfileInt(m->description, "BG", boomBoxEnabled, file);
}

void visBoomWriteConfig(struct winampVisModule *m, const char *file) {
    char buf[32];
    wsprintf(buf, "%d", boomBoxMode);
    WritePrivateProfileString(m->description, "BBMode", buf, file);
    wsprintf(buf, "%d", boomBoxEnabled);
    WritePrivateProfileString(m->description, "BG", buf, file);
}

void visBoomInit(struct winampVisModule *m) {
    GetModuleFileName(m->hDllInstance, boomBoxFilePath, MAX_PATH);
    stripDirectory(boomBoxFilePath);
    strcat(boomBoxFilePath, "boom.bmp");
    boomBoxBitmap = load_bitmap(boomBoxFilePath, NULL);
}

void visBoomQuit() {
    if (boomBoxBitmap) destroy_bitmap(boomBoxBitmap);
    boomBoxBitmap = NULL;
}

__attribute__((constructor)) static void registerBoomBox() {
    static const VisModuleDef moduleDef = {
        .name = "Boom Box - Explosion",
        .latencyMs = 105, .delayMs = 5, .spectrumNumCh = chStereo, .waveformNumCh = chStereo,
        .width = 280, .height = 250, .bitsPerPixel = bpp16, .scrollLength = 20,
        .render = visBoomRender,
        .initModule = visBoomInit, .quitModule = visBoomQuit,
        .inputHandler = visBoomInput,
        .readConfig = visBoomReadConfig, .writeConfig = visBoomWriteConfig
    };
    registerModule(&moduleDef);
}
