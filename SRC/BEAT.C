#include <math.h>
#include "beat.h"
#include "fft.h"

struct SoundInfoChannel {
    int   isBeat;
    int   timer;
    float hasFallen;
    float lastHitVolume;
    float threshold;
    float limit;
    float sum[FREQ_BANDS];
    float infiniteAverage[FREQ_BANDS];
    float immediate[FREQ_BANDS];
    float average[FREQ_BANDS];
    float mediumAverage[FREQ_BANDS];
    float longAverage[FREQ_BANDS];
    float fWaveform[WAVEFORM_SAMPLES];
    float fSpectrum[FFT_BINS];
};

static struct SoundInfoChannel soundInfo[MAX_AUDIO_CH];
static int beatDetectionInitialized = 0;
static float decayRate = 0.5f;
static float beatTimeDecay = 1.0f;
static int minInfiniteSamples = 300;
static int bandStart[FREQ_BANDS];
static int bandEnd[FREQ_BANDS];

__attribute__((constructor)) static void initBands() {
    int i;
    for (i = 0; i < FREQ_BANDS; i++) {
        float exp = 2.1f;
        bandStart[i] = (int)(FFT_BINS * 0.5f * powf(i / (float)FREQ_BANDS, exp));
        bandEnd[i]   = (int)(FFT_BINS * 0.5f * powf((i + 1) / (float)FREQ_BANDS, exp));
    }
}

int getBeat(int channel) {
    if (channel < 0 || channel >= MAX_AUDIO_CH) return 0;
    return soundInfo[channel].isBeat;
}

static void resetSoundInfo() {
    int ch, b;
    for (ch = 0; ch < MAX_AUDIO_CH; ch++) {
        soundInfo[ch].hasFallen = 1; soundInfo[ch].threshold = 1; soundInfo[ch].limit = 1; soundInfo[ch].timer = 1;
        soundInfo[ch].isBeat = 0; soundInfo[ch].lastHitVolume = 0;
        for (b = 0; b < FREQ_BANDS; b++) soundInfo[ch].sum[b] = 0;
    }
}

void beatDetectionQuit() {
    if (beatDetectionInitialized) {
        resetSoundInfo();
        FFT_CleanUp();
        beatDetectionInitialized = 0;
    }
}

static int beatDetectionInit() {
    if (beatDetectionInitialized) {
        beatDetectionQuit();
        beatDetectionInitialized = 0;
    }
    if (FFT_Init(WAVEFORM_SAMPLES)) return -1;
    resetSoundInfo();
    beatDetectionInitialized = 1;
    return 0;
}

static float adjustRateTo(float perFrameDecayRateAtNum1, float num1, float actualNum) {
    float perSecondDecayRateAtNum1 = powf(perFrameDecayRateAtNum1, num1);
    float perFrameDecayRateAtNum2 = powf(perSecondDecayRateAtNum1, 1.0f / actualNum);
    return perFrameDecayRateAtNum2;
}

