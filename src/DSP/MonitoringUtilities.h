#pragma once

#include <atomic>
#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "MonitoringTypes.h"

namespace fieldroom {

// Snapshot of the metering values, safe to copy on any thread
struct MeterLevels {
    float peakLeftDb{-100.0f};
    float peakRightDb{-100.0f};
    float rmsLeftDb{-100.0f};
    float rmsRightDb{-100.0f};
    float correlation{1.0f}; // -1.0 (anti-phase) to +1.0 (mono)
    bool isClipping{false};
};

/**
 * Audition routing (mode, swap), DIM, output trim and metering.
 *
 * The audio thread publishes meter values through atomics, so the editor can read a snapshot at any
 * time without locks and without a data race.
 */
class MonitoringUtilities {
public:
    MonitoringUtilities();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setMode(MonitoringMode mode);
    void setSwap(bool enabled);
    void setDim(bool enabled);
    void setOutputTrim(float trimDb);

    // Routing, swap, trim and DIM are applied to the buffer, then the meters are updated
    void process(float* left, float* right, int numSamples);

    // Meters only, leaves the buffer untouched (used for master bypass)
    void measure(const float* left, const float* right, int numSamples);

    MeterLevels getMeterLevels() const noexcept;

private:
    void accumulateMeters(float l, float r) noexcept;
    void publishMetersIfWindowFull() noexcept;

    MonitoringMode currentMode{MonitoringMode::Normal};
    bool swapActive{false};
    bool dimActive{false};

    juce::SmoothedValue<float> trimSmoother;
    juce::SmoothedValue<float> dimSmoother;

    // Running sums over one meter window (~512 samples), audio thread only
    float sumSquaresL{0.0f};
    float sumSquaresR{0.0f};
    float sumDotProduct{0.0f};
    float windowPeakL{0.0f};
    float windowPeakR{0.0f};
    int meterSampleCount{0};

    // Published results, read by the UI thread
    std::atomic<float> publishedPeakLDb{-100.0f};
    std::atomic<float> publishedPeakRDb{-100.0f};
    std::atomic<float> publishedRmsLDb{-100.0f};
    std::atomic<float> publishedRmsRDb{-100.0f};
    std::atomic<float> publishedCorrelation{1.0f};
    std::atomic<bool> publishedClipping{false};
};

} // namespace fieldroom
