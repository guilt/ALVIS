#include "vis_mod.h"

int visWaveRender(struct winampVisModule *thisMod) {
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
            int yCenter = windowHeight / 2;
            int offset = ch == 0 ? -1 : 0;
            int sample = thisMod->waveformData[ch][x] ^ 128;
            int y = yCenter + offset + (ch == 0 ? -sample : sample);
            int r = ch == 0 ? 0 : 255;
            int b = ch == 0 ? 255 : 0;
            if (x) {
                int prevSample = thisMod->waveformData[ch][x-1] ^ 128;
                int prevY = yCenter + offset + (ch == 0 ? -prevSample : prevSample);
                line(usedBuffer, x, y, x-1, prevY, makecol(r, 0, b));
            }
            else
            {
                putpixel(usedBuffer, x, y, makecol(r, 0, b));
            }
        }
    }

    if (displayCh == 1) renderMonoLabel(0, 255, 0);
    endFrame(0, 255, 0);
    return 0;
}

__attribute__((constructor)) static void registerWaveForm() {
    static const VisModuleDef moduleDef = {
        .name = "WaveForm Analyser",
        .latencyMs = 75, .delayMs = 25, .spectrumNumCh = chNone, .waveformNumCh = chStereo,
        .width = 288, .height = 320, .bitsPerPixel = bppPaletted, .scrollLength = 18,
        .render = visWaveRender
    };
    registerModule(&moduleDef);
}
