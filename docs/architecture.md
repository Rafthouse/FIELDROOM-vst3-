# FIELDROOM Audio Processing Architecture

## 1. High-Level DSP Pipeline

The audio thread processes stereo audio in contiguous blocks with deterministic $O(N)$ real-time execution.

```text
                  [ STEREO DAW INPUT ]
                            │
               [ Input Gain & Safety Guard ]
              (Denormal clamp, DC blocker, NaN)
                            │
                   [ Monitoring Mode ]
           (Mono, Side, L Solo, R Solo, L/R Swap)
                            │
              [ Virtual Loudspeaker Geometry ]
        (Near: 1.1m, Mid: 2.2m, Far: 4.2m / 60° triangle)
      (Inverse-Square loss, HF air absorption, Doppler guard)
                            │
                [ Binaural Spatialization ]
      (Woodworth-Schlosser spherical head model / SOFA HRIR)
              (Direct sound + ITD + ILD + Pinna cues)
                            │
               [ Room Acoustic Simulation ]
        (Early reflections: Image-source ray tracing)
         (Late decay: 8-delay unitary Feedback Delay Network)
                            │
            [ Loudness Matching Compensation ]
        (Deterministic ±0.5 dB loudness equalization)
                            │
             [ Headphone Calibration Engine ]
     (10-band minimum-phase biquad PEQ + AutoEq curves)
          (Automatic negative pre-amp gain matching)
                            │
               [ Output Trim & DIM (-18dB) ]
                            │
            [ Metering (True Peak, RMS, Phase) ]
                            │
                 [ HEADPHONE STEREO OUTPUT ]
```

## 2. Real-Time Thread Safety Guarantees

1. **Zero Heap Allocation**: All memory buffers, delay lines, FFT workspaces, and filter states are allocated during `prepareToPlay()` and locked in physical memory.
2. **Lock-Free State Transfer**: Parameter adjustments and profile changes are staged on the message/UI thread into atomic structures and applied smoothly in the audio thread via linear and exponential smoothers (`juce::SmoothedValue`).
3. **Partitioned Convolution**: Convolution buffers are partitioned into sub-blocks of size 128 samples, yielding deterministic execution time with zero spikes.
4. **Denormal & Exception Safety**: `juce::ScopedNoDenormals` active across all audio blocks to eliminate CPU scaling slowdowns.
