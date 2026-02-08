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
static double prevTransientEnergy = 0;

// Callback
static DoubleSnapCallback doubleSnapCallback = nullptr;

void initSnapDetector() {
  lastSnapTime = 0;
  lastActivationTime = 0;
  snapCount = 0;
  waitingForDecay = false;
  energyHistoryIdx = 0;
  prevSnapEnergy = 0;
  prevTransientEnergy = 0;
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
  double transientEnergy = getTransientEnergy();

  // Derived metrics
  double snapRatio = (rejectEnergy > 1e-6) ? (snapEnergy / rejectEnergy) : 999.0;

  // Spectral flux: sudden energy change (key for transient detection)
  double spectralFlux = fabs(snapEnergy - prevSnapEnergy) + fabs(transientEnergy - prevTransientEnergy);

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
  prevTransientEnergy = transientEnergy;
  energyHistoryIdx = (energyHistoryIdx + 1) % ENERGY_HISTORY_SIZE;
  
  unsigned long now = millis();

  // DEBUG: Print all metrics every 100ms
  static unsigned long lastDebugPrint = 0;
  if (now - lastDebugPrint > 100) {
    Serial.printf("E[snap=%.0f rej=%.0f trans=%.0f] R[s/r=%.2f] M[flux=%.0f rise=%.1fx crest=%.1f] A[max=%ld rms=%.0f]",
        snapEnergy, rejectEnergy, transientEnergy,
        snapRatio,
        spectralFlux, riseFactor, crestFactor,
        maxAmplitude, rms);

    // Show which checks would pass
    if (snapEnergy > SNAP_ENERGY_THRESHOLD) Serial.print(" \xE2\x9C\x93""ENERGY");
    if (spectralFlux > FLUX_THRESHOLD) Serial.print(" \xE2\x9C\x93""FLUX");
    if (riseFactor > MIN_RISE_FACTOR) Serial.print(" \xE2\x9C\x93""RISE");

    Serial.println();
    lastDebugPrint = now;
  }

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
  
  // === PRIMARY SNAP DETECTION (Essential Checks) ===
  bool hasEnoughSnapEnergy = snapEnergy > SNAP_ENERGY_THRESHOLD;
  bool hasSuddenChange = spectralFlux > FLUX_THRESHOLD;
  bool hasSharpRise = riseFactor > MIN_RISE_FACTOR;
  bool hasGoodCrestFactor = crestFactor > MIN_CREST_FACTOR;
  bool hasGoodRatio = snapRatio > SNAP_RATIO_THRESHOLD;

  bool isPrimarySnap = hasEnoughSnapEnergy && hasSuddenChange && hasSharpRise &&
                       hasGoodCrestFactor && hasGoodRatio;

  // === DIAGNOSTICS ===
  if (hasEnoughSnapEnergy || hasSuddenChange) {
    if (!hasGoodCrestFactor) Serial.printf("[WARN] Low crest: %.1f < %.1f\n", crestFactor, MIN_CREST_FACTOR);
    if (!hasGoodRatio) Serial.printf("[WARN] Low ratio: %.2f < %.2f\n", snapRatio, SNAP_RATIO_THRESHOLD);
  }

  if (isPrimarySnap) {
    Serial.println("[TRIGGER] Primary snap signature detected - entering decay confirmation");
    waitingForDecay = true;
    peakEnergyDuringSnap = snapEnergy;
    snapStartTime = now;
    return SNAP_WAITING_DECAY;
  }
  
  return SNAP_NONE;
}
