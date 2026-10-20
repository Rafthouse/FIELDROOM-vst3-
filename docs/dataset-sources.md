# FIELDROOM Dataset Sources & Licensing Manifest

Clean-Room Principle: All data bundled or supported by FIELDROOM originates strictly from open, non-proprietary, redistributable research projects under permissive open licenses (MIT, CC-BY 4.0, or Open Database). No proprietary files from commercial plugins are used.

## 1. Headphone Calibration Profiles

All parametric EQ compensation curves are derived from open measurement repositories:
- **AutoEq Project** by Jaakko Pasanen (MIT License).
- Measurement Rigs:
  - GRAS 43AG / KB0065 (Industry standard pinna simulation).
  - miniDSP E.A.R.S. (Cross-verified where applicable).
- Parametric EQ Format: 10-band minimum-phase biquads (Peaking, Low Shelf at 105 Hz, High Shelf at 10 kHz).
- Automatic Preamp: Calculated as $\min(0, -\max(\text{Filter Gain Response})) - 0.5\text{ dB}$ headroom to strictly prevent digital clipping.

### Bundled Verified Headphone Models:
1. **Sennheiser HD 600** (300Ω Open Dynamic) - Industry mastering reference.
2. **Sennheiser HD 650** (300Ω Open Dynamic) - Warm mid-forward mixing reference.
3. **Sennheiser HD 800 S** (300Ω Open Dynamic) - Ultra-wide soundstage analytical reference.
4. **Beyerdynamic DT 770 Pro (80Ω)** (Closed Dynamic) - Tracking & low-end assessment.
5. **Beyerdynamic DT 880 Pro (250Ω)** (Semi-Open) - Neutral mixing workhorse.
6. **Beyerdynamic DT 990 Pro (250Ω)** (Open Dynamic) - Treble-resolving mixing headphone.
7. **Beyerdynamic DT 1990 Pro (250Ω)** (Open Dynamic) - High-resolution studio flagship.
8. **Audio-Technica ATH-M50x** (38Ω Closed Dynamic) - Ubiquitous modern tracking standard.
9. **Sony MDR-7506** (63Ω Closed Dynamic) - Broadcast & transient inspection reference.
10. **AKG K371** (32Ω Closed Dynamic) - Harman-target tuned closed reference.
11. **AKG K702** (62Ω Open Dynamic) - Critical acoustic & classical monitoring.
12. **Focal Clear MG Professional** (55Ω Open Dynamic) - Premium French dynamic monitoring.
13. **Audeze LCD-X (2021 Revision)** (20Ω Planar Magnetic) - Ultra-low distortion planar reference.
14. **Neumann NDH 20** (150Ω Closed Dynamic) - High isolation studio monitor headphone.
15. **Neumann NDH 30** (120Ω Open Dynamic) - Neumann KH monitor voicing translated to headphones.

## 2. Binaural HRTF & Room Impulses
- **MIT KEMAR Compact Dataset** (Bill Gardner and Keith Martin, MIT Media Lab, Open Research License).
- **Acoustic Research Center SOFA Files** (AES69 compliant, CC-BY-SA).
- **Synthetic Image-Source BRIRs**: Generated in real-time by FIELDROOM's mathematical acoustic engine to ensure infinite parameter scalability without static IR file bloat.
