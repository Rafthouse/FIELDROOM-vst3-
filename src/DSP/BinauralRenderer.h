#pragma once

#include <vector>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "SpeakerGeometry.h"
#include "HRTFManager.h"

namespace fieldroom {

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
    double currentSampleRate{48000.0};
    int currentField{1}; // 0 = Near, 1 = Mid, 2 = Far
    float currentSpreadDeg{60.0f}; // Included angle (30° - 110°)
    float currentRotationDeg{0.0f};
    bool loudnessMatchEnabled{true};

    // Fractional delay lines for ipsilateral & contralateral loudspeaker propagation
    static constexpr int MaxDelaySamples = 4096;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex{0};

    // Lowpass head-shadow biquad filters for contralateral diffraction
    juce::dsp::IIR::Filter<float> shadowFilterLL; // Left speaker -> Left ear
    juce::dsp::IIR::Filter<float> shadowFilterLR; // Left speaker -> Right ear (contralateral)
    juce::dsp::IIR::Filter<float> shadowFilterRL; // Right speaker -> Left ear (contralateral)
    juce::dsp::IIR::Filter<float> shadowFilterRR; // Right speaker -> Right ear

    // Air absorption high-shelf / lowpass for distance propagation
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
    float readInterpolatedDelay(const std::vector<float>& buf, float delayInSamples) const noexcept;
};

} // namespace fieldroom
