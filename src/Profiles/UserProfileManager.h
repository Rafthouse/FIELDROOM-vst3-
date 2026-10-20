#pragma once

#include <string>
#include <vector>
#include <juce_core/juce_core.h>

namespace fieldroom {

struct UserProfile {
    std::string profileId;
    std::string userName;      // e.g. "Lead Mixing Engineer" or "Mastering Lab"
    std::string headphoneModel;
    bool calibrationActive{true};
    bool autoPreampActive{true};
    float customPreampDb{0.0f};
};

class UserProfileManager {
public:
    static std::vector<UserProfile> getDefaultUserProfiles() {
        return {
            {"prof_1", "Profile A (Producer)", "Sennheiser HD 600", true, true, 0.0f},
            {"prof_2", "Profile B (Student / Mobile)", "Audio-Technica ATH-M50x", true, true, 0.0f},
            {"prof_3", "Profile C (Mastering Suite)", "Audeze LCD-X (2021)", true, true, 0.0f}
        };
    }
};

} // namespace fieldroom
