#pragma once

#include <cmath>
#include <juce_dsp/juce_dsp.h>
#include "MonitoringTypes.h"

namespace fieldroom {

struct MeterLevels {
    float peakLeftDb{-100.0f};
    float peakRightDb{-100.0f};
    float rmsLeftDb{-100.0f};
    float rmsRightDb{-100.0f};
    float correlation{1.0f}; // -1.0 (anti-phase) to +1.0 (mono)
    bool isClipping{false};
};

class MonitoringUtilities {
public:
    MonitoringUtilities();

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setMode(MonitoringMode mode);
    void setSwap(bool enabled);
    void setDim(bool enabled);
    void setOutputTrim(float trimDb);

    void process(float* left, float* right, int numSamples);

    MeterLevels getMeterLevels() const noexcept { return currentLevels; }

private:
    double currentSampleRate{48000.0};
    MonitoringMode currentMode{MonitoringMode::Normal};
    bool swapActive{false};
    bool dimActive{false};
    float currentTrimDb{0.0f};

    juce::SmoothedValue<float> trimSmoother;
    juce::SmoothedValue<float> dimSmoother;

    MeterLevels currentLevels;

    // Running RMS & peak tracking
    float sumSquaresL{0.0f};
    float sumSquaresR{0.0f};
    float sumDotProduct{0.0f};
    int meterSampleCount{0};
};

} // namespace fieldroom
