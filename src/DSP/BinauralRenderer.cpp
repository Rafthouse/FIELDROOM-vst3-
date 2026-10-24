#include "BinauralRenderer.h"
#include <algorithm>

namespace fieldroom {

BinauralRenderer::BinauralRenderer() {
    loudnessGainSmoother.setCurrentAndTargetValue(1.0f);
}

void BinauralRenderer::prepare(double sampleRate, int maxBlockSize) {
    juce::ignoreUnused(maxBlockSize); // ring buffers are fixed-size members, no per-block sizing needed
    currentSampleRate = sampleRate;

    // Reset smoothers
    const double rampSec = 0.02; // 20 ms click-free crossfade
    delayLeftIpsi.reset(sampleRate, rampSec);
    delayLeftContra.reset(sampleRate, rampSec);
    delayRightIpsi.reset(sampleRate, rampSec);
    delayRightContra.reset(sampleRate, rampSec);

    gainLeftIpsi.reset(sampleRate, rampSec);
    gainLeftContra.reset(sampleRate, rampSec);
    gainRightIpsi.reset(sampleRate, rampSec);
    gainRightContra.reset(sampleRate, rampSec);

    loudnessGainSmoother.reset(sampleRate, rampSec);

    // Contralateral head shadow: lowpass around 4.2 kHz
    auto shadowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 4200.0f, 0.65f);
    shadowFilterLR.coefficients = shadowCoeffs;
    shadowFilterRL.coefficients = shadowCoeffs;

    // Air absorption: high shelf at 8 kHz
    auto airCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 8000.0f, 0.707f, 0.95f);
    airAbsorptionL.coefficients = airCoeffs;
    airAbsorptionR.coefficients = airCoeffs;

    reset();
    updateGeometry();
}

void BinauralRenderer::reset() {
    delayBufferL.fill(0.0f);
    delayBufferR.fill(0.0f);
    writeIndex = 0;
    shadowFilterLR.reset();
    shadowFilterRL.reset();
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
    const float clamped = juce::jlimit(30.0f, 110.0f, spreadDegrees);
    if (std::abs(currentSpreadDeg - clamped) > 0.01f) {
        currentSpreadDeg = clamped;
        updateGeometry();
    }
}

