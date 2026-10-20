#pragma once

#include <cmath>
#include <juce_core/juce_core.h>

namespace fieldroom {

struct Vec3D {
    float x{0.0f}; // Lateral (meters, positive = right)
    float y{0.0f}; // Longitudinal / depth (meters, positive = front)
    float z{0.0f}; // Height (meters, positive = up)

    float distanceTo(const Vec3D& o) const noexcept {
        float dx = x - o.x;
        float dy = y - o.y;
        float dz = z - o.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
};

struct SpeakerLayoutConfig {
    float distanceMeters{2.2f};
    float angleDegrees{60.0f}; // Inter-speaker opening angle (equilateral = 60)
    float heightMeters{1.2f};   // Ear acoustic center height
    float drrDb{7.0f};          // Direct-to-reverberant ratio in reference room
    float airAbsorptionHfDb{-0.5f}; // 16 kHz atmospheric dissipation
    float loudnessCompensationGainDb{4.5f}; // Level match offset
};

class SpeakerGeometry {
public:
    static constexpr float SpeedOfSound = 343.2f; // m/s at 20 deg C
    static constexpr float HeadRadius = 0.0875f;  // Average adult human head (8.75 cm)

    static SpeakerLayoutConfig getFieldConfig(int fieldIndex) noexcept {
        SpeakerLayoutConfig cfg;
        switch (fieldIndex) {
            case 0: // Nearfield
                cfg.distanceMeters = 1.1f;
                cfg.angleDegrees = 60.0f;
                cfg.heightMeters = 1.15f;
                cfg.drrDb = 14.0f;
                cfg.airAbsorptionHfDb = 0.0f;
                cfg.loudnessCompensationGainDb = 0.0f;
                break;
            case 1: // Midfield
                cfg.distanceMeters = 2.2f;
                cfg.angleDegrees = 60.0f;
                cfg.heightMeters = 1.25f;
                cfg.drrDb = 7.0f;
                cfg.airAbsorptionHfDb = -0.5f;
                cfg.loudnessCompensationGainDb = 4.5f;
                break;
            case 2: // Farfield
            default:
                cfg.distanceMeters = 4.2f;
                cfg.angleDegrees = 60.0f;
                cfg.heightMeters = 1.45f;
                cfg.drrDb = 1.5f;
                cfg.airAbsorptionHfDb = -1.8f;
                cfg.loudnessCompensationGainDb = 8.2f;
                break;
        }
        return cfg;
    }

    // Calculates speaker 3D position relative to origin (listener head center)
    static void getSpeakerCoordinates(float distance, float totalAngleDeg, Vec3D& leftSpk, Vec3D& rightSpk) noexcept {
        float halfAngleRad = (totalAngleDeg * 0.5f) * (juce::MathConstants<float>::pi / 180.0f);
        leftSpk.x = -distance * std::sin(halfAngleRad);
        leftSpk.y = distance * std::cos(halfAngleRad);
        leftSpk.z = 0.0f;

        rightSpk.x = distance * std::sin(halfAngleRad);
        rightSpk.y = distance * std::cos(halfAngleRad);
        rightSpk.z = 0.0f;
    }
};

} // namespace fieldroom
