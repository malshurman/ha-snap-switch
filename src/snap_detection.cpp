#include "snap_detection.h"
#include "audio.h"
#include "iir_filters.h"
#include <math.h>

// State variables
static unsigned long lastSnapTime = 0;
static unsigned long lastActivationTime = 0;
static int snapCount = 0;
static bool waitingForDecay = false;
static double peakEnergyDuringSnap = 0;
static unsigned long snapStartTime = 0;

// Energy history for rise time detection
static double energyHistory[ENERGY_HISTORY_SIZE] = {0};
static int energyHistoryIdx = 0;
static double prevSnapEnergy = 0;
static double prevBrightEnergy = 0;

// Callback
static DoubleSnapCallback doubleSnapCallback = nullptr;

void initSnapDetector() {
  lastSnapTime = 0;
  lastActivationTime = 0;
  snapCount = 0;
  waitingForDecay = false;
  energyHistoryIdx = 0;
  prevSnapEnergy = 0;
  prevBrightEnergy = 0;
  for (int i = 0; i < ENERGY_HISTORY_SIZE; i++) {
    energyHistory[i] = 0;
  }
}

void setDoubleSnapCallback(DoubleSnapCallback callback) {
  doubleSnapCallback = callback;
}

SnapResult processSnapDetection(int32_t maxAmplitude, double rms) {
  // Calculate crest factor
  double peakValue = (double)(maxAmplitude >> 8);
  double crestFactor = (rms > 100) ? (peakValue / rms) : 0;

  // Get IIR filter energies
  double rejectEnergy = getRejectEnergy();
  double snapEnergy = getSnapEnergy();
  double brightEnergy = getBrightEnergy();

  // Derived metrics
  double totalEnergy = snapEnergy + brightEnergy;
  double snapRatio = (rejectEnergy > 1e-6) ? (snapEnergy / rejectEnergy) : 999.0;
  double brightRatio = (totalEnergy > 1e-6) ? (brightEnergy / totalEnergy) : 0.0;

  // Spectral flux: sudden energy change (key for transient detection)
  double spectralFlux = fabs(snapEnergy - prevSnapEnergy) + fabs(brightEnergy - prevBrightEnergy);

  // Rise time detection - compare to recent history
  double avgPrevEnergy = 0;
  for (int i = 0; i < ENERGY_HISTORY_SIZE; i++) {
    avgPrevEnergy += energyHistory[i];
  }
  avgPrevEnergy /= ENERGY_HISTORY_SIZE;

  double riseFactor = (avgPrevEnergy > 100) ? (snapEnergy / avgPrevEnergy) :
                      (snapEnergy > SNAP_ENERGY_THRESHOLD ? 999.0 : 0.0);

  // Update energy history ring buffer (BEFORE any early returns)
  energyHistory[energyHistoryIdx] = snapEnergy;
  prevSnapEnergy = snapEnergy;
  prevBrightEnergy = brightEnergy;
  energyHistoryIdx = (energyHistoryIdx + 1) % ENERGY_HISTORY_SIZE;
  
  unsigned long now = millis();
  
  // Check cooldown
  if (now - lastActivationTime < ACTIVATION_COOLDOWN_MS) {
    return SNAP_NONE;
  }
  
  // Check for decay after potential snap
  if (waitingForDecay) {
    unsigned long elapsed = now - snapStartTime;

    // Anti-sustain check: reject if energy stays high after 50ms (catches toilet flushes, faucets)
    if (elapsed > SUSTAIN_CHECK_TIME_MS && elapsed < DECAY_TIME_MS) {
      double sustainRatio = snapEnergy / peakEnergyDuringSnap;
      if (sustainRatio > SUSTAIN_THRESHOLD) {
        waitingForDecay = false;
        return SNAP_NONE;
      }
    }

    // Max duration check - reject sustained sounds
    if (elapsed > MAX_SNAP_DURATION_MS) {
      waitingForDecay = false;
      return SNAP_NONE;
    }

    if (elapsed > DECAY_TIME_MS) {
      double currentEnergy = snapEnergy;
      double decayRatio = (peakEnergyDuringSnap > 0) ? (currentEnergy / peakEnergyDuringSnap) : 1.0;
      
      if (decayRatio < DECAY_FACTOR) {
        // Good decay - confirms impulsive sound (snap-like)
        snapCount++;

        // Clear energy history to prevent contamination for next snap
        for (int i = 0; i < ENERGY_HISTORY_SIZE; i++) {
          energyHistory[i] = 0;
        }

        if (snapCount >= 2) {
          unsigned long gap = now - lastSnapTime;
          if (gap >= DOUBLE_SNAP_MIN_GAP_MS && gap <= DOUBLE_SNAP_MAX_GAP_MS) {
            // Double snap detected!
            snapCount = 0;
            lastSnapTime = now;
            lastActivationTime = now;
            waitingForDecay = false;

            if (doubleSnapCallback) {
              doubleSnapCallback();
            }

            return SNAP_DOUBLE;
          } else {
            // Gap out of range, treat this as a new first snap
            snapCount = 1;
            lastSnapTime = now;
            waitingForDecay = false;
            return SNAP_FIRST;
          }
        }

        // First snap in sequence
        lastSnapTime = now;
        Serial.printf("[SNAP %d/2] Waiting for second snap...\n", snapCount);

        waitingForDecay = false;
        return SNAP_FIRST;
      } else {
        // Poor decay - not a snap (sustained sound)
        waitingForDecay = false;
        return SNAP_NONE;
      }
    }
    
    // Track peak energy during the snap event (for decay comparison)
    if (snapEnergy > peakEnergyDuringSnap) {
      peakEnergyDuringSnap = snapEnergy;
    }
    
    return SNAP_WAITING_DECAY;
  }
  
  // Reset snap count if too much time passed
  if (snapCount > 0 && (now - lastSnapTime) > DOUBLE_SNAP_MAX_GAP_MS) {
    snapCount = 0;
  }
  
  // === Snap signature checks (simplified for IIR) ===
  bool hasEnoughAmplitude = maxAmplitude > AMPLITUDE_THRESHOLD;
  bool hasEnoughSnapEnergy = snapEnergy > SNAP_ENERGY_THRESHOLD;
  bool hasGoodRatio = snapRatio > SNAP_RATIO_THRESHOLD;
  bool hasGoodCrestFactor = crestFactor > MIN_CREST_FACTOR;
  bool hasSuddenChange = spectralFlux > FLUX_THRESHOLD;
  bool hasSharpRise = riseFactor > MIN_RISE_FACTOR;
  bool hasBrightness = brightRatio > MIN_HIGH_FREQ_RATIO;

  bool isLikelySnap = hasEnoughAmplitude && hasEnoughSnapEnergy && hasGoodRatio &&
                      hasGoodCrestFactor && hasSuddenChange && hasSharpRise && hasBrightness;
  
  if (isLikelySnap) {
    waitingForDecay = true;
    peakEnergyDuringSnap = snapEnergy;
    snapStartTime = now;

    return SNAP_WAITING_DECAY;
  }
  
  return SNAP_NONE;
}
