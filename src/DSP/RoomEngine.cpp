#include "RoomEngine.h"

namespace fieldroom {

RoomAcousticParameters RoomEngine::getRoomParameters(int profileIndex) noexcept {
    switch (profileIndex) {
        case 0: // Neutral Reference
            return {"Neutral Reference", 0.15f, -18.0f, 9000.0f, 4.5f, 5.5f, 2.8f, 16.0f};
        case 1: // Small Studio
            return {"Small Studio", 0.28f, -12.0f, 6500.0f, 4.2f, 5.0f, 2.6f, 10.0f};
        case 2: // Professional Control Room
            return {"Professional Control Room", 0.38f, -9.0f, 5200.0f, 6.2f, 7.8f, 3.2f, 7.0f};
        case 3: // Large Control Room
            return {"Large Control Room", 0.58f, -6.5f, 4400.0f, 8.5f, 11.0f, 4.0f, 3.5f};
        case 4: // Domestic Listening Room
            return {"Domestic Listening Room", 0.48f, -8.0f, 3800.0f, 4.8f, 6.2f, 2.5f, 5.0f};
        default:
            return {"Professional Control Room", 0.38f, -9.0f, 5200.0f, 6.2f, 7.8f, 3.2f, 7.0f};
    }
}

RoomEngine::RoomEngine() {
    wetMixSmoother.setCurrentAndTargetValue(0.5f);
}

void RoomEngine::prepare(double sampleRate, int maxBlockSize) {
    currentSampleRate = sampleRate;
    wetMixSmoother.reset(sampleRate, 0.05); // 50ms ramp

    for (int i = 0; i < NumDelays; ++i) {
        int len = static_cast<int>(DelayTimesMs[i] * 0.001f * static_cast<float>(sampleRate));
        delayLengthsInSamples[i] = std::max(64, len);
        delayBuffers[i].assign(delayLengthsInSamples[i] + maxBlockSize + 64, 0.0f);
        writePointers[i] = 0;
        onePoleLowpassState[i] = 0.0f;
    }

    // Allocate 100ms early reflection buffer
    int erSize = static_cast<int>(sampleRate * 0.10f) + maxBlockSize + 64;
    erBufferLeft.assign(erSize, 0.0f);
    erBufferRight.assign(erSize, 0.0f);
    erWritePos = 0;

    updateFdnCoefficients();
}

void RoomEngine::reset() {
    for (int i = 0; i < NumDelays; ++i) {
        std::fill(delayBuffers[i].begin(), delayBuffers[i].end(), 0.0f);
        writePointers[i] = 0;
        onePoleLowpassState[i] = 0.0f;
    }
    std::fill(erBufferLeft.begin(), erBufferLeft.end(), 0.0f);
    std::fill(erBufferRight.begin(), erBufferRight.end(), 0.0f);
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
    auto params = getRoomParameters(currentProfile);
    float rt60 = params.rt60Seconds;

    // Field-dependent scale
    if (currentField == 0) rt60 *= 0.75f; // Nearfield shorter perceived tail
    else if (currentField == 2) rt60 *= 1.25f; // Farfield energizes room more

    for (int i = 0; i < NumDelays; ++i) {
        float delaySec = static_cast<float>(delayLengthsInSamples[i]) / static_cast<float>(currentSampleRate);
        // Formula: gain = 10 ^ (-3 * delay / RT60)
        feedbackGains[i] = std::pow(10.0f, -3.0f * delaySec / rt60);
    }
}

void RoomEngine::process(const float* leftIn, const float* rightIn,
                        float* leftOut, float* rightOut, int numSamples) {
    if (numSamples <= 0) return;

    auto params = getRoomParameters(currentProfile);
    float erGain = std::pow(10.0f, params.earlyReflGainDb / 20.0f);
    float dampFactor = std::exp(-2.0f * juce::MathConstants<float>::pi * (params.dampingCutoffHz / static_cast<float>(currentSampleRate)));

    int erBufLen = static_cast<int>(erBufferLeft.size());

    // Compute early reflection delay offsets based on room dimensions
    int er1 = static_cast<int>((params.roomWidthMeters * 0.45f / SpeakerGeometry::SpeedOfSound) * currentSampleRate);
    int er2 = static_cast<int>((params.roomLengthMeters * 0.60f / SpeakerGeometry::SpeedOfSound) * currentSampleRate);
    int er3 = static_cast<int>((params.roomHeightMeters * 0.85f / SpeakerGeometry::SpeedOfSound) * currentSampleRate);

    std::array<float, NumDelays> delayOutputs;

    for (int n = 0; n < numSamples; ++n) {
        float inL = leftIn[n];
        float inR = rightIn[n];
        float wetScale = wetMixSmoother.getNextValue();

        // 1. Write to Early Reflection Buffer
        erBufferLeft[erWritePos] = inL;
        erBufferRight[erWritePos] = inR;

        // Read early taps
        int r1 = (erWritePos - er1 + erBufLen) % erBufLen;
        int r2 = (erWritePos - er2 + erBufLen) % erBufLen;
        int r3 = (erWritePos - er3 + erBufLen) % erBufLen;

        float erL = erGain * (0.6f * erBufferLeft[r1] + 0.35f * erBufferRight[r2] - 0.25f * erBufferLeft[r3]);
        float erR = erGain * (0.6f * erBufferRight[r1] + 0.35f * erBufferLeft[r2] - 0.25f * erBufferRight[r3]);

        erWritePos = (erWritePos + 1) % erBufLen;

        // 2. Read from FDN delay lines
        for (int i = 0; i < NumDelays; ++i) {
            int readPos = (writePointers[i] - delayLengthsInSamples[i] + static_cast<int>(delayBuffers[i].size())) % delayBuffers[i].size();
            delayOutputs[i] = delayBuffers[i][readPos];
        }

        // 3. Householder mixing matrix: A = I - 2/N * 1*1^T
        float sum = 0.0f;
        for (int i = 0; i < NumDelays; ++i) {
            sum += delayOutputs[i];
        }
        float householderShared = sum * (2.0f / static_cast<float>(NumDelays));

        // Inject input + early reflections into FDN
        float fdnInputL = inL * 0.5f + erL * 0.2f;
        float fdnInputR = inR * 0.5f + erR * 0.2f;

        for (int i = 0; i < NumDelays; ++i) {
            float reflected = delayOutputs[i] - householderShared;
            // One-pole damping filter
            onePoleLowpassState[i] = reflected * (1.0f - dampFactor) + onePoleLowpassState[i] * dampFactor;
            float feedbackSig = onePoleLowpassState[i] * feedbackGains[i];

            float inputInjection = (i % 2 == 0) ? fdnInputL : fdnInputR;
            delayBuffers[i][writePointers[i]] = inputInjection + feedbackSig;
            writePointers[i] = (writePointers[i] + 1) % delayBuffers[i].size();
        }

        // 4. Output Tap Summation
        float lateL = (delayOutputs[0] + delayOutputs[2] + delayOutputs[4] - delayOutputs[6]) * 0.35f;
        float lateR = (delayOutputs[1] + delayOutputs[3] + delayOutputs[5] - delayOutputs[7]) * 0.35f;

        // Blend early + late diffuse reverberation
        leftOut[n] = inL + (erL + lateL) * wetScale;
        rightOut[n] = inR + (erR + lateR) * wetScale;
    }
}

} // namespace fieldroom
