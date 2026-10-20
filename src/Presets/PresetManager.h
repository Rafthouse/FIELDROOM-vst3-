#pragma once

#include <vector>
#include <string>
#include <juce_core/juce_core.h>

namespace fieldroom {

struct Preset {
    std::string name;
    int speakerField;
    int roomProfile;
    float roomAmount;
    float headRotation;
    bool calibrationEnabled;
    bool eqOnly;
    float outputTrim;
};

class PresetManager {
public:
    static std::vector<Preset> getFactoryPresets() {
        return {
            {"Critical Nearfield", 0, 0, 0.25f, 0.0f, true, false, 0.0f},
            {"Balanced Midfield", 1, 2, 0.50f, 0.0f, true, false, 0.0f},
            {"Main Monitors", 2, 3, 0.65f, 0.0f, true, false, 0.0f},
            {"Dry Reference", 0, 0, 0.05f, 0.0f, true, false, 0.0f},
            {"Small Control Room", 1, 1, 0.40f, 0.0f, true, false, 0.0f},
            {"Large Control Room", 2, 3, 0.60f, 0.0f, true, false, 0.0f},
            {"Domestic Playback Check", 1, 4, 0.70f, 0.0f, true, false, 0.0f},
            {"Headphone EQ Only", 1, 2, 0.00f, 0.0f, true, true, 0.0f}
        };
    }
};

} // namespace fieldroom
