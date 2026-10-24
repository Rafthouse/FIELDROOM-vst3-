#pragma once

#include <array>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "SpeakerGeometry.h"
#include "HRTFManager.h"

namespace fieldroom {

/**
 * Virtual loudspeaker to binaural renderer (2x2 speaker-to-ear matrix).
 *
 * All delay lines are fixed-size, power-of-two ring buffers allocated as members, so prepare() and
 * process() never allocate and wrap-around is a bit mask instead of a modulo.
 */
class BinauralRenderer {
public:
    BinauralRenderer();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setSpeakerField(int fieldIndex);
    void setSpeakerSpread(float spreadDegrees);
    void setHeadRotation(float rotationDegrees);
    void setLoudnessMatch(bool enable);

    void process(const float* inL, const float* inR,
                 float* outL, float* outR, int numSamples);

    int getField() const noexcept { return currentField; }
    float getSpeakerSpread() const noexcept { return currentSpreadDeg; }
    float getHeadRotation() const noexcept { return currentRotationDeg; }

private:
    static constexpr int MaxDelaySamples = 4096;               // ~85 ms at 48 kHz, far beyond any field
    static constexpr int DelayBufferSize = 8192;                // power of two >= MaxDelaySamples + 1
    static constexpr int DelayMask = DelayBufferSize - 1;

    double currentSampleRate{48000.0};
    int currentField{1}; // 0 = Near, 1 = Mid, 2 = Far
    float currentSpreadDeg{60.0f}; // Included angle (30° - 110°)
    float currentRotationDeg{0.0f};
    bool loudnessMatchEnabled{true};

    // Ring buffers for ipsilateral & contralateral loudspeaker propagation
    std::array<float, DelayBufferSize> delayBufferL{};
    std::array<float, DelayBufferSize> delayBufferR{};
    int writeIndex{0};

    // Lowpass head-shadow filters for the contralateral paths
    juce::dsp::IIR::Filter<float> shadowFilterLR; // Left speaker -> Right ear (contralateral)
    juce::dsp::IIR::Filter<float> shadowFilterRL; // Right speaker -> Left ear (contralateral)

    // Air absorption high-shelf for distance propagation
    juce::dsp::IIR::Filter<float> airAbsorptionL;
    juce::dsp::IIR::Filter<float> airAbsorptionR;

    // Smoothed acoustic geometry parameters
    juce::SmoothedValue<float> delayLeftIpsi;
    juce::SmoothedValue<float> delayLeftContra;
    juce::SmoothedValue<float> delayRightIpsi;
    juce::SmoothedValue<float> delayRightContra;

    juce::SmoothedValue<float> gainLeftIpsi;
    juce::SmoothedValue<float> gainLeftContra;
    juce::SmoothedValue<float> gainRightIpsi;
    juce::SmoothedValue<float> gainRightContra;

    juce::SmoothedValue<float> loudnessGainSmoother;

    void updateGeometry();
    float readInterpolatedDelay(const std::array<float, DelayBufferSize>& buf, float delayInSamples) const noexcept;
};

} // namespace fieldroom
