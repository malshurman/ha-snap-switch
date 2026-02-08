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
// Research-based: snaps concentrate at 1500-3500 Hz, peak 2kHz
// Duration ~7ms, ultrafast attack, single impulse
// ============================================
#define SNAP_FREQ_LOW          1500     // Lower bound (Hz) - for display
#define SNAP_FREQ_HIGH         3500     // Upper bound (Hz) - for display
#define SNAP_ENERGY_THRESHOLD  3000     // Min energy in snap band (start conservative)
#define SNAP_RATIO_THRESHOLD   1.5      // Snap/reject ratio (diagnostic only)
#define AMPLITUDE_THRESHOLD    5000000  // Min raw amplitude
#define DECAY_TIME_MS          70       // Wait before checking decay
#define DECAY_FACTOR           0.3      // Must drop to <30% to confirm snap

// Anti-sustain logic (rejects toilet flushes, running water)
#define SUSTAIN_CHECK_TIME_MS  50       // Check for sustained energy
#define SUSTAIN_THRESHOLD      0.7      // Reject if >70% of peak

// Transient detection (critical for snap vs sustained sounds)
#define MIN_RISE_FACTOR        4.0      // Sharp energy jump from background
#define MAX_SNAP_DURATION_MS   150      // Reject sounds lasting longer
#define ENERGY_HISTORY_SIZE    4        // Previous frames to track
#define FLUX_THRESHOLD         1500     // Spectral flux for sudden changes

// Optional checks (diagnostic)
#define MIN_CREST_FACTOR       3.0      // Peak/RMS ratio
#define TRANSIENT_THRESHOLD    500      // Min transient band energy (3kHz+)

// ============================================
// DOUBLE SNAP DETECTION
// ============================================
#define DOUBLE_SNAP_MIN_GAP_MS   150   // Minimum time between snaps
#define DOUBLE_SNAP_MAX_GAP_MS   800   // Maximum time between snaps
#define ACTIVATION_COOLDOWN_MS   2000  // Cooldown after activation

#endif // CONFIG_H
