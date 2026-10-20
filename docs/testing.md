# FIELDROOM Verification & Automated Testing Specification

## 1. Automated Test Suite Matrix

The C++ unit test suite (`tests/dsp/test_dsp_core.cpp`) runs headlessly in CI and local machines.

| Test ID | Test Category | Target Assertion | Pass Threshold |
|---|---|---|---|
| **DSP-01** | Channel Isolation & Symmetry | Symmetrical HRTF when head rotation $\phi = 0^\circ$ | $\|H_{LL} - H_{RR}\| < 10^{-6}$ |
| **DSP-02** | Contralateral Delay & ITD | Contralateral ear lag matches Woodworth model at $30^\circ$ | $261\ \mu\text{s} \pm 10\ \mu\text{s}$ |
| **DSP-03** | Near / Mid / Far Differentiation | Measurable Direct-to-Reverberant and early delay shift | $\Delta\text{Delay}_{\text{Mid-Near}} \ge 3.0\text{ ms}$, $\text{DRR}_{\text{Near}} > \text{DRR}_{\text{Mid}} > \text{DRR}_{\text{Far}}$ |
| **DSP-04** | PEQ Filter Stability | Biquad poles stay inside the unit circle for all $f_0 \in [20, 20000]\text{ Hz}$ | Radius $|z| < 1.0$, Zero NaN / Inf |
| **DSP-05** | Preamp Headroom Safety | Calibration curve maximum gain plus preamp does not exceed $0\text{ dBFS}$ | Peak $\le 0.0\text{ dBFS}$ |
| **DSP-06** | Monitoring Modes | Mono sum attenuation equals $-3\text{ dB}$; Side $= [L-R]/2$ | Exact arithmetic |
| **DSP-07** | Head Rotation Sweep | Smooth transition without DC offset or phase discontinuity | Zero clicks / discontinuities |
| **DSP-08** | Real-Time Latency | Processing buffer latency reported to host | $\le 256$ samples at $48\text{ kHz}$ |

## 2. Plugin Validation (`pluginval`)
FIELDROOM is validated using Tracktion's industry-standard `pluginval` test runner up to strictness level 10:
```bash
pluginval --strictness-level 10 --validate-in-process --validate "build/FIELDROOM_artefacts/Release/VST3/FIELDROOM.vst3"
```
Ensures:
- Fuzz testing of rapid buffer size changes (16, 32, 64, 128, 256, 512, 1024, 2048 samples).
- Sample rate jumps (44.1 kHz, 48 kHz, 88.2 kHz, 96 kHz, 192 kHz).
- APVTS parameter automation concurrency safety.
- Complete state serialization recall.
