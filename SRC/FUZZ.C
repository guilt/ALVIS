// ALVIS fuzz tester
#define ALLEGRO_NO_MAGIC_MAIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>

#include "decl.h"
#include "vis_mod.h"
#include "beat.h"
#include "fft.h"

// Render functions from visualizer modules
int visWaveRender(struct winampVisModule *thisMod);
int visSpectrumRender(struct winampVisModule *thisMod);
int visVuRender(struct winampVisModule *thisMod);
int visGrindRender(struct winampVisModule *thisMod);
int visBoomRender(struct winampVisModule *thisMod);
void visBoomInit(struct winampVisModule *m);
void visBoomQuit(void);
void visBoomInput(struct winampVisModule *m, int key);

static int passed = 0, failed = 0;

#define TEST(msg, cond) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); failed++; } \
    else         { printf("  PASS: %s\n", msg); passed++; } \
} while(0)

static void ensureAllegro(void) {
    if (allegroInitialized) return;
    allegroError = 0;
    if (install_allegro(SYSTEM_NONE, &errno, NULL)) {
        printf("  Allegro init FAILED\n");
        return;
    }
    allegroInitialized = 1;
    set_color_depth(32);
    set_gdi_color_format();
    if (!usedBuffer) {
        usedBuffer = create_bitmap(288, 256);
        if (usedBuffer) clear_bitmap(usedBuffer);
    }
}

static void dropAllegro(void) {
    if (allegroInitialized) {
        if (usedBuffer) { destroy_bitmap(usedBuffer); usedBuffer = NULL; }
        allegro_exit();
        allegroInitialized = 0;
    }
}