void BinauralRenderer::setHeadRotation(float rotationDegrees) {
    const float clamped = juce::jlimit(-90.0f, 90.0f, rotationDegrees);
    if (std::abs(currentRotationDeg - clamped) > 0.01f) {
        currentRotationDeg = clamped;
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
    const auto config = SpeakerGeometry::getFieldConfig(currentField);

    // Symmetrical speaker azimuth based on Speaker Spread angle (30° to 110°, default 60°)
    const float halfSpreadRad = (currentSpreadDeg * 0.5f) * (juce::MathConstants<float>::pi / 180.0f);
    const float rotRad = currentRotationDeg * (juce::MathConstants<float>::pi / 180.0f);
    const float leftSpkAngleRad = -halfSpreadRad - rotRad;
    const float rightSpkAngleRad = halfSpreadRad - rotRad;

    // Direct distance propagation delay (baseline distance to center of head)
    const float baseDelaySec = config.distanceMeters / SpeakerGeometry::SpeedOfSound;
    const float sr = static_cast<float>(currentSampleRate);
    const float maxDelay = static_cast<float>(MaxDelaySamples - 2);

    // Woodworth ITD offsets
    const float itdLeftSpk = HRTFManager::calculateWoodworthITD(leftSpkAngleRad);
    const float itdRightSpk = HRTFManager::calculateWoodworthITD(rightSpkAngleRad);

    // Delays in samples: ipsilateral is the ear on the same side as the speaker
    auto toSamples = [&](float sec) { return juce::jlimit(0.0f, maxDelay, sec * sr); };
    delayLeftIpsi.setTargetValue(toSamples(baseDelaySec - itdLeftSpk * 0.5f));
    delayLeftContra.setTargetValue(toSamples(baseDelaySec + itdLeftSpk * 0.5f));
    delayRightIpsi.setTargetValue(toSamples(baseDelaySec + itdRightSpk * 0.5f));
    delayRightContra.setTargetValue(toSamples(baseDelaySec - itdRightSpk * 0.5f));

    // Gains & head-shadow level difference (ILD) for the contralateral path
    const float gLContra = std::pow(10.0f, HRTFManager::calculateHeadShadowLoss(leftSpkAngleRad, 2500.0f) / 20.0f) * 0.88f;
    const float gRContra = std::pow(10.0f, HRTFManager::calculateHeadShadowLoss(rightSpkAngleRad, 2500.0f) / 20.0f) * 0.88f;

    gainLeftIpsi.setTargetValue(1.0f);
    gainLeftContra.setTargetValue(gLContra);
    gainRightIpsi.setTargetValue(1.0f);
    gainRightContra.setTargetValue(gRContra);

    // Loudness match: per-field gain so Near, Mid and Far sound equally loud
    float targetLoudnessGain = 1.0f;
    if (loudnessMatchEnabled) {
        targetLoudnessGain = std::pow(10.0f, config.loudnessCompensationGainDb / 20.0f);
    }
    loudnessGainSmoother.setTargetValue(targetLoudnessGain);
}

float BinauralRenderer::readInterpolatedDelay(const std::array<float, DelayBufferSize>& buf, float delayInSamples) const noexcept {
    // Linear interpolation; the mask wraps negative indices correctly for a power-of-two ring
    const float readPos = static_cast<float>(writeIndex) - delayInSamples;
    const int base = static_cast<int>(std::floor(readPos));
    const float frac = readPos - static_cast<float>(base);
    const float s0 = buf[static_cast<size_t>(base & DelayMask)];
    const float s1 = buf[static_cast<size_t>((base + 1) & DelayMask)];
    return s0 + frac * (s1 - s0);
}

void BinauralRenderer::process(const float* inL, const float* inR,
                               float* outL, float* outR, int numSamples) {
    if (numSamples <= 0) return;

    for (int n = 0; n < numSamples; ++n) {
        // Air absorption, then store into the delay lines
        const float sampleL = airAbsorptionL.processSample(inL[n]);
        const float sampleR = airAbsorptionR.processSample(inR[n]);

        delayBufferL[static_cast<size_t>(writeIndex)] = sampleL;
        delayBufferR[static_cast<size_t>(writeIndex)] = sampleR;

        // Current smoothed parameters
        const float dLI = delayLeftIpsi.getNextValue();
        const float dLC = delayLeftContra.getNextValue();
        const float dRI = delayRightIpsi.getNextValue();
        const float dRC = delayRightContra.getNextValue();

        const float gLI = gainLeftIpsi.getNextValue();
        const float gLC = gainLeftContra.getNextValue();
        const float gRI = gainRightIpsi.getNextValue();
        const float gRC = gainRightContra.getNextValue();
        const float lGain = loudnessGainSmoother.getNextValue();

        // 2x2 loudspeaker-to-ear matrix
        const float spkLeftToEarL = readInterpolatedDelay(delayBufferL, dLI) * gLI;
        const float spkLeftToEarR = shadowFilterLR.processSample(readInterpolatedDelay(delayBufferL, dLC) * gLC);

        const float spkRightToEarR = readInterpolatedDelay(delayBufferR, dRI) * gRI;
        const float spkRightToEarL = shadowFilterRL.processSample(readInterpolatedDelay(delayBufferR, dRC) * gRC);

        // Ear summation
        outL[n] = (spkLeftToEarL + spkRightToEarL) * lGain;
        outR[n] = (spkRightToEarR + spkLeftToEarR) * lGain;

        writeIndex = (writeIndex + 1) & DelayMask;
    }
}

} // namespace fieldroom
