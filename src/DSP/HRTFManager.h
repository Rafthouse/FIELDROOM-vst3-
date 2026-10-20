#pragma once

#include <vector>
#include <cmath>
#include <juce_core/juce_core.h>
#include "SpeakerGeometry.h"

namespace fieldroom {

struct HRIRPair {
    std::vector<float> left;
    std::vector<float> right;
    float itdSeconds{0.0f};
    float ildDb{0.0f};
};

class HRTFManager {
public:
    HRTFManager() = default;

    void prepare(double sampleRate) {
        currentSampleRate = sampleRate;
    }

    // Calculates analytical Woodworth-Schlosser ITD and spherical head diffraction
    // Azimuth theta: -pi/2 (left) to +pi/2 (right). Elevation phi: radians
    static float calculateWoodworthITD(float azimuthRad) noexcept {
        float absTheta = std::abs(azimuthRad);
        float itd = (SpeakerGeometry::HeadRadius / SpeakerGeometry::SpeedOfSound) * (std::sin(absTheta) + absTheta);
        return (azimuthRad >= 0.0f) ? itd : -itd; // positive means right ear arrives later if source is on left
    }

    // Calculate frequency-dependent contralateral head-shadow attenuation
    static float calculateHeadShadowLoss(float azimuthRad, float freqHz) noexcept {
        float absTheta = std::abs(azimuthRad);
        if (freqHz < 400.0f) return 0.0f; // low frequencies diffract around head without attenuation
        float maxLossDb = -7.5f * (std::sin(absTheta));
        float freqWeight = std::min(1.0f, (freqHz - 400.0f) / 3600.0f);
        return maxLossDb * freqWeight;
    }

    double getSampleRate() const noexcept { return currentSampleRate; }

private:
    double currentSampleRate{48000.0};
};

} // namespace fieldroom
