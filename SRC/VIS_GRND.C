#include "vis_mod.h"

int visGrindRender(struct winampVisModule *thisMod) {
    int displayCh = thisMod->nCh;
    if (displayCh < 1) return 1;
    if (displayCh > MAX_DISPLAY_CH) displayCh = MAX_DISPLAY_CH;
    if (allegroError || !allegroInitialized) return 1;
    if (showStats) updateSongData(thisMod->hwndParent);
    if (usedBuffer) clear_bitmap(usedBuffer);

    analyze(displayCh, thisMod->waveformData);
    int ch;
    for (ch = 0; ch < displayCh; ch++)
        if (getBeat(ch)) frameCounter[ch]++;

    for (ch = 0; ch < displayCh; ch++) {
        int x;
        for (x = DISPLAY_SAMPLES - 1; x >= 0; x--) {
            int grayValue = 64 + abs((int)thisMod->waveformData[ch][x] - 128);
            int color = makecol(grayValue, grayValue, grayValue);
            if (usedBuffer) {
                int cx = ch ? (int)(3 * windowWidth / 4) : (int)(windowWidth / 4);
                int cy;
                if (frameCounter[ch] % 2)
                    cy = ch ? (int)(3 * windowHeight / 4) : (int)(windowHeight / 4);
                else
                    cy = ch ? (int)(windowHeight / 4) : (int)(3 * windowHeight / 4);
                ellipsefill(usedBuffer, cx, cy,
                    (int)((x * windowWidth) / (4 * DISPLAY_SAMPLES)),
                    (int)((x * windowHeight) / (4 * DISPLAY_SAMPLES)), color);
            }
        }
    }

    if (displayCh == 1) renderMonoLabel(255, 191, 127);
    endFrame(255, 191, 127);
    return 0;
}

__attribute__((constructor)) static void registerGrind() {
    static const VisModuleDef moduleDef = {
        .name = "The Grind",
        .latencyMs = 105, .delayMs = 5, .spectrumNumCh = chNone, .waveformNumCh = chStereo,
        .width = 240, .height = 240, .bitsPerPixel = bpp16, .scrollLength = 12,
        .render = visGrindRender
    };
    registerModule(&moduleDef);
}
