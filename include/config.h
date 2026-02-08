#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// ============================================
#define I2S_WS   25   // Word Select (LRCLK)
#define I2S_SD   32   // Serial Data
#define I2S_SCK  33   // Serial Clock (BCLK)
#define I2S_PORT I2S_NUM_0

// ============================================
// AUDIO SETTINGS
// ============================================
#define SAMPLES 512
#define SAMPLING_FREQ 16000

// ============================================
// SNAP DETECTION PARAMETERS (IIR Filter-Based)
// Finger snaps are extremely impulsive:
// - Main transient is only ~10-50ms
// - Strong energy concentrated in 1500-6000 Hz
// - Near-instant rise from silence to peak
// - Very fast decay back to silence
// - High-frequency brightness (>6kHz)
// - Low energy below 500Hz (reject filter)
// - High crest factor (peaky waveform)
// ============================================
#define SNAP_FREQ_LOW          1500     // Lower bound of snap frequency (Hz) - for display only
#define SNAP_FREQ_HIGH         6000     // Upper bound of snap frequency (Hz) - for display only
#define SNAP_ENERGY_THRESHOLD  5000     // Minimum energy in snap band (tune empirically)
#define SNAP_RATIO_THRESHOLD   1.5      // Snap band must be X times stronger than reject band
#define AMPLITUDE_THRESHOLD    5000000  // Minimum raw amplitude to consider
#define DECAY_TIME_MS          70       // Time to check for decay confirmation
#define DECAY_FACTOR           0.3      // Energy must drop to this fraction to confirm snap

// Anti-sustain logic (rejects toilet flushes, running water)
#define SUSTAIN_CHECK_TIME_MS  50       // Check for sustained energy after this time
#define SUSTAIN_THRESHOLD      0.7      // Reject if energy stays >70% of peak

// Impulsiveness / temporal checks
#define MIN_RISE_FACTOR        4.0      // Sharp energy jump from background
#define MAX_SNAP_DURATION_MS   150      // Reject sounds lasting longer than this
#define ENERGY_HISTORY_SIZE    4        // Number of previous frames to track
#define FLUX_THRESHOLD         2000     // Spectral flux for sudden changes (tune empirically)

// Simplified IIR-based checks
#define MIN_CREST_FACTOR       3.0      // Peak/RMS ratio - snaps are very peaky
#define MIN_HIGH_FREQ_RATIO    0.10     // Minimum ratio of bright band (>6kHz) to total energy

// ============================================
// DOUBLE SNAP DETECTION
// ============================================
#define DOUBLE_SNAP_MIN_GAP_MS   150   // Minimum time between snaps
#define DOUBLE_SNAP_MAX_GAP_MS   800   // Maximum time between snaps
#define ACTIVATION_COOLDOWN_MS   2000  // Cooldown after activation

#endif // CONFIG_H
