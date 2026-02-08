#ifndef SNAP_DETECTION_H
#define SNAP_DETECTION_H

#include <Arduino.h>
#include "config.h"

// Detection result
enum SnapResult {
  SNAP_NONE,           // No snap detected
  SNAP_WAITING_DECAY,  // Potential snap, waiting for decay confirmation
  SNAP_FIRST,          // First snap confirmed (waiting for second)
  SNAP_DOUBLE          // Double snap detected!
};

// Callback type for double snap events
typedef void (*DoubleSnapCallback)();

// Initialize the snap detector
void initSnapDetector();

// Set the callback for double snap events
void setDoubleSnapCallback(DoubleSnapCallback callback);

// Process audio and detect snaps
// Call this each loop iteration after reading audio
// Returns the detection result
SnapResult processSnapDetection(int32_t maxAmplitude, double rms);

#endif // SNAP_DETECTION_H
