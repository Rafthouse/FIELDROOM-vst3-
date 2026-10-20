#include "BinauralRenderer.h"

namespace fieldroom {

BinauralRenderer::BinauralRenderer() {
    loudnessGainSmoother.setCurrentAndTargetValue(1.0f);
}

void BinauralRenderer::prepare(double sampleRate, int maxBlockSize) {
    currentSampleRate = sampleRate;
    delayBufferL.assign(MaxDelaySamples + maxBlockSize + 64, 0.0f);
    delayBufferR.assign(MaxDelaySamples + maxBlockSize + 64, 0.0f);
    writeIndex = 0;

    // Reset smoothers
    double rampSec = 0.02; // 20ms click-free crossfade
    delayLeftIpsi.reset(sampleRate, rampSec);
    delayLeftContra.reset(sampleRate, rampSec);
    delayRightIpsi.reset(sampleRate, rampSec);
    delayRightContra.reset(sampleRate, rampSec);

    gainLeftIpsi.reset(sampleRate, rampSec);
    gainLeftContra.reset(sampleRate, rampSec);
    gainRightIpsi.reset(sampleRate, rampSec);
    gainRightContra.reset(sampleRate, rampSec);

    loudnessGainSmoother.reset(sampleRate, rampSec);

    // Initial shadow filter coefficients: Butterworth lowpass around 4.5 kHz for contralateral shadowing
    auto shadowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 4200.0f, 0.65f);
    shadowFilterLR.coefficients = shadowCoeffs;
    shadowFilterRL.coefficients = shadowCoeffs;
    shadowFilterLL.reset();
    shadowFilterLR.reset();
    shadowFilterRL.reset();
    shadowFilterRR.reset();

    // Air absorption filter
    auto airCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 8000.0f, 0.707f, 0.95f);
    airAbsorptionL.coefficients = airCoeffs;
    airAbsorptionR.coefficients = airCoeffs;
    airAbsorptionL.reset();
    airAbsorptionR.reset();

    updateGeometry();
}

void BinauralRenderer::reset() {
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writeIndex = 0;
    shadowFilterLL.reset();
    shadowFilterLR.reset();
    shadowFilterRL.reset();
    shadowFilterRR.reset();
    airAbsorptionL.reset();
    airAbsorptionR.reset();
}

void BinauralRenderer::setSpeakerField(int fieldIndex) {
    if (currentField != fieldIndex) {
        currentField = fieldIndex;
        updateGeometry();
    }
}

void BinauralRenderer::setSpeakerSpread(float spreadDegrees) {
    float clamped = juce::jlimit(30.0f, 110.0f, spreadDegrees);
    if (std::abs(currentSpreadDeg - clamped) > 0.01f) {
        currentSpreadDeg = clamped;
        updateGeometry();
    }
}

void BinauralRenderer::setHeadRotation(float rotationDegrees) {
    if (std::abs(currentRotationDeg - rotationDegrees) > 0.01f) {
        currentRotationDeg = juce::jlimit(-90.0f, 90.0f, rotationDegrees);
        updateGeometry();
    }
}

void BinauralRenderer::setLoudnessMatch(bool enable) {
    if (loudnessMatchEnabled != enable) {
        loudnessMatchEnabled = enable;
        updateGeometry();
    }
}

