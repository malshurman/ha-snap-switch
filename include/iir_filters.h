#ifndef IIR_FILTERS_H
#define IIR_FILTERS_H

// Butterworth 2nd-order filter coefficients for fs=16000 Hz
// Generated with scipy.signal.butter (verified stable)

// REJECT Band: HPF @ 500Hz (removes rumble, voice fundamentals, door slams)
#define REJECT_B0  0.870330779310
#define REJECT_B1  -1.740661558621
#define REJECT_B2  0.870330779310
#define REJECT_A1  -1.723776172763
#define REJECT_A2  0.757546944479

// SNAP Band Stage 1: HPF @ 1500Hz (lower cutoff)
#define SNAP_HP_B0  0.657455191491
#define SNAP_HP_B1  -1.314910382982
#define SNAP_HP_B2  0.657455191491
#define SNAP_HP_A1  -1.193913367721
#define SNAP_HP_A2  0.435907398244

// SNAP Band Stage 2: LPF @ 3500Hz (upper cutoff)
#define SNAP_LP_B0  0.237643994385
#define SNAP_LP_B1  0.475287988770
#define SNAP_LP_B2  0.237643994385
#define SNAP_LP_A1  -0.230396252687
#define SNAP_LP_A2  0.180972230228

// TRANSIENT Band: HPF @ 3000Hz (sharp attack detection)
#define TRANSIENT_B0  0.418163345762
#define TRANSIENT_B1  -0.836326691524
#define TRANSIENT_B2  0.418163345762
#define TRANSIENT_A1  -0.462938025291
#define TRANSIENT_A2  0.209715357757

// Energy envelope parameters
#define EMA_ATTACK_ALPHA   0.6   // Faster attack for transients (~2ms)
#define EMA_RELEASE_ALPHA  0.1   // Slower release (20ms)

void initFilterBank();
void processFilterBank(double sample);
double getRejectEnergy();
double getSnapEnergy();
double getTransientEnergy();
void resetFilterBank();

#endif
