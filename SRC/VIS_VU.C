#include "vis_mod.h"

int visVuRender(struct winampVisModule *thisMod) {
    int displayCh = thisMod->nCh;
    if (displayCh < 1) return 1;
    if (displayCh > MAX_DISPLAY_CH) displayCh = MAX_DISPLAY_CH;
    if (allegroError || !allegroInitialized) return 1;
    int textColor = 0;
    if (showStats) { updateSongData(thisMod->hwndParent); textColor = rand() % 256; }
    if (usedBuffer) clear_bitmap(usedBuffer);

    int x, ch;
    for (ch = 0; ch < displayCh; ch++) {
        int total = 0;
        int lastSample = thisMod->waveformData[ch][0];
        for (x = 1; x < DISPLAY_SAMPLES; x++) {
            int diff = lastSample - thisMod->waveformData[ch][x];
            total += diff < 0 ? -diff : diff;
        }
        total /= DISPLAY_SAMPLES;

        if (usedBuffer) {
            if (ch == 1) {
                int remaining = total > (windowWidth / 2 - 1) ? 0 : (windowWidth / 2 - 1 - total);
                rectfill(usedBuffer, windowWidth / 2, 0, windowWidth / 2 + remaining, windowHeight, makecol(255, 255, 255));
                int barH = (total * windowHeight) / windowWidth;
                int r = barH > (windowHeight / 2 - 1) ? 0 : (windowHeight / 2 - 1 - barH);
                line(usedBuffer, 0, windowHeight / 2 + r, windowWidth, windowHeight / 2 + r, makecol(191, 191, 191));
            }
            else
            {
                int remaining = total > (windowWidth / 2 - 1) ? 0 : (windowWidth / 2 - 1 - total);
                rectfill(usedBuffer, 0, 0, windowWidth / 2 - remaining - 1, windowHeight, makecol(191, 191, 191));
                int barH = (total * windowHeight) / windowWidth;
                int r = barH > (windowHeight / 2 - 1) ? 0 : (windowHeight / 2 - 1 - barH);
                line(usedBuffer, 0, windowHeight / 2 - r - 1, windowWidth, windowHeight / 2 - r - 1, makecol(255, 255, 255));
            }
        }
    }

    if (displayCh == 1) renderMonoLabel(textColor, textColor, textColor);
    endFrame(textColor, textColor, textColor);
    return 0;
}

__attribute__((constructor)) static void registerVu() {
    static const VisModuleDef moduleDef = {
        .name = "VU Analyser",
        .latencyMs = 75, .delayMs = 25, .spectrumNumCh = chNone, .waveformNumCh = chStereo,
        .width = 320, .height = 50, .bitsPerPixel = bppPaletted, .scrollLength = 24,
        .render = visVuRender
    };
    registerModule(&moduleDef);
}
