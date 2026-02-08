#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

// Audio buffers (accessible externally for FFT processing)
extern int32_t rawSamples[SAMPLES * 2];
extern double vReal[SAMPLES];
extern double vImag[SAMPLES];

// Initialize the I2S microphone
void setupI2S();

// Read audio samples and return info
// Returns: number of samples read, fills maxAmplitude and rms
int readAudioSamples(int32_t* maxAmplitude, double* rms);

#endif // AUDIO_H
