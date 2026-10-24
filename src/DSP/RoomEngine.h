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
};

/**
 * Early reflections plus an 8-line Feedback Delay Network (Householder mixing) for the late tail.
 *
 * Every delay line is a power-of-two ring buffer sized in prepare(), so process() never allocates
 * and wrap-around is a bit mask instead of a modulo.
 */
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

    // RT60 actually rendered for a room/field pair (includes the speaker-field scale)
    static float getEffectiveRt60(int profileIndex, int fieldIndex) noexcept;

private:
    static constexpr int MaxEarlySamples = 8192; // early taps are at most ~20 ms; 8192 covers 192 kHz with margin

    void updateFdnCoefficients();

    double currentSampleRate{48000.0};
    int currentProfile{2};
    float currentRoomAmount{0.5f};
    int currentField{1};

    // Feedback Delay Network state (each line is a power-of-two ring, see prepare())
    std::array<std::vector<float>, NumDelays> delayBuffers;
    std::array<int, NumDelays> delayMasks{{0}};
    std::array<int, NumDelays> writePointers{{0}};
    std::array<int, NumDelays> delayLengthsInSamples{{0}};
    std::array<float, NumDelays> feedbackGains{{0.0f}};
    std::array<float, NumDelays> onePoleLowpassState{{0.0f}};

    // Early reflection delay lines (shared ring, power-of-two)
    std::array<float, MaxEarlySamples> erBufferLeft{};
    std::array<float, MaxEarlySamples> erBufferRight{};
    int erWritePos{0};

    juce::SmoothedValue<float> wetMixSmoother;
};

} // namespace fieldroom
