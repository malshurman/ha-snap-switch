#include "iir_filters.h"
#include <math.h>

// Biquad filter state (Direct Form II)
struct BiquadState {
    double x1, x2;  // Input history
    double y1, y2;  // Output history
};

// Three filter states
static BiquadState rejectState = {0, 0, 0, 0};
static BiquadState snapState = {0, 0, 0, 0};
static BiquadState brightState = {0, 0, 0, 0};

// Energy envelopes (exponential moving average)
static double rejectEnergy = 0.0;
static double snapEnergy = 0.0;
static double brightEnergy = 0.0;

// Biquad filter (Direct Form II)
static double processBiquad(double input, BiquadState* state,
                           double b0, double b1, double b2,
                           double a1, double a2) {
    // Compute intermediate value
    double w = input - a1 * state->y1 - a2 * state->y2;

    // Compute output
    double output = b0 * w + b1 * state->y1 + b2 * state->y2;

    // Update state
    state->y2 = state->y1;
    state->y1 = w;

    return output;
}

// Energy envelope detector with attack/release
static double updateEnvelope(double currentEnergy, double newSample) {
    double sampleEnergy = newSample * newSample;
    double alpha = (sampleEnergy > currentEnergy) ? EMA_ATTACK_ALPHA : EMA_RELEASE_ALPHA;
    return alpha * sampleEnergy + (1.0 - alpha) * currentEnergy;
}

void initFilterBank() {
    rejectState = {0, 0, 0, 0};
    snapState = {0, 0, 0, 0};
    brightState = {0, 0, 0, 0};
    rejectEnergy = 0.0;
    snapEnergy = 0.0;
    brightEnergy = 0.0;
}

void processFilterBank(double sample) {
    // Process through all three filters
    double rejectOut = processBiquad(sample, &rejectState,
                                     REJECT_B0, REJECT_B1, REJECT_B2,
                                     REJECT_A1, REJECT_A2);

    double snapOut = processBiquad(sample, &snapState,
                                   SNAP_B0, SNAP_B1, SNAP_B2,
                                   SNAP_A1, SNAP_A2);

    double brightOut = processBiquad(sample, &brightState,
                                     BRIGHT_B0, BRIGHT_B1, BRIGHT_B2,
                                     BRIGHT_A1, BRIGHT_A2);

    // Update energy envelopes
    rejectEnergy = updateEnvelope(rejectEnergy, rejectOut);
    snapEnergy = updateEnvelope(snapEnergy, snapOut);
    brightEnergy = updateEnvelope(brightEnergy, brightOut);
}

double getRejectEnergy() {
    return rejectEnergy;
}

double getSnapEnergy() {
    return snapEnergy;
}

double getBrightEnergy() {
    return brightEnergy;
}

void resetFilterBank() {
    initFilterBank();
}
