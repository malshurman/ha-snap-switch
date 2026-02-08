#include "snap_detection.h"
#include "audio.h"
#include <arduinoFFT.h>
#include <math.h>

// FFT object
static ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);

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
static double prevTotalEnergy = 0;

// Callback
static DoubleSnapCallback doubleSnapCallback = nullptr;

void initSnapDetector() {
  lastSnapTime = 0;
  lastActivationTime = 0;
  snapCount = 0;
  waitingForDecay = false;
  energyHistoryIdx = 0;
  prevTotalEnergy = 0;
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
  
  // Perform FFT
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();
  
  // Calculate energy in different frequency bands + spectral centroid
  double lowFreqEnergy = 0;
  double midFreqEnergy = 0;
  double snapFreqEnergy = 0;
  double highFreqEnergy = 0;
  double weightedFreqSum = 0;
  double totalMagnitude = 0;
  
  for (int i = 2; i < (SAMPLES / 2); i++) {
    double freq = (double)(i * SAMPLING_FREQ) / SAMPLES;
    double energy = vReal[i];
    
    weightedFreqSum += freq * energy;
    totalMagnitude += energy;
    
    if (freq < 500) {
      lowFreqEnergy += energy;
    } else if (freq < SNAP_FREQ_LOW) {
      midFreqEnergy += energy;
    } else if (freq <= SNAP_FREQ_HIGH) {
      snapFreqEnergy += energy;
    } else {
      highFreqEnergy += energy;
    }
  }
  
  double totalEnergy = lowFreqEnergy + midFreqEnergy + snapFreqEnergy + highFreqEnergy;
  double snapRatio = (lowFreqEnergy > 1000) ? (snapFreqEnergy / lowFreqEnergy) : 999;
  double spectralCentroid = (totalMagnitude > 1000) ? (weightedFreqSum / totalMagnitude) : 0;
  double highFreqRatio = (totalEnergy > 1000) ? (highFreqEnergy / totalEnergy) : 0;
  double midFreqRatio = (totalEnergy > 1000) ? (midFreqEnergy / totalEnergy) : 0;
  
  // === NEW: Spectral concentration - what fraction of energy is in the snap band ===
  double spectralConcentration = (totalEnergy > 1000) ? (snapFreqEnergy / totalEnergy) : 0;
  
  // === NEW: Rise time detection - compare current energy to recent history ===
  // Use the average of the energy history as the "background" level
  double avgPrevEnergy = 0;
  for (int i = 0; i < ENERGY_HISTORY_SIZE; i++) {
    avgPrevEnergy += energyHistory[i];
  }
  avgPrevEnergy /= ENERGY_HISTORY_SIZE;
  
  // Rise factor: how much did snap-band energy jump from background?
  double riseFactor = (avgPrevEnergy > 100) ? (snapFreqEnergy / avgPrevEnergy) : 
                      (snapFreqEnergy > SNAP_ENERGY_THRESHOLD ? 999.0 : 0.0);
  
  // Update energy history ring buffer (BEFORE any early returns)
  energyHistory[energyHistoryIdx] = snapFreqEnergy;
  prevTotalEnergy = totalEnergy;
  energyHistoryIdx = (energyHistoryIdx + 1) % ENERGY_HISTORY_SIZE;
  
  unsigned long now = millis();
  
  // Check cooldown
  if (now - lastActivationTime < ACTIVATION_COOLDOWN_MS) {
    return SNAP_NONE;
  }
  
  // Check for decay after potential snap
  if (waitingForDecay) {
    unsigned long elapsed = now - snapStartTime;
    
    // Max duration check - reject sustained sounds early
    if (elapsed > MAX_SNAP_DURATION_MS) {
      waitingForDecay = false;
      return SNAP_NONE;
    }
    
    if (elapsed > DECAY_TIME_MS) {
      double currentEnergy = snapFreqEnergy;
      double decayRatio = (peakEnergyDuringSnap > 0) ? (currentEnergy / peakEnergyDuringSnap) : 1.0;
      
      if (decayRatio < DECAY_FACTOR) {
        // Good decay - confirms impulsive sound (snap-like)
        snapCount++;
        
        if (snapCount >= 2) {
          unsigned long gap = now - lastSnapTime;
          if (gap >= DOUBLE_SNAP_MIN_GAP_MS && gap <= DOUBLE_SNAP_MAX_GAP_MS) {
            Serial.println();
            // Double snap detected!
            snapCount = 0;
            lastActivationTime = now;
            waitingForDecay = false;
            
            if (doubleSnapCallback) {
              doubleSnapCallback();
            }
            
            return SNAP_DOUBLE;
          } else {
            // Gap out of range, reset to 1
            snapCount = 1;
          }
        }
        
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
    if (snapFreqEnergy > peakEnergyDuringSnap) {
      peakEnergyDuringSnap = snapFreqEnergy;
    }
    
    return SNAP_WAITING_DECAY;
  }
  
  // Reset snap count if too much time passed
  if (snapCount > 0 && (now - lastSnapTime) > DOUBLE_SNAP_MAX_GAP_MS) {
    snapCount = 0;
  }
  
  // === Snap signature checks (original + new temporal/spectral checks) ===
  bool hasEnoughAmplitude = maxAmplitude > AMPLITUDE_THRESHOLD;
  bool hasEnoughSnapEnergy = snapFreqEnergy > SNAP_ENERGY_THRESHOLD;
  bool hasGoodRatio = snapRatio > SNAP_RATIO_THRESHOLD;
  bool hasHighCentroid = spectralCentroid > MIN_SPECTRAL_CENTROID;
  bool hasLowBassEnergy = lowFreqEnergy < MAX_LOW_FREQ_ENERGY;
  bool hasGoodCrestFactor = crestFactor > MIN_CREST_FACTOR;
  bool hasEnoughHighFreq = highFreqRatio > MIN_HIGH_FREQ_RATIO;
  bool hasLowMidFreq = midFreqRatio < MAX_MID_FREQ_RATIO;
  
  // === NEW checks based on spectrogram analysis ===
  bool hasSharpRise = riseFactor > MIN_RISE_FACTOR;           // Sharp onset from silence
  bool hasSpectralConc = spectralConcentration > MIN_SPECTRAL_CONCENTRATION;  // Energy concentrated in snap band
  
  // Core spectral checks (must all pass)
  bool spectralMatch = hasEnoughAmplitude && hasEnoughSnapEnergy && hasGoodRatio &&
                       hasHighCentroid && hasLowBassEnergy && hasGoodCrestFactor &&
                       hasLowMidFreq;
  
  // Temporal / impulsiveness checks (the key improvement)
  bool temporalMatch = hasSharpRise && hasSpectralConc;
  
  bool isLikelySnap = spectralMatch && temporalMatch;
  
  if (isLikelySnap) {
    waitingForDecay = true;
    peakEnergyDuringSnap = snapFreqEnergy;
    snapStartTime = now;
    
    return SNAP_WAITING_DECAY;
  }
  
  return SNAP_NONE;
}
