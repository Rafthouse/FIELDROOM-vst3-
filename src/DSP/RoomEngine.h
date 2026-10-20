#pragma once

#include <vector>
#include <array>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "SpeakerGeometry.h"

namespace fieldroom {

struct RoomAcousticParameters {
    const char* name;
    float rt60Seconds;           // Reverberation time
    float earlyReflGainDb;       // Early reflection energy level
    float dampingCutoffHz;       // HF absorption rolloff
    float roomWidthMeters;
    float roomLengthMeters;
    float roomHeightMeters;
    float defaultDrrDb;
};

class RoomEngine {
public:
    static constexpr int NumDelays = 8;
    // Mutually prime delay lengths in milliseconds to avoid modal resonances
    static constexpr std::array<float, NumDelays> DelayTimesMs = {
        29.3f, 37.1f, 43.7f, 53.3f, 61.9f, 71.1f, 83.9f, 97.7f
    };

    RoomEngine();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setRoomProfile(int profileIndex);
    void setRoomAmount(float amount01);
    void setSpeakerField(int fieldIndex);

    void process(const float* leftIn, const float* rightIn,
                 float* leftOut, float* rightOut, int numSamples);

    static RoomAcousticParameters getRoomParameters(int profileIndex) noexcept;

private:
    double currentSampleRate{48000.0};
    int currentProfile{2};
    float currentRoomAmount{0.5f};
    int currentField{1};

    // Feedback Delay Network State
    std::array<std::vector<float>, NumDelays> delayBuffers;
    std::array<int, NumDelays> writePointers{{0}};
    std::array<int, NumDelays> delayLengthsInSamples{{0}};
    std::array<float, NumDelays> feedbackGains{{0.0f}};
    std::array<float, NumDelays> onePoleLowpassState{{0.0f}};

    // Early Reflection Delay Lines
    std::vector<float> erBufferLeft;
    std::vector<float> erBufferRight;
    int erWritePos{0};

    juce::SmoothedValue<float> wetMixSmoother;
    void updateFdnCoefficients();
};

} // namespace fieldroom
