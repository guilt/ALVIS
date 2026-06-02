/*
  LICENSE
  -------
  Copyright (C) 1999-2002 Nullsoft, Inc.

  This source code is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this source code or the software it produces.

  Permission is granted to anyone to use this source code for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this source code must not be misrepresented; you must not
     claim that you wrote the original source code.  If you use this source code
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original source code.
  3. This notice may not be removed or altered from any source distribution.
*/

/* Altered Edition */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "decl.h"
#include "fft.h"

#define SafeDeleteArray(x) { if (x) { free(x); x = NULL; } }

static int   *bitReverseTable  = NULL;
static float *windowEnvelope   = NULL;
static float *equalizeTable    = NULL;
static float *realBuffer       = NULL;
static float *imagBuffer       = NULL;
static float (*cosSinTable)[2] = NULL;
static int    fftSampleCount    = 0;

static void initEqualizeTable() {
    int i;
    float scaling = -0.02f;
    float invHalfNfreq = 1.0f / (float)(FFT_BINS / 2);

    equalizeTable = malloc(sizeof(float) * (FFT_BINS / 2));
    for (i = 0; i < FFT_BINS / 2; i++)
        equalizeTable[i] = scaling * log((float)(FFT_BINS / 2 - i) * invHalfNfreq);
}

static void initEnvelopeTable(float power) {
    int i;
    float mult = 1.0f / (float)fftSampleCount * 6.2831853f;

    windowEnvelope = malloc(sizeof(float) * fftSampleCount);
    if (power == 1.0f)
        for (i = 0; i < fftSampleCount; i++)
            windowEnvelope[i] = 0.5f + 0.5f * sin(i * mult - 1.5707963268f);
    else
        for (i = 0; i < fftSampleCount; i++)
            windowEnvelope[i] = pow(0.5f + 0.5f * sin(i * mult - 1.5707963268f), power);
}

static void initBitReverseTable() {
    int i, j, m, temp;
    bitReverseTable = malloc(sizeof(int) * FFT_BINS);
    for (i = 0; i < FFT_BINS; i++)
        bitReverseTable[i] = i;

    for (i = 0, j = 0; i < FFT_BINS; i++) {
        if (j > i) {
            temp = bitReverseTable[i];
            bitReverseTable[i] = bitReverseTable[j];
            bitReverseTable[j] = temp;
        }
        m = FFT_BINS >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
}

static void initCosSinTable() {
    int i, dftsize, tabsize;
    float theta;

    dftsize = 2;
    tabsize = 0;
    while (dftsize <= FFT_BINS) {
        tabsize++;
        dftsize <<= 1;
    }

    cosSinTable = malloc(sizeof(float) * tabsize * 2);
    dftsize = 2;
    i = 0;
    while (dftsize <= FFT_BINS) {
        theta = -2.0f * PI / (float)dftsize;
        cosSinTable[i][0] = cosf(theta);
        cosSinTable[i][1] = sinf(theta);
        i++;
        dftsize <<= 1;
    }
}

int FFT_Init(int inSampleCount) {
    FFT_CleanUp();

    if (inSampleCount <= 0)
        fftSampleCount = WAVEFORM_SAMPLES;
    else
        fftSampleCount = inSampleCount;

    initBitReverseTable();
    initCosSinTable();
    initEnvelopeTable(1.0f);
    initEqualizeTable();
    realBuffer = malloc(sizeof(float) * FFT_BINS);
    imagBuffer = malloc(sizeof(float) * FFT_BINS);

    if (!bitReverseTable || !windowEnvelope || !cosSinTable || !equalizeTable || !realBuffer || !imagBuffer) {
        FFT_CleanUp();
        return -1;
    }
    return 0;
}

__attribute__((constructor)) static void autoInit() {
    FFT_Init(WAVEFORM_SAMPLES);
}

void FFT_CleanUp() {
    SafeDeleteArray(windowEnvelope);
    SafeDeleteArray(equalizeTable);
    SafeDeleteArray(bitReverseTable);
    SafeDeleteArray(cosSinTable);
    SafeDeleteArray(realBuffer);
    SafeDeleteArray(imagBuffer);
}

void FFT_TimeToFrequencyDomain(float *inWaveData, float *outSpectralData) {
    int j, m, i, dftsize, hdftsize, t;
    float wr, wi, wpi, wpr, wtemp, tempr, tempi;
    float *real, *imag;

    if (!bitReverseTable || !windowEnvelope || !cosSinTable || !equalizeTable || !realBuffer || !imagBuffer) {
        if (FFT_Init(WAVEFORM_SAMPLES))
            return;
    }

    for (i = 0; i < FFT_BINS; i++) {
        int idx = bitReverseTable[i];
        if (idx < fftSampleCount)
            realBuffer[i] = inWaveData[idx] * windowEnvelope[idx];
        else
            realBuffer[i] = 0;
    }
    memset(imagBuffer, 0, sizeof(float) * FFT_BINS);

    real = realBuffer;
    imag = imagBuffer;
    dftsize = 2;
    t = 0;
    while (dftsize <= FFT_BINS) {
        wpr = cosSinTable[t][0];
        wpi = cosSinTable[t][1];
        wr = 1.0f;
        wi = 0.0f;
        hdftsize = dftsize >> 1;

        for (m = 0; m < hdftsize; m += 1) {
            for (i = m; i < FFT_BINS; i += dftsize) {
                j = i + hdftsize;
                tempr = wr * real[j] - wi * imag[j];
                tempi = wr * imag[j] + wi * real[j];
                real[j] = real[i] - tempr;
                imag[j] = imag[i] - tempi;
                real[i] += tempr;
                imag[i] += tempi;
            }
            wr = (wtemp = wr) * wpr - wi * wpi;
            wi = wi * wpr + wtemp * wpi;
        }
        dftsize <<= 1;
        t++;
    }

    for (i = 0; i < FFT_BINS / 2; i++)
        outSpectralData[i] = equalizeTable[i] * sqrtf(realBuffer[i] * realBuffer[i] + imagBuffer[i] * imagBuffer[i]);
}
