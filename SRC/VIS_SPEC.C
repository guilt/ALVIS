#include "vis_mod.h"

int visSpectrumRender(struct winampVisModule *thisMod) {
    int displayCh = thisMod->nCh;
    if (displayCh < 1) return 1;
    if (displayCh > MAX_DISPLAY_CH) displayCh = MAX_DISPLAY_CH;
    if (allegroError || !allegroInitialized) return 1;
    if (showStats) updateSongData(thisMod->hwndParent);
    if (usedBuffer) clear_bitmap(usedBuffer);

    int x, ch;
    for (ch = 0; ch < displayCh; ch++) {
        for (x = 0; x < DISPLAY_SAMPLES; x++) {
            if (!usedBuffer) continue;
            int yBase = windowHeight / 2;
            int h = thisMod->spectrumData[ch][x];
            if (ch == 1)
                line(usedBuffer, x, yBase, x, yBase + h, makecol(255, 255, 0));
            else
                line(usedBuffer, x, yBase - 1, x, yBase - 1 - h, makecol(0, 255, 255));
        }
    }

    if (displayCh == 1) renderMonoLabel(255, 0, 255);
    endFrame(255, 0, 255);
    return 0;
}

__attribute__((constructor)) static void registerSpectrum() {
    static const VisModuleDef moduleDef = {
        .name = "Spectrum Analyser",
        .latencyMs = 175, .delayMs = 25, .spectrumNumCh = chStereo, .waveformNumCh = chNone,
        .width = 288, .height = 256, .bitsPerPixel = bppPaletted, .scrollLength = 18,
        .render = visSpectrumRender
    };
    registerModule(&moduleDef);
}
