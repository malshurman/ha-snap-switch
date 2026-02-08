#include "iir_filters.h"
#include <math.h>

// Biquad filter state (Direct Form II)
struct BiquadState {
    double y1, y2;  // Delay line for Direct Form II
};

// Four filter states (reject + snap_hp + snap_lp + transient)
static BiquadState rejectState = {0, 0};
static BiquadState snapHpState = {0, 0};
static BiquadState snapLpState = {0, 0};
static BiquadState transientState = {0, 0};

// Energy envelopes (exponential moving average)
static double rejectEnergy = 0.0;
static double snapEnergy = 0.0;
static double transientEnergy = 0.0;

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
    rejectState = {0, 0};
    snapHpState = {0, 0};
    snapLpState = {0, 0};
    transientState = {0, 0};
    rejectEnergy = 0.0;
    snapEnergy = 0.0;
    transientEnergy = 0.0;
}

void processFilterBank(double sample) {
    // Stage 1: Reject low frequencies
    double rejectOut = processBiquad(sample, &rejectState,
                                     REJECT_B0, REJECT_B1, REJECT_B2,
                                     REJECT_A1, REJECT_A2);

    // Stage 2a: SNAP band high-pass @ 1500 Hz
    double snapHpOut = processBiquad(sample, &snapHpState,
                                     SNAP_HP_B0, SNAP_HP_B1, SNAP_HP_B2,
                                     SNAP_HP_A1, SNAP_HP_A2);

    // Stage 2b: SNAP band low-pass @ 3500 Hz (cascaded)
    double snapOut = processBiquad(snapHpOut, &snapLpState,
                                   SNAP_LP_B0, SNAP_LP_B1, SNAP_LP_B2,
                                   SNAP_LP_A1, SNAP_LP_A2);

    // Stage 3: TRANSIENT band @ 3000 Hz
    double transientOut = processBiquad(sample, &transientState,
                                        TRANSIENT_B0, TRANSIENT_B1, TRANSIENT_B2,
                                        TRANSIENT_A1, TRANSIENT_A2);

    // Update energy envelopes
    rejectEnergy = updateEnvelope(rejectEnergy, rejectOut);
    snapEnergy = updateEnvelope(snapEnergy, snapOut);
    transientEnergy = updateEnvelope(transientEnergy, transientOut);
}

double getRejectEnergy() {
    return rejectEnergy;
}

double getSnapEnergy() {
    return snapEnergy;
}

double getTransientEnergy() {
    return transientEnergy;
}

void resetFilterBank() {
    initFilterBank();
}
