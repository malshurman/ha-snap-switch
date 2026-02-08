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
// SNAP DETECTION PARAMETERS
// Finger snaps are extremely impulsive:
// - Main transient is only ~10-50ms
// - Strong energy concentrated in 2000-6000 Hz
// - Near-instant rise from silence to peak
// - Very fast decay back to silence
// - High spectral centroid (brightness)
// - Low energy below 500Hz
// - High crest factor (peaky waveform)
// ============================================
#define SNAP_FREQ_LOW      1500   // Lower bound of snap frequency (Hz)
#define SNAP_FREQ_HIGH     6000   // Upper bound of snap frequency (Hz) - snaps extend to ~6kHz
#define SNAP_ENERGY_THRESHOLD  1500000  // Minimum energy in snap frequency band (reduced for sensitivity)
#define SNAP_RATIO_THRESHOLD   1.3      // Snap band must be X times stronger than low freq (reduced)
#define AMPLITUDE_THRESHOLD    4000000  // Minimum raw amplitude to consider (reduced)
#define DECAY_TIME_MS          50       // Snap must decay within this time (tighter for sharp snaps)
#define DECAY_FACTOR           0.3      // Energy must drop to this fraction to confirm snap (stricter)

// Impulsiveness / temporal checks (key improvement for sharp transients)
#define MIN_RISE_FACTOR        5.0      // Current snap energy must be Nx the previous frame (reduced)
#define MAX_SNAP_DURATION_MS   150      // Reject sounds lasting longer than this
#define MIN_SPECTRAL_CONCENTRATION 0.30 // Min fraction of total energy in snap band (reduced)
#define ENERGY_HISTORY_SIZE    4        // Number of previous frames to track

// Additional snap signature checks
#define MIN_SPECTRAL_CENTROID  2000     // Minimum spectral centroid (Hz) - snaps are "bright"
#define MAX_LOW_FREQ_ENERGY    3000000  // Reject if too much bass (voice, music)
#define MIN_CREST_FACTOR       3.0      // Peak/RMS ratio - snaps are very peaky
#define MIN_HIGH_FREQ_RATIO    0.10     // Minimum ratio of 6kHz+ energy to total (lowered since band widened)
#define MAX_MID_FREQ_RATIO     0.35     // Maximum ratio of 500-1500Hz (voice range)

// ============================================
// DOUBLE SNAP DETECTION
// ============================================
#define DOUBLE_SNAP_MIN_GAP_MS   150   // Minimum time between snaps
#define DOUBLE_SNAP_MAX_GAP_MS   800   // Maximum time between snaps
#define ACTIVATION_COOLDOWN_MS   2000  // Cooldown after activation

#endif // CONFIG_H
