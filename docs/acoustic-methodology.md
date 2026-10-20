# FIELDROOM Acoustic Methodology & Physical Models

## 1. Stereo Loudspeaker-to-Ear Transfer Matrix

In real room listening, each ear receives energy from both virtual speakers:
- **Ipsilateral Path**: Direct speaker to ear on the same side ($H_{LL}, H_{RR}$).
- **Contralateral Path**: Cross-talk path across the skull ($H_{RL}, H_{LR}$).

$$\begin{aligned}
Y_L(\omega) &= X_L(\omega) H_{LL}(\omega) + X_R(\omega) H_{RL}(\omega) \\
Y_R(\omega) &= X_L(\omega) H_{LR}(\omega) + X_R(\omega) H_{RR}(\omega)
\end{aligned}$$

## 2. Spherical Head Model & Binaural Cues

For an acoustic source at angle $\theta$ relative to the listener's head with radius $a = 0.0875\text{ m}$ (standard human head radius) and speed of sound $c = 343\text{ m/s}$:

### Interaural Time Difference (Woodworth-Schlosser Formula):
$$\text{ITD}(\theta) = \frac{a}{c} (\sin|\theta| + |\theta|)$$

For $60^\circ$ standard equilateral triangle listening ($\theta = 30^\circ$ relative to front axis):
$$\text{ITD}(30^\circ) = \frac{0.0875}{343} \left(\sin 30^\circ + \frac{\pi}{6}\right) \approx 0.255\text{ ms} \times (0.5 + 0.5236) \approx 261\ \mu\text{s}$$

### Interaural Level Difference & Head Shadowing:
Contralateral acoustic paths undergo head shadow diffraction modeled as a continuous one-pole one-zero lowpass filter:
$$H_{\text{shadow}}(s) = \frac{1 + s / \omega_z}{1 + s / \omega_p}$$
where cutoff frequencies shift dynamically with source azimuth and head rotation angle $\phi \in [-90^\circ, +90^\circ]$.

## 3. Physical Distinction Between NEAR, MID, and FAR

Unlike simple plugins that only adjust dry/wet or volume, FIELDROOM modulates four physical acoustic variables:

| Acoustic Variable | NEARFIELD | MIDFIELD | FARFIELD |
|---|---|---|---|
| **Distance ($r$)** | 1.1 meters | 2.2 meters | 4.2 meters |
| **Direct Sound Delay** | $3.2\text{ ms}$ | $6.4\text{ ms}$ | $12.2\text{ ms}$ |
| **Direct-to-Reverberant Ratio (DRR)** | $+14\text{ dB}$ (Dry, upfront, analytical) | $+7\text{ dB}$ (Balanced, cohesive) | $+1.5\text{ dB}$ (Immersive room interaction) |
| **Air Absorption (HF Dissipation)** | Flat up to $20\text{ kHz}$ | $-0.5\text{ dB}$ at $16\text{ kHz}$ | $-1.8\text{ dB}$ at $16\text{ kHz}$ |
| **First Wall Reflection Delay** | $\Delta t \approx 4.8\text{ ms}$ | $\Delta t \approx 7.2\text{ ms}$ | $\Delta t \approx 11.5\text{ ms}$ |
| **Phantom Center Exteriorization** | Intimate, $0.9\text{ m}$ forward | Natural, $2.0\text{ m}$ forward | Expansive, $3.8\text{ m}$ forward |
| **Loudness Normalization Offset** | $0.0\text{ dB}$ | $+4.5\text{ dB}$ | $+8.2\text{ dB}$ |

## 4. Room Engine: Hybrid Image-Source + 8-Delay FDN

1. **Early Reflections (0 - 50 ms)**: Calculated geometrically using the 1st and 2nd order image source method from room boundaries (Floor, Ceiling, Left Wall, Right Wall, Front Wall, Rear Wall). Each boundary has frequency-dependent absorption coefficients $(\alpha_{\text{low}}, \alpha_{\text{mid}}, \alpha_{\text{high}})$.
2. **Late Diffuse Reverberation (50 - 1200 ms)**: Synthesized using an 8-channel Feedback Delay Network (FDN) with an orthogonal Householder mixing matrix $A = I - \frac{2}{N} \mathbf{1}\mathbf{1}^T$. Mutually prime delay lengths prevent metallic resonances and preserve dense modal distribution.
