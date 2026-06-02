#ifndef BEAT_H
#define BEAT_H

#include "decl.h"

int getBeat(int channel);
void beatDetectionQuit();
void analyze(int numCh, unsigned char waveformData[][WAVEFORM_SAMPLES]);

#endif
