#include "MonitoringUtilities.h"
#include <algorithm>

namespace fieldroom {

MonitoringUtilities::MonitoringUtilities() {
    trimSmoother.setCurrentAndTargetValue(1.0f);
    dimSmoother.setCurrentAndTargetValue(1.0f);
}

void MonitoringUtilities::prepare(double sampleRate, int maxBlockSize) {
    juce::ignoreUnused(maxBlockSize);
    currentSampleRate = sampleRate;

    trimSmoother.reset(sampleRate, 0.02); // 20ms ramp
    dimSmoother.reset(sampleRate, 0.03);  // 30ms click-free DIM transition

    reset();
}

void MonitoringUtilities::reset() {
    sumSquaresL = 0.0f;
    sumSquaresR = 0.0f;
    sumDotProduct = 0.0f;
    meterSampleCount = 0;
    currentLevels = MeterLevels{};
}

void MonitoringUtilities::setMode(MonitoringMode mode) {
    currentMode = mode;
}

void MonitoringUtilities::setSwap(bool enabled) {
    swapActive = enabled;
}

void MonitoringUtilities::setDim(bool enabled) {
    dimActive = enabled;
    // -18 dB dim factor = 0.12589
    float target = dimActive ? std::pow(10.0f, -18.0f / 20.0f) : 1.0f;
    dimSmoother.setTargetValue(target);
}

void MonitoringUtilities::setOutputTrim(float trimDb) {
    currentTrimDb = trimDb;
    float target = std::pow(10.0f, currentTrimDb / 20.0f);
    trimSmoother.setTargetValue(target);
}

void MonitoringUtilities::process(float* left, float* right, int numSamples) {
    if (numSamples <= 0) return;

    float peakL = 0.0f;
    float peakR = 0.0f;

    for (int n = 0; n < numSamples; ++n) {
        float l = left[n];
        float r = right[n];

        // 1. Channel Audition Routing Modes (Mutually exclusive)
        switch (currentMode) {
            case MonitoringMode::Mono: {
                // True Mono with -3dB pan-law compensation (gain 1/sqrt(2) = ~0.7071)
                float mono = (l + r) * 0.70710678f;
                l = mono;
                r = mono;
                break;
            }
            case MonitoringMode::Side: {
                // Side / Difference audition: (L - R) / 2
                float side = (l - r) * 0.5f;
                l = side;
                r = -side; // Preserves out-of-phase acoustic externalization
                break;
            }
            case MonitoringMode::LeftSolo: {
                // Left channel to both ears for phantom center inspection
                r = l;
                break;
            }
            case MonitoringMode::RightSolo: {
                // Right channel to both ears
                l = r;
                break;
            }
            case MonitoringMode::Normal:
            default:
                break;
        }

        // 2. Independent L/R Channel Swap
        if (swapActive) {
            std::swap(l, r);
        }

        // 3. Apply Output Trim and DIM
        float trim = trimSmoother.getNextValue();
        float dim = dimSmoother.getNextValue();
        l *= trim * dim;
        r *= trim * dim;

        left[n] = l;
        right[n] = r;

        // 4. Metering Accumulation
        float absL = std::abs(l);
        float absR = std::abs(r);
        if (absL > peakL) peakL = absL;
        if (absR > peakR) peakR = absR;

        sumSquaresL += l * l;
        sumSquaresR += r * r;
        sumDotProduct += l * r;
        meterSampleCount++;
    }

    // Update Meter Levels roughly every block or ~512 samples
    if (meterSampleCount >= 512) {
        float rmsL = std::sqrt(sumSquaresL / static_cast<float>(meterSampleCount));
        float rmsR = std::sqrt(sumSquaresR / static_cast<float>(meterSampleCount));

        currentLevels.peakLeftDb = (peakL > 1e-5f) ? (20.0f * std::log10(peakL)) : -100.0f;
        currentLevels.peakRightDb = (peakR > 1e-5f) ? (20.0f * std::log10(peakR)) : -100.0f;
        currentLevels.rmsLeftDb = (rmsL > 1e-5f) ? (20.0f * std::log10(rmsL)) : -100.0f;
        currentLevels.rmsRightDb = (rmsR > 1e-5f) ? (20.0f * std::log10(rmsR)) : -100.0f;

        float denom = std::sqrt(sumSquaresL * sumSquaresR);
        if (denom > 1e-6f) {
            currentLevels.correlation = juce::jlimit(-1.0f, 1.0f, sumDotProduct / denom);
        } else {
            currentLevels.correlation = 1.0f;
        }
        currentLevels.isClipping = (peakL >= 1.0f || peakR >= 1.0f);

        sumSquaresL = 0.0f;
        sumSquaresR = 0.0f;
        sumDotProduct = 0.0f;
        meterSampleCount = 0;
    }
}

} // namespace fieldroom
