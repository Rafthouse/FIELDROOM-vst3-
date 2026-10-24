# FIELDROOM Audio Processing Architecture

This document describes what the code does now. Earlier drafts listed features that were never
implemented (DC blocker, NaN guard, partitioned convolution, SOFA HRIR, inverse-square distance
loss); those have been removed from the description.

## 1. Signal chain

```text
  [ STEREO INPUT ]  ──> copied to dry buffer (preallocated)
          │
          ├──────────────────────────────────────────────┐
          ▼                                              │
  [ Binaural loudspeaker matrix ]                        │ dry path
     air absorption, ITD (Woodworth), head shadow,       │ (used by EQ ONLY)
     per-field loudness match                            │
          ▼                                              │
  [ Room: early reflections + 8-line FDN ]               │
          ▼                                              │
  [ Spatial blend ]  <───────────────────────────────────┘
          ▼                    (30 ms fade between virtual speakers and dry path)
  [ Headphone calibration ]  10-band biquad PEQ + automatic preamp, crossfaded model switch
          ▼
  [ Monitoring ]  Normal / Mono (L+R)/2 / Side / L solo / R solo, L/R swap, DIM -18 dB, trim
          ▼
  [ Output limiter ]  threshold -0.3 dBFS, release 100 ms, zero latency
          ▼
  [ STEREO OUTPUT ]
```

Master bypass short-circuits the whole chain: the input passes through unchanged and only the
meters keep running. Leaving bypass resets the spatial and calibration state so no old tail resurfaces.

Notes on the individual stages:

- **Binaural matrix** (`BinauralRenderer`): fractional delays from the speaker geometry, Woodworth ITD,
  contralateral head shadow (low-pass plus level drop around 2.5 kHz), and an air-absorption high shelf
  at 8 kHz. Distance changes only the delay; there is no 1/r gain law.
- **Room** (`RoomEngine`): three early-reflection taps from the room dimensions plus an 8-line FDN with
  Householder mixing and a one-pole damping filter per line. Decay follows the room RT60, scaled by the
  speaker field (near 0.75, far 1.25).
- **Loudness match**: a fixed per-field gain (up to +8.2 dB for the far field). The limiter catches any
  overs this creates.
- **Calibration** (`HeadphoneCalibration`): profiles without a verified calibration are passed through
  at unity gain and never filtered. The automatic preamp is computed from the profile's positive band gains.
- **Metering**: peaks, RMS, and phase correlation are measured before the output limiter, so the clip
  indicator still reports what would have clipped without it.

## 2. Real-time guarantees

1. **No allocation on the audio thread.** Every buffer, delay line and filter bank is sized in
   `prepareToPlay()`. Delay lines are fixed-size power-of-two rings, so wrap-around is a bit mask.
2. **Lock-free parameter handoff.** The editor and host write APVTS parameters. `processBlock()` reads
   them once per block. Calibration requests are packed into one 64-bit atomic. Meter values are
   published through atomics, so the UI reads a consistent snapshot without locks.
3. **Calibration is prepared ahead of time.** `HeadphoneCalibration::prepare()` builds one immutable state
   per bundled profile. A model change copies a fixed-size struct into an idle filter bank and crossfades
   over 512 samples. A request that arrives during a fade is applied after the fade completes, so a fade is
   never cut short.
4. **Variable block sizes.** Host blocks longer than the prepared maximum are processed in chunks.
5. **Denormals.** `juce::ScopedNoDenormals` wraps every block. The build does not use fast-math, so NaN
   and infinity checks behave as written.

## 3. Threading

| Thread | Does | Must not |
|---|---|---|
| Message / UI | Builds the UI, writes parameters, reads meter snapshots | Touch DSP state directly |
| Host / audio | `prepareToPlay`, `processBlock`, `releaseResources` | Allocate after `prepareToPlay`, take locks |

`prepareToPlay()` is the only place that allocates. It is called before audio runs.