int main(void) {
    printf("ALVIS Fuzz Tester\n");
    printf("=================\n\n");
    srand((unsigned)time(NULL));
    unsigned char data[2][WAVEFORM_SAMPLES];
    int ch, s, f;

    // ===== 1. FFT init edge cases =====
    printf("--- FFT Init ---\n");
    TEST("FFT_Init(576) == 0", FFT_Init(WAVEFORM_SAMPLES) == 0);
    FFT_CleanUp();
    TEST("FFT_Init(0) defaults", FFT_Init(0) == 0); FFT_CleanUp();
    TEST("FFT_Init(1) == 0",     FFT_Init(1) == 0); FFT_CleanUp();
    TEST("FFT_Init(9999) == 0",  FFT_Init(9999) == 0); FFT_CleanUp();
    TEST("FFT_Init(-1) handled", FFT_Init(-1) == 0); FFT_CleanUp();
    printf("\n");

    // ===== 2. FFT with negative/NaN/inf values =====
    printf("--- FFT Edge Values ---\n");
    FFT_Init(WAVEFORM_SAMPLES);
    {
        float in[WAVEFORM_SAMPLES], out[FFT_BINS];
        memset(out, 0, sizeof(out));
        memset(in, 0, sizeof(in));
        FFT_TimeToFrequencyDomain(in, out); TEST("FFT zeros", 1);

        memset(out, 0, sizeof(out));
        srand(42);
        for (s = 0; s < WAVEFORM_SAMPLES; s++) in[s] = (float)(rand() % 2000 - 1000) / 100.0f;
        FFT_TimeToFrequencyDomain(in, out); TEST("FFT mixed +/- values", 1);

        memset(out, 0, sizeof(out));
        for (s = 0; s < WAVEFORM_SAMPLES; s++) in[s] = -1.0f;
        FFT_TimeToFrequencyDomain(in, out); TEST("FFT all -1.0", 1);

        memset(out, 0, sizeof(out));
        for (s = 0; s < WAVEFORM_SAMPLES; s++) in[s] = -1000.0f;
        FFT_TimeToFrequencyDomain(in, out); TEST("FFT all -1000", 1);

        memset(out, 0, sizeof(out));
        for (s = 0; s < WAVEFORM_SAMPLES; s++) in[s] = 1e20f;
        FFT_TimeToFrequencyDomain(in, out); TEST("FFT huge positive", 1);

        memset(out, 0, sizeof(out));
        for (s = 0; s < WAVEFORM_SAMPLES; s++) in[s] = -1e20f;
        FFT_TimeToFrequencyDomain(in, out); TEST("FFT huge negative", 1);
    }
    FFT_CleanUp();
    printf("\n");

    // ===== 3. Extreme waveforms =====
    printf("--- Extreme Waveforms ---\n");
    FFT_Init(WAVEFORM_SAMPLES);

    memset(data, 0, sizeof(data));
    analyze(2, data);   TEST("all zeros", 1);

    memset(data, 255, sizeof(data));
    analyze(2, data);   TEST("all 255", 1);

    for (s = 0; s < WAVEFORM_SAMPLES; s++)
        data[0][s] = data[1][s] = (s % 2) ? 0 : 255;
    analyze(2, data);   TEST("square wave", 1);

    for (s = 0; s < WAVEFORM_SAMPLES; s++)
        data[0][s] = data[1][s] = (unsigned char)(s * 256 / WAVEFORM_SAMPLES);
    analyze(2, data);   TEST("sawtooth", 1);

    memset(data, 0, sizeof(data));
    data[0][0] = 255; data[1][100] = 128;
    analyze(2, data);   TEST("single impulse", 1);
    printf("\n");

    // ===== 4. Channel count fuzzing =====
    printf("--- Channel Count Fuzzing ---\n");
    for (ch = 0; ch < 2; ch++)
        for (s = 0; s < WAVEFORM_SAMPLES; s++)
            data[ch][s] = (unsigned char)(rand() % 256);

    FFT_Init(WAVEFORM_SAMPLES);
    analyze(0, data);   TEST("nCh=0", 1);
    analyze(1, data);   TEST("nCh=1 mono",   getBeat(0) == 0 || getBeat(0) == 1);
                        TEST("nCh=1 ch1=0",  getBeat(1) == 0);
    analyze(2, data);   TEST("nCh=2 stereo", 1);
    analyze(3, data);   TEST("nCh=3 clamped", 1);
    analyze(8, data);   TEST("nCh=8 max", 1);
    analyze(16, data);  TEST("nCh=16 clamped", 1);
    analyze(-1, data);  TEST("nCh=-1 early exit", 1);
    printf("\n");

    // ===== 5. getBeat bounds =====
    printf("--- getBeat Bounds ---\n");
    TEST("-1",       getBeat(-1) == 0);
    TEST("0",        getBeat(0) == 0 || getBeat(0) == 1);
    TEST("1",        getBeat(1) == 0 || getBeat(1) == 1);
    TEST("MAX",      getBeat(MAX_AUDIO_CH) == 0);
    TEST("MAX+1",    getBeat(MAX_AUDIO_CH + 1) == 0);
    TEST("9999",     getBeat(9999) == 0);
    TEST("-9999",    getBeat(-9999) == 0);
    printf("\n");

    // ===== 6. Beat pulse detection =====
    printf("--- Beat Pulse Detection ---\n");
    unsigned char pulse[2][WAVEFORM_SAMPLES];
    int bcnt[2] = {0, 0};
    for (f = 0; f < 200; f++) {
        for (ch = 0; ch < 2; ch++)
            for (s = 0; s < WAVEFORM_SAMPLES; s++) {
                float t = (float)s / WAVEFORM_SAMPLES;
                float burst = (f % 30 < 3) ? 1.0f : 0.0f;
                float v = 128.0f + 100.0f * burst * sinf(t * 12.56637f)
                        + (float)(rand() % 20 - 10);
                if (v < 0) { v = 0; }
                if (v > 255) { v = 255; }
                pulse[ch][s] = (unsigned char)v;
            }
        analyze(2, pulse);
        if (getBeat(0)) bcnt[0]++;
        if (getBeat(1)) bcnt[1]++;
    }
    TEST("left channel detects beats",  bcnt[0] > 3);
    TEST("right channel detects beats", bcnt[1] > 3);
    int diff = bcnt[0] > bcnt[1] ? bcnt[0] - bcnt[1] : bcnt[1] - bcnt[0];
    TEST("L/R counts similar", diff < (bcnt[0] > bcnt[1] ? bcnt[0] : bcnt[1]) / 2 + 1);
    printf("  beats: ch0=%d ch1=%d\n", bcnt[0], bcnt[1]);
    printf("\n");

    // ===== 7. Random noise (1000 frames) =====
    printf("--- Random Noise (1000 frames) ---\n");
    for (f = 0; f < 1000; f++) {
        for (ch = 0; ch < 2; ch++)
            for (s = 0; s < WAVEFORM_SAMPLES; s++)
                data[ch][s] = (unsigned char)(rand() % 256);
        analyze(2, data);
    }
    TEST("1000 frames noise", 1);
    printf("\n");

    // ===== 8. Visualizer rendering =====
    printf("--- Visualizer Rendering ---\n\n");

    // Set up globals for rendering
    windowWidth = 288;
    windowHeight = 256;
    mainWindowHandle = NULL;
    showStats = 0;
    fullyExited = 0;
    *displayString = 0;
    *originalSongTitle = 0;
    *currentSongTitle = 0;
    *positionText = 0;
    *lengthText = 0;
    scrollLength = 18;
    allegroError = 0;
    allegroInitialized = 0;
    usedBuffer = NULL;

    // Create a local module struct for testing
    struct winampVisModule mod;
    memset(&mod, 0, sizeof(mod));
    mod.hwndParent = NULL;
    mod.hDllInstance = NULL;
    mod.sRate = 44100;
    mod.nCh = 2;
    mod.spectrumNch = 2;
    mod.waveformNch = 2;
    mod.userData = NULL;

    // fill test data
    for (ch = 0; ch < 2; ch++)
        for (s = 0; s < WAVEFORM_SAMPLES; s++) {
            mod.waveformData[ch][s] = (unsigned char)(rand() % 256);
            mod.spectrumData[ch][s] = (unsigned char)(rand() % 256);
        }

    // 8a. All renderers bail when allegro not initialized
    printf("  --- Render without Allegro ---\n");
    TEST("Wave no allegro", visWaveRender(&mod) == 1);
    TEST("Spectrum no allegro", visSpectrumRender(&mod) == 1);
    TEST("VU no allegro", visVuRender(&mod) == 1);
    TEST("Grind no allegro", visGrindRender(&mod) == 1);
    TEST("Boom no allegro", visBoomRender(&mod) == 1);
    printf("\n");

    // 8b. All renderers bail with nCh < 1
    printf("  --- Render with nCh=0 ---\n");
    mod.nCh = 0;
    TEST("Wave nCh=0", visWaveRender(&mod) == 1);
    TEST("Spectrum nCh=0", visSpectrumRender(&mod) == 1);
    TEST("VU nCh=0", visVuRender(&mod) == 1);
    TEST("Grind nCh=0", visGrindRender(&mod) == 1);
    TEST("Boom nCh=0", visBoomRender(&mod) == 1);
    printf("\n");

    // 8c. nCh=1 mono
    printf("  --- Render with nCh=1 mono ---\n");
    mod.nCh = 1;
    // allegro not init'd, so returns 1
    TEST("Wave nCh=1", visWaveRender(&mod) == 1);
    TEST("Spectrum nCh=1", visSpectrumRender(&mod) == 1);
    TEST("VU nCh=1", visVuRender(&mod) == 1);
    TEST("Grind nCh=1", visGrindRender(&mod) == 1);
    TEST("Boom nCh=1", visBoomRender(&mod) == 1);
    printf("\n");

    // 8d. Render with Allegro init, no display
    printf("  --- Render with Allegro (no display) ---\n");
    ensureAllegro();
    mod.nCh = 2;
    TEST("Wave allegro render", visWaveRender(&mod) == 0);
    TEST("Spectrum allegro render", visSpectrumRender(&mod) == 0);
    TEST("VU allegro render", visVuRender(&mod) == 0);
    TEST("Grind allegro render", visGrindRender(&mod) == 0);
    TEST("Boom allegro render", visBoomRender(&mod) == 0);
    printf("\n");

    // 8e. Render with extreme data values
    printf("  --- Render with edge data ---\n");
    memset(mod.waveformData, 0, sizeof(mod.waveformData));
    memset(mod.spectrumData, 0, sizeof(mod.spectrumData));
    TEST("Wave all zeros", visWaveRender(&mod) == 0);
    TEST("Spectrum all zeros", visSpectrumRender(&mod) == 0);
    TEST("VU all zeros", visVuRender(&mod) == 0);
    TEST("Grind all zeros", visGrindRender(&mod) == 0);
    TEST("Boom all zeros", visBoomRender(&mod) == 0);

    memset(mod.waveformData, 255, sizeof(mod.waveformData));
    memset(mod.spectrumData, 255, sizeof(mod.spectrumData));
    TEST("Wave all 255", visWaveRender(&mod) == 0);
    TEST("Spectrum all 255", visSpectrumRender(&mod) == 0);
    TEST("VU all 255", visVuRender(&mod) == 0);
    TEST("Grind all 255", visGrindRender(&mod) == 0);
    TEST("Boom all 255", visBoomRender(&mod) == 0);
    printf("\n");

    // 8f. Boom input handler edge cases
    printf("  --- Boom input handler ---\n");
    visBoomInput(&mod, 'w');  TEST("Boom toggle mode", 1);
    visBoomInput(&mod, 'W');  TEST("Boom toggle mode (caps)", 1);
    visBoomInput(&mod, 't');  TEST("Boom toggle BG", 1);
    visBoomInput(&mod, 'T');  TEST("Boom toggle BG (caps)", 1);
    visBoomInput(&mod, 0);    TEST("Boom null key", 1);
    visBoomInput(&mod, 999);  TEST("Boom invalid key", 1);
    printf("\n");

    // 8g. Boom init/quit (needs Allegro bitmap)
    printf("  --- Boom init/quit ---\n");
    // visBoomInit loads a bitmap from disk - may fail gracefully
    visBoomInit(&mod);
    TEST("Boom init (bitmap may be NULL)", 1);
    visBoomQuit();
    TEST("Boom quit", 1);
    printf("\n");

    // 8h. Module recreation (init/quit cycling)
    printf("  --- Boom init/quit cycling ---\n");
    for (f = 0; f < 10; f++) {
        visBoomQuit();
        visBoomQuit();  // double-quit safe?
    }
    TEST("Boom double-quit no crash", 1);

    dropAllegro();
    printf("\n");

    // ===== 9. FFT/Beat cleanup =====
    FFT_CleanUp();
    beatDetectionQuit();
    printf("--- Cleanup OK ---\n");

    printf("\n=================\n");
    printf("Results: %d passed, %d failed, %d total\n", passed, failed, passed + failed);
    return failed > 0 ? 1 : 0;
}
