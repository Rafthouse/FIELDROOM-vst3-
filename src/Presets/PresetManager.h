#pragma once

#include <vector>
#include <string>

namespace fieldroom {

/**
 * Factory preset. Values are plain (not normalised) and map 1:1 onto the APVTS parameters.
 * Calibration and bypass state are left to the user and are not part of a preset.
 */
struct Preset {
    std::string name;
    int speakerField;     // 0 = Near, 1 = Mid, 2 = Far
    int roomProfile;      // index into RoomEngine::getRoomParameters()
    float roomAmount;     // 0..1
    float spreadDeg;      // 30..110
    bool eqOnly;          // Headphone EQ only
};

class PresetManager {
public:
    static std::vector<Preset> getFactoryPresets() {
        return {
            {"Balanced Midfield (2.2m / Pro Studio / 60\xc2\xb0)",    1, 2, 0.50f, 60.0f, false},
            {"Intimate Nearfield (1.1m / Small Studio / 60\xc2\xb0)", 0, 1, 0.30f, 60.0f, false},
            {"Cinema Farfield (4.2m / Large Control / 60\xc2\xb0)",   2, 3, 0.65f, 60.0f, false},
            {"Narrow Mastering Field (2.2m / 45\xc2\xb0)",            1, 0, 0.25f, 45.0f, false},
            {"Dry Reference (Neutral, minimal room)",                 0, 0, 0.05f, 60.0f, false},
            {"Domestic Playback Check",                               1, 4, 0.70f, 60.0f, false},
            {"Headphone EQ Only",                                     1, 2, 0.50f, 60.0f, true}
        };
    }
};

} // namespace fieldroom
