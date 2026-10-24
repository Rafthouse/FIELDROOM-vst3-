#include "MonitoringUtilities.h"
#include <algorithm>

namespace fieldroom {

MonitoringUtilities::MonitoringUtilities() {
    trimSmoother.setCurrentAndTargetValue(1.0f);
    dimSmoother.setCurrentAndTargetValue(1.0f);
}

void MonitoringUtilities::prepare(double sampleRate, int maxBlockSize) {
    juce::ignoreUnused(maxBlockSize);
    trimSmoother.reset(sampleRate, 0.02); // 20 ms ramp
    dimSmoother.reset(sampleRate, 0.03);  // 30 ms click-free DIM transition

    reset();
}

void MonitoringUtilities::reset() {
    sumSquaresL = 0.0f;
    sumSquaresR = 0.0f;
    sumDotProduct = 0.0f;
    windowPeakL = 0.0f;
    windowPeakR = 0.0f;
    meterSampleCount = 0;

    publishedPeakLDb.store(-100.0f, std::memory_order_relaxed);
    publishedPeakRDb.store(-100.0f, std::memory_order_relaxed);
    publishedRmsLDb.store(-100.0f, std::memory_order_relaxed);
    publishedRmsRDb.store(-100.0f, std::memory_order_relaxed);
    publishedCorrelation.store(1.0f, std::memory_order_relaxed);
    publishedClipping.store(false, std::memory_order_relaxed);
}

void MonitoringUtilities::setMode(MonitoringMode mode) {
    currentMode = mode;
}

void MonitoringUtilities::setSwap(bool enabled) {
    swapActive = enabled;
}

void MonitoringUtilities::setDim(bool enabled) {
    if (dimActive == enabled) return;
    dimActive = enabled;
    // -18 dB DIM factor
    const float target = dimActive ? std::pow(10.0f, -18.0f / 20.0f) : 1.0f;
    dimSmoother.setTargetValue(target);
}

void MonitoringUtilities::setOutputTrim(float trimDb) {
    trimSmoother.setTargetValue(std::pow(10.0f, trimDb / 20.0f));
}

void MonitoringUtilities::process(float* left, float* right, int numSamples) {
    if (numSamples <= 0) return;

    for (int n = 0; n < numSamples; ++n) {
        float l = left[n];
        float r = right[n];

        // 1. Audition routing (mutually exclusive)
        switch (currentMode) {
            case MonitoringMode::Mono: {
                // Mono fold-down as (L + R) / 2: a centred source keeps its level, so the check is level-neutral
                const float mono = (l + r) * 0.5f;
                l = mono;
                r = mono;
                break;
            }
            case MonitoringMode::Side: {
                // Side / difference audition: (L - R) / 2, kept out of phase for externalisation
                const float side = (l - r) * 0.5f;
                l = side;
                r = -side;
                break;
            }
            case MonitoringMode::LeftSolo:
                r = l;
                break;
            case MonitoringMode::RightSolo:
                l = r;
                break;
            case MonitoringMode::Normal:
            default:
                break;
        }

        // 2. Independent L/R swap
        if (swapActive) {
            std::swap(l, r);
        }

        // 3. Output trim and DIM
        const float gain = trimSmoother.getNextValue() * dimSmoother.getNextValue();
        l *= gain;
        r *= gain;

        left[n] = l;
        right[n] = r;

        accumulateMeters(l, r);
    }

    publishMetersIfWindowFull();
}

void MonitoringUtilities::measure(const float* left, const float* right, int numSamples) {
    if (numSamples <= 0) return;

    for (int n = 0; n < numSamples; ++n) {
        accumulateMeters(left[n], right[n]);
    }

    publishMetersIfWindowFull();
}

void MonitoringUtilities::accumulateMeters(float l, float r) noexcept {
    // Peaks are held across every block of the window, not only the block that closes it
    windowPeakL = std::max(windowPeakL, std::abs(l));
    windowPeakR = std::max(windowPeakR, std::abs(r));

    sumSquaresL += l * l;
    sumSquaresR += r * r;
    sumDotProduct += l * r;
    meterSampleCount++;
}

void MonitoringUtilities::publishMetersIfWindowFull() noexcept {
    if (meterSampleCount < 512) return;

    const float count = static_cast<float>(meterSampleCount);
    const float rmsL = std::sqrt(sumSquaresL / count);
    const float rmsR = std::sqrt(sumSquaresR / count);

    auto toDb = [](float linear) { return (linear > 1e-5f) ? 20.0f * std::log10(linear) : -100.0f; };

    publishedPeakLDb.store(toDb(windowPeakL), std::memory_order_relaxed);
    publishedPeakRDb.store(toDb(windowPeakR), std::memory_order_relaxed);
    publishedRmsLDb.store(toDb(rmsL), std::memory_order_relaxed);
    publishedRmsRDb.store(toDb(rmsR), std::memory_order_relaxed);

    const float denom = std::sqrt(sumSquaresL * sumSquaresR);
    const float correlation = (denom > 1e-6f) ? juce::jlimit(-1.0f, 1.0f, sumDotProduct / denom) : 1.0f;
    publishedCorrelation.store(correlation, std::memory_order_relaxed);
    publishedClipping.store(windowPeakL >= 1.0f || windowPeakR >= 1.0f, std::memory_order_relaxed);

    sumSquaresL = 0.0f;
    sumSquaresR = 0.0f;
    sumDotProduct = 0.0f;
    windowPeakL = 0.0f;
    windowPeakR = 0.0f;
    meterSampleCount = 0;
}

MeterLevels MonitoringUtilities::getMeterLevels() const noexcept {
    MeterLevels levels;
    levels.peakLeftDb = publishedPeakLDb.load(std::memory_order_relaxed);
    levels.peakRightDb = publishedPeakRDb.load(std::memory_order_relaxed);
    levels.rmsLeftDb = publishedRmsLDb.load(std::memory_order_relaxed);
    levels.rmsRightDb = publishedRmsRDb.load(std::memory_order_relaxed);
    levels.correlation = publishedCorrelation.load(std::memory_order_relaxed);
    levels.isClipping = publishedClipping.load(std::memory_order_relaxed);
    return levels;
}

} // namespace fieldroom
