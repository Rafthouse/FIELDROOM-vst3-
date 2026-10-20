# Headphone Calibration Targets & Binaural Compatibility

## 1. The Target Compatibility Challenge

In binaural monitoring software, a common engineering error is naively combining an arbitrary headphone EQ curve with a room impulse response:

1. **Diffuse-Field Target (ISO 11904-1)**: Equalizes headphones to sound flat in a completely diffuse reverberant field. When listening to binaural BRIRs captured with an acoustic dummy head (e.g., KEMAR), using diffuse-field equalized headphones prevents double-counting ear canal / concha resonances.
2. **Harman Target (AES 2013-2019)**: An empirical consumer and studio preference curve with boosted low-end shelf (+4 to +6 dB below 105 Hz) and smoothed ear gain.
3. **Free-Field Target**: Equalizes headphones to match a single frontal loudspeaker in an anechoic chamber.

## 2. FIELDROOM's Calibration Strategy

FIELDROOM avoids "double equalization" through a dual-mode target architecture:

- **Binaural Mode (Default for Monitor Simulation)**:
  Applies headphone compensation designed specifically for binaural BRIR playback. It corrects non-linearities of the headphone driver and ear coupling relative to the neutral dummy head listening reference.
- **Harman Reference Target**:
  Used when **Headphone EQ Only** mode is engaged, converting non-standard headphones to the Harman 2018 AE/OE reference target curve for direct, non-spatialised audio listening.
- **Headroom Management**:
  Every profile automatically applies negative digital preamp gain equal to the maximum positive peak boost plus $0.5\text{ dB}$ safety margin. Users can override to manual preamp when working with 32-bit floating point DAW mixers.