void BinauralRenderer::updateGeometry() {
    auto config = SpeakerGeometry::getFieldConfig(currentField);

    // Symmetrical speaker azimuth based on Speaker Spread angle (30° to 110°, default 60°)
    float halfSpreadRad = (currentSpreadDeg * 0.5f) * (juce::MathConstants<float>::pi / 180.0f);
    float rotRad = currentRotationDeg * (juce::MathConstants<float>::pi / 180.0f);
    float leftSpkAngleRad = -halfSpreadRad - rotRad;
    float rightSpkAngleRad = halfSpreadRad - rotRad;

    // Direct distance propagation delay (baseline distance to center of head)
    float baseDelaySec = config.distanceMeters / SpeakerGeometry::SpeedOfSound;

    // Woodworth ITD offsets
    float itdLeftSpk = HRTFManager::calculateWoodworthITD(leftSpkAngleRad);
    float itdRightSpk = HRTFManager::calculateWoodworthITD(rightSpkAngleRad);

    // Delays in samples:
    // Left speaker to left ear (ipsilateral if angle < 0)
    float dLeftIpsi = std::max(0.0f, (baseDelaySec - (itdLeftSpk * 0.5f)) * static_cast<float>(currentSampleRate));
    float dLeftContra = std::max(0.0f, (baseDelaySec + (itdLeftSpk * 0.5f)) * static_cast<float>(currentSampleRate));

    // Right speaker to right ear & left ear
    float dRightIpsi = std::max(0.0f, (baseDelaySec + (itdRightSpk * 0.5f)) * static_cast<float>(currentSampleRate));
    float dRightContra = std::max(0.0f, (baseDelaySec - (itdRightSpk * 0.5f)) * static_cast<float>(currentSampleRate));

    delayLeftIpsi.setTargetValue(dLeftIpsi);
    delayLeftContra.setTargetValue(dLeftContra);
    delayRightIpsi.setTargetValue(dRightIpsi);
    delayRightContra.setTargetValue(dRightContra);

    // Gains & Head Shadow Level Difference (ILD)
    float gLIpsi = 1.0f;
    float gLContra = std::pow(10.0f, HRTFManager::calculateHeadShadowLoss(leftSpkAngleRad, 2500.0f) / 20.0f) * 0.88f;

    float gRIpsi = 1.0f;
    float gRContra = std::pow(10.0f, HRTFManager::calculateHeadShadowLoss(rightSpkAngleRad, 2500.0f) / 20.0f) * 0.88f;

    gainLeftIpsi.setTargetValue(gLIpsi);
    gainLeftContra.setTargetValue(gLContra);
    gainRightIpsi.setTargetValue(gRIpsi);
    gainRightContra.setTargetValue(gRContra);

    // Loudness match offset to balance Near/Mid/Far without perceived volume drop
    float targetLoudnessGain = 1.0f;
    if (loudnessMatchEnabled) {
        targetLoudnessGain = std::pow(10.0f, config.loudnessCompensationGainDb / 20.0f);
    }
    loudnessGainSmoother.setTargetValue(targetLoudnessGain);
}

float BinauralRenderer::readInterpolatedDelay(const std::vector<float>& buf, float delayInSamples) const noexcept {
    int bufSize = static_cast<int>(buf.size());
    float readIdx = static_cast<float>(writeIndex) - delayInSamples;
    while (readIdx < 0.0f) readIdx += static_cast<float>(bufSize);

    int i0 = static_cast<int>(readIdx) % bufSize;
    int i1 = (i0 + 1) % bufSize;
    float frac = readIdx - std::floor(readIdx);

    return buf[i0] + frac * (buf[i1] - buf[i0]); // Linear interpolation
}

void BinauralRenderer::process(const float* inL, const float* inR,
                              float* outL, float* outR, int numSamples) {
    if (numSamples <= 0) return;

    int bufSize = static_cast<int>(delayBufferL.size());

    for (int n = 0; n < numSamples; ++n) {
        float sampleL = inL[n];
        float sampleR = inR[n];

        // Air absorption filtering
        sampleL = airAbsorptionL.processSample(sampleL);
        sampleR = airAbsorptionR.processSample(sampleR);

        // Store into delay lines
        delayBufferL[writeIndex] = sampleL;
        delayBufferR[writeIndex] = sampleR;

        // Fetch current smooth parameters
        float dLI = delayLeftIpsi.getNextValue();
        float dLC = delayLeftContra.getNextValue();
        float dRI = delayRightIpsi.getNextValue();
        float dRC = delayRightContra.getNextValue();

        float gLI = gainLeftIpsi.getNextValue();
        float gLC = gainLeftContra.getNextValue();
        float gRI = gainRightIpsi.getNextValue();
        float gRC = gainRightContra.getNextValue();
        float lGain = loudnessGainSmoother.getNextValue();

        // 2x2 loudspeaker binaural matrix
        // Left speaker contributions:
        float spkLeftToEarL = readInterpolatedDelay(delayBufferL, dLI) * gLI;
        float rawSpkLeftToEarR = readInterpolatedDelay(delayBufferL, dLC) * gLC;
        float spkLeftToEarR = shadowFilterLR.processSample(rawSpkLeftToEarR);

        // Right speaker contributions:
        float spkRightToEarR = readInterpolatedDelay(delayBufferR, dRI) * gRI;
        float rawSpkRightToEarL = readInterpolatedDelay(delayBufferR, dRC) * gRC;
        float spkRightToEarL = shadowFilterRL.processSample(rawSpkRightToEarL);

        // Ear summation:
        // Left Ear = Left Spk -> Left Ear + Right Spk -> Left Ear
        // Right Ear = Right Spk -> Right Ear + Left Spk -> Right Ear
        outL[n] = (spkLeftToEarL + spkRightToEarL) * lGain;
        outR[n] = (spkRightToEarR + spkLeftToEarR) * lGain;

        writeIndex = (writeIndex + 1) % bufSize;
    }
}

} // namespace fieldroom
