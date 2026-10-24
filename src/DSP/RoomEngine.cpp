#include "RoomEngine.h"
#include <algorithm>

namespace fieldroom {

RoomAcousticParameters RoomEngine::getRoomParameters(int profileIndex) noexcept {
    switch (profileIndex) {
        case 0: // Neutral Reference
            return {"Neutral Reference", 0.15f, -18.0f, 9000.0f, 4.5f, 5.5f, 2.8f};
        case 1: // Small Studio
            return {"Small Studio", 0.28f, -12.0f, 6500.0f, 4.2f, 5.0f, 2.6f};
        case 2: // Professional Control Room
            return {"Professional Control Room", 0.38f, -9.0f, 5200.0f, 6.2f, 7.8f, 3.2f};
        case 3: // Large Control Room
            return {"Large Control Room", 0.58f, -6.5f, 4400.0f, 8.5f, 11.0f, 4.0f};
        case 4: // Domestic Listening Room
            return {"Domestic Listening Room", 0.48f, -8.0f, 3800.0f, 4.8f, 6.2f, 2.5f};
        default:
            return {"Professional Control Room", 0.38f, -9.0f, 5200.0f, 6.2f, 7.8f, 3.2f};
    }
}

float RoomEngine::getEffectiveRt60(int profileIndex, int fieldIndex) noexcept {
    float rt60 = getRoomParameters(profileIndex).rt60Seconds;

    // Field-dependent scale
    if (fieldIndex == 0) rt60 *= 0.75f;      // Nearfield shorter perceived tail
    else if (fieldIndex == 2) rt60 *= 1.25f; // Farfield energizes room more
    return rt60;
}

namespace {
int nextPowerOfTwo(int value) noexcept {
    int p = 1;
    while (p < value) p <<= 1;
    return p;
}
} // namespace

RoomEngine::RoomEngine() {
    wetMixSmoother.setCurrentAndTargetValue(0.5f);
}

void RoomEngine::prepare(double sampleRate, int maxBlockSize) {
    juce::ignoreUnused(maxBlockSize);
    currentSampleRate = sampleRate;
    wetMixSmoother.reset(sampleRate, 0.05); // 50 ms ramp

    for (int i = 0; i < NumDelays; ++i) {
        const int len = std::max(64, static_cast<int>(DelayTimesMs[static_cast<size_t>(i)] * 0.001f * static_cast<float>(sampleRate)));
        delayLengthsInSamples[static_cast<size_t>(i)] = len;

        // Ring holds len + 1 samples, rounded up so wrap-around is a mask
        const int size = nextPowerOfTwo(len + 1);
        delayBuffers[static_cast<size_t>(i)].assign(static_cast<size_t>(size), 0.0f);
        delayMasks[static_cast<size_t>(i)] = size - 1;
        writePointers[static_cast<size_t>(i)] = 0;
        onePoleLowpassState[static_cast<size_t>(i)] = 0.0f;
    }

    erBufferLeft.fill(0.0f);
    erBufferRight.fill(0.0f);
    erWritePos = 0;

    updateFdnCoefficients();
}

void RoomEngine::reset() {
    for (int i = 0; i < NumDelays; ++i) {
        std::fill(delayBuffers[static_cast<size_t>(i)].begin(), delayBuffers[static_cast<size_t>(i)].end(), 0.0f);
        writePointers[static_cast<size_t>(i)] = 0;
        onePoleLowpassState[static_cast<size_t>(i)] = 0.0f;
    }
    erBufferLeft.fill(0.0f);
    erBufferRight.fill(0.0f);
    erWritePos = 0;
}

void RoomEngine::setRoomProfile(int profileIndex) {
    if (currentProfile != profileIndex) {
        currentProfile = profileIndex;
        updateFdnCoefficients();
    }
}

void RoomEngine::setRoomAmount(float amount01) {
    currentRoomAmount = juce::jlimit(0.0f, 1.0f, amount01);
    wetMixSmoother.setTargetValue(currentRoomAmount);
}

void RoomEngine::setSpeakerField(int fieldIndex) {
    if (currentField != fieldIndex) {
        currentField = fieldIndex;
        updateFdnCoefficients();
    }
}

void RoomEngine::updateFdnCoefficients() {
    const float rt60 = getEffectiveRt60(currentProfile, currentField);

    for (int i = 0; i < NumDelays; ++i) {
        const float delaySec = static_cast<float>(delayLengthsInSamples[static_cast<size_t>(i)]) / static_cast<float>(currentSampleRate);
        // Feedback gain for a -60 dB decay over RT60: 10^(-3 * delay / RT60)
        feedbackGains[static_cast<size_t>(i)] = std::pow(10.0f, -3.0f * delaySec / rt60);
    }
}

void RoomEngine::process(const float* leftIn, const float* rightIn,
                         float* leftOut, float* rightOut, int numSamples) {
    if (numSamples <= 0) return;

    const auto params = getRoomParameters(currentProfile);
    const float erGain = std::pow(10.0f, params.earlyReflGainDb / 20.0f);
    const float dampFactor = std::exp(-2.0f * juce::MathConstants<float>::pi * (params.dampingCutoffHz / static_cast<float>(currentSampleRate)));

    // Early reflection tap offsets from the room dimensions, clamped to the ring
    auto tapSamples = [&](float meters) {
        const int taps = static_cast<int>((meters / SpeakerGeometry::SpeedOfSound) * static_cast<float>(currentSampleRate));
        return juce::jlimit(0, MaxEarlySamples - 1, taps);
    };
    const int er1 = tapSamples(params.roomWidthMeters * 0.45f);
    const int er2 = tapSamples(params.roomLengthMeters * 0.60f);
    const int er3 = tapSamples(params.roomHeightMeters * 0.85f);
    constexpr int erMask = MaxEarlySamples - 1;

    std::array<float, NumDelays> delayOutputs{};

    for (int n = 0; n < numSamples; ++n) {
        const float inL = leftIn[n];
        const float inR = rightIn[n];
        const float wetScale = wetMixSmoother.getNextValue();

        // 1. Early reflections: write, then read three taps
        erBufferLeft[static_cast<size_t>(erWritePos)] = inL;
        erBufferRight[static_cast<size_t>(erWritePos)] = inR;

        const int r1 = (erWritePos - er1) & erMask;
        const int r2 = (erWritePos - er2) & erMask;
        const int r3 = (erWritePos - er3) & erMask;

        const float erL = erGain * (0.6f * erBufferLeft[static_cast<size_t>(r1)] + 0.35f * erBufferRight[static_cast<size_t>(r2)] - 0.25f * erBufferLeft[static_cast<size_t>(r3)]);
        const float erR = erGain * (0.6f * erBufferRight[static_cast<size_t>(r1)] + 0.35f * erBufferLeft[static_cast<size_t>(r2)] - 0.25f * erBufferRight[static_cast<size_t>(r3)]);

        erWritePos = (erWritePos + 1) & erMask;

        // 2. Read the FDN lines
        float sum = 0.0f;
        for (size_t i = 0; i < NumDelays; ++i) {
            const int readPos = (writePointers[i] - delayLengthsInSamples[i]) & delayMasks[i];
            delayOutputs[i] = delayBuffers[i][static_cast<size_t>(readPos)];
            sum += delayOutputs[i];
        }

        // 3. Householder mixing (A = I - 2/N * 11^T), input injection, damping, feedback write
        const float householderShared = sum * (2.0f / static_cast<float>(NumDelays));
        const float fdnInputL = inL * 0.5f + erL * 0.2f;
        const float fdnInputR = inR * 0.5f + erR * 0.2f;

        for (size_t i = 0; i < NumDelays; ++i) {
            const float reflected = delayOutputs[i] - householderShared;
            onePoleLowpassState[i] = reflected * (1.0f - dampFactor) + onePoleLowpassState[i] * dampFactor;
            const float feedbackSig = onePoleLowpassState[i] * feedbackGains[i];

            const float inputInjection = (i % 2 == 0) ? fdnInputL : fdnInputR;
            delayBuffers[i][static_cast<size_t>(writePointers[i])] = inputInjection + feedbackSig;
            writePointers[i] = (writePointers[i] + 1) & delayMasks[i];
        }

        // 4. Output taps
        const float lateL = (delayOutputs[0] + delayOutputs[2] + delayOutputs[4] - delayOutputs[6]) * 0.35f;
        const float lateR = (delayOutputs[1] + delayOutputs[3] + delayOutputs[5] - delayOutputs[7]) * 0.35f;

        // Dry signal plus the wet early + late diffuse reverberation
        leftOut[n] = inL + (erL + lateL) * wetScale;
        rightOut[n] = inR + (erR + lateR) * wetScale;
    }
}

} // namespace fieldroom
