#include "audio.h"
#include <math.h>

// Audio buffers
int32_t rawSamples[SAMPLES * 2];
double vReal[SAMPLES];
double vImag[SAMPLES];

void setupI2S() {
  i2s_driver_uninstall(I2S_PORT);
  delay(100);

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLING_FREQ,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
  i2s_start(I2S_PORT);
}

int readAudioSamples(int32_t* maxAmplitude, double* rms) {
  size_t bytesRead = 0;
  
  esp_err_t result = i2s_read(I2S_PORT, rawSamples, sizeof(rawSamples), &bytesRead, portMAX_DELAY);
  if (result != ESP_OK || bytesRead == 0) return 0;
  
  int samplesRead = bytesRead / sizeof(int32_t);
  
  // Extract left channel and calculate amplitude + RMS
  *maxAmplitude = 0;
  double sumSquares = 0;
  
  for (int i = 0; i < SAMPLES && i * 2 < samplesRead; i++) {
    int32_t sample = rawSamples[i * 2];
    double normalizedSample = (double)(sample >> 8);
    vReal[i] = normalizedSample;
    vImag[i] = 0.0;
    
    sumSquares += normalizedSample * normalizedSample;
    
    int32_t absSample = sample < 0 ? -sample : sample;
    if (absSample > *maxAmplitude) *maxAmplitude = absSample;
  }
  
  *rms = sqrt(sumSquares / SAMPLES);
  
  return SAMPLES;
}