void analyze(int numCh, unsigned char waveformData[][WAVEFORM_SAMPLES]) {
    int i, ch, b, previousIndex = -1;
    float tempWave[MAX_AUDIO_CH][WAVEFORM_SAMPLES];
    float mediumMix[MAX_AUDIO_CH], longMix[MAX_AUDIO_CH];
    float averageMix[MAX_AUDIO_CH], volume[MAX_AUDIO_CH], decay;
    if (numCh > MAX_AUDIO_CH) numCh = MAX_AUDIO_CH;
    if (numCh < 1) return;

    if (!beatDetectionInitialized && beatDetectionInit()) return;

    for (i = 0; i < numCh; i++) soundInfo[i].isBeat = 0;
    for (i = 0; i < numCh; i++) { mediumMix[i] = 0.965f; longMix[i] = 0.985f; }

    for (i = 0; i < WAVEFORM_SAMPLES; i++) {
        for (ch = 0; ch < numCh; ch++) {
            soundInfo[ch].fWaveform[i] = (float)((waveformData[ch][i] ^ 128) - 128);
            if (previousIndex >= 0)
                tempWave[ch][i] = 0.5f * (soundInfo[ch].fWaveform[i] + soundInfo[ch].fWaveform[previousIndex]);
        }
        previousIndex = i;
    }
    for (ch = 0; ch < numCh; ch++)
        FFT_TimeToFrequencyDomain(tempWave[ch], soundInfo[ch].fSpectrum);

    for (i = 0; i < FREQ_BANDS; i++) {
        int j;
        int start = bandStart[i];
        int ends  = bandEnd[i];
        for (ch = 0; ch < numCh; ch++) {
            soundInfo[ch].immediate[i] = 0;
            for (j = start; j < ends; j++)
                soundInfo[ch].immediate[i] += soundInfo[ch].fSpectrum[j];
            soundInfo[ch].immediate[i] /= (float)(ends - start);
        }
    }

    for (ch = 0; ch < numCh; ch++) {
        for (b = 0; b < FREQ_BANDS; b++)
            soundInfo[ch].sum[b] = beatTimeDecay * soundInfo[ch].sum[b] + soundInfo[ch].immediate[b];
        if (soundInfo[ch].timer > minInfiniteSamples) {
            for (b = 0; b < FREQ_BANDS; b++)
                soundInfo[ch].infiniteAverage[b] = soundInfo[ch].sum[b] / (soundInfo[ch].timer - minInfiniteSamples);
        }
        else
        {
            soundInfo[ch].infiniteAverage[0] = 0.270f;
            soundInfo[ch].infiniteAverage[1] = 0.343f;
            soundInfo[ch].infiniteAverage[2] = 0.295f;
        }
        soundInfo[ch].timer++;
    }

    for (ch = 0; ch < numCh; ch++)
        for (b = 0; b < FREQ_BANDS; b++)
            soundInfo[ch].immediate[b] /= soundInfo[ch].infiniteAverage[b];

    for (ch = 0; ch < numCh; ch++) {
        mediumMix[ch]  = adjustRateTo(mediumMix[ch], (float)BLEND_FPS, decayRate);
        longMix[ch] = adjustRateTo(longMix[ch], (float)BLEND_FPS, decayRate);
    }

    for (i = 0; i < FREQ_BANDS; i++) {
        for (ch = 0; ch < numCh; ch++) {
            if (soundInfo[ch].immediate[i] > soundInfo[ch].average[i])
                averageMix[ch] = adjustRateTo(0.2f, (float)BLEND_FPS, decayRate);
            else
                averageMix[ch] = adjustRateTo(0.5f, (float)BLEND_FPS, decayRate);
            soundInfo[ch].average[i] = soundInfo[ch].average[i] * averageMix[ch]
                + soundInfo[ch].immediate[i] * (1 - averageMix[ch]);
            soundInfo[ch].mediumAverage[i] = soundInfo[ch].mediumAverage[i] * mediumMix[ch]
                + soundInfo[ch].immediate[i] * (1 - mediumMix[ch]);
            soundInfo[ch].longAverage[i] = soundInfo[ch].longAverage[i] * longMix[ch]
                + soundInfo[ch].immediate[i] * (1 - longMix[ch]);
        }
    }

    for (ch = 0; ch < numCh; ch++) {
        float avgDiv = 0;
        for (b = 0; b < FREQ_BANDS; b++)
            avgDiv += soundInfo[ch].average[b] / soundInfo[ch].mediumAverage[b];
        volume[ch] = avgDiv / FREQ_BANDS;
    }

    decay = adjustRateTo(0.997f, 20.0f, decayRate);

    for (ch = 0; ch < numCh; ch++) {
        soundInfo[ch].threshold = soundInfo[ch].threshold * decay + soundInfo[ch].limit * (1 - decay);
        if (!soundInfo[ch].hasFallen) {
            if (volume[ch] < soundInfo[ch].threshold) {
                soundInfo[ch].hasFallen = 1;
                soundInfo[ch].threshold = soundInfo[ch].lastHitVolume * 1.03f;
                soundInfo[ch].limit = 1.1f;
            }
        }
        else
        {
            if (volume[ch] > soundInfo[ch].threshold && volume[ch] > 0.2f) {
                soundInfo[ch].isBeat = 1;
                soundInfo[ch].lastHitVolume = volume[ch];
                soundInfo[ch].hasFallen = 0;
                soundInfo[ch].threshold = fmaxf(1.0f, volume[ch] * 0.85f);
                soundInfo[ch].limit = fmaxf(1.0f, volume[ch] * 0.70f);
            }
        }
    }
}
