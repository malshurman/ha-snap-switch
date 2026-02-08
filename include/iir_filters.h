#ifndef IIR_FILTERS_H
#define IIR_FILTERS_H

// Butterworth 2nd-order filter coefficients for fs=16000 Hz

// Reject Band: HPF @ 500Hz (removes rumble, voice fundamentals)
#define REJECT_B0  0.870330779310
#define REJECT_B1  -1.740661558621
#define REJECT_B2  0.870330779310
#define REJECT_A1  -1.723776172763
#define REJECT_A2  0.757546944479

// Snap Band: BPF @ 1500-6000Hz (primary snap energy)
#define SNAP_B0    0.352842120729
#define SNAP_B1    0.000000000000
#define SNAP_B2    -0.352842120729
#define SNAP_A1    -0.903615366026
#define SNAP_A2    0.294315758543

// Bright Band: HPF @ 6000Hz (brightness confirmation)
#define BRIGHT_B0  0.097631072938
#define BRIGHT_B1  -0.195262145876
#define BRIGHT_B2  0.097631072938
#define BRIGHT_A1  0.942809041582
#define BRIGHT_A2  0.333333333333

// Energy envelope parameters
#define EMA_ATTACK_ALPHA   0.3   // Fast attack (5ms)
#define EMA_RELEASE_ALPHA  0.1   // Slower release (20ms)

void initFilterBank();
void processFilterBank(double sample);
double getRejectEnergy();
double getSnapEnergy();
double getBrightEnergy();
void resetFilterBank();

#endif
