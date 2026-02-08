#include <Arduino.h>
#include "config.h"
#include "audio.h"
#include "snap_detection.h"
#include "mqtt_ha.h"

// ============================================
// DOUBLE SNAP CALLBACK
// ============================================
void onDoubleSnapDetected() {
  Serial.println();
  Serial.println("***********************************************");
  Serial.println("*         DOUBLE SNAP DETECTED!               *");
  Serial.println("*              ACTIVATED!                     *");
  Serial.println("***********************************************");
  Serial.println();
  
  // Publish to Home Assistant via MQTT
  publishSnapEvent();
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println();
  Serial.println("================================================");
  Serial.println("          ha-snap-switch");
  Serial.println("================================================");
  Serial.println();
  
  // Initialize network (WiFi + MQTT)
  initNetwork();
  
  Serial.println();
  Serial.println("Detection Parameters:");
  Serial.printf("  Snap frequency range: %d - %d Hz\n", SNAP_FREQ_LOW, SNAP_FREQ_HIGH);
  Serial.printf("  Energy threshold: %d\n", SNAP_ENERGY_THRESHOLD);
  Serial.printf("  Double snap window: %d - %d ms\n", DOUBLE_SNAP_MIN_GAP_MS, DOUBLE_SNAP_MAX_GAP_MS);
  Serial.println();
  Serial.println("Snap your fingers TWICE to activate!");
  Serial.println("================================================");
  Serial.println();
  
  // Initialize I2S microphone
  setupI2S();
  
  // Initialize snap detector with callback
  initSnapDetector();
  setDoubleSnapCallback(onDoubleSnapDetected);
  
  delay(500);
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  // Maintain network connection
  networkLoop();
  
  // Read audio samples
  int32_t maxAmplitude;
  double rms;
  int samplesRead = readAudioSamples(&maxAmplitude, &rms);
  
  if (samplesRead == 0) return;
  
  // Process snap detection
  processSnapDetection(maxAmplitude, rms);
  
  delay(10);
}
