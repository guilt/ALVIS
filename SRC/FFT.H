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

#ifndef FFT_H
#define FFT_H

#define NFREQ 512 /* Powers of 2 */

int m_ready;
int m_samples_in;

int  FFT_Init(int samples_in);
void time_to_frequency_domain(float *in_wavedata, float *out_spectraldata);
int  GetNumFreq() { return NFREQ; };
void FFT_CleanUp();

void InitEnvelopeTable(float power);
void InitEqualizeTable();
void InitBitRevTable();
void InitCosSinTable();
    
int   *bitrevtable=NULL;
float *envelope=NULL;
float *equalize=NULL;
float *temp1=NULL;
float *temp2=NULL;
float (*cossintable)[2]=NULL;

#endif /* FFT_H */
