#!/usr/bin/env python3
"""Generate stable IIR filter coefficients for snap detection."""

from scipy import signal
import numpy as np

def print_coefficients(name, sos):
    """Print coefficients in C header format."""
    b = sos[0, :3]
    a = [1.0, sos[0, 4], sos[0, 5]]

    print(f"\n// {name}")
    print(f"#define {name}_B0  {b[0]:.12f}")
    print(f"#define {name}_B1  {b[1]:.12f}")
    print(f"#define {name}_B2  {b[2]:.12f}")
    print(f"#define {name}_A1  {a[1]:.12f}")
    print(f"#define {name}_A2  {a[2]:.12f}")

    # Verify stability
    poles = np.roots([1, a[1], a[2]])
    pole_mags = np.abs(poles)
    print(f"// Poles: {poles}")
    print(f"// |z| = {pole_mags}")
    assert all(pole_mags < 1), f"{name} UNSTABLE!"
    print(f"// ✓ STABLE")

fs = 16000

print("// ============================================")
print("// SNAP DETECTION FILTER COEFFICIENTS")
print("// Sampling rate: 16000 Hz")
print("// Generated with scipy.signal.butter")
print("// ============================================")

# SNAP band high-pass @ 1500 Hz
sos_snap_hp = signal.butter(2, 1500, 'high', fs=fs, output='sos')
print_coefficients("SNAP_HP", sos_snap_hp)

# SNAP band low-pass @ 3500 Hz
sos_snap_lp = signal.butter(2, 3500, 'low', fs=fs, output='sos')
print_coefficients("SNAP_LP", sos_snap_lp)

# TRANSIENT band high-pass @ 3000 Hz
sos_transient = signal.butter(2, 3000, 'high', fs=fs, output='sos')
print_coefficients("TRANSIENT", sos_transient)
