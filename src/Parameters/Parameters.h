#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/MonitoringTypes.h"

namespace fieldroom {

enum class SpeakerField : int {
    Near = 0,
    Mid = 1,
    Far = 2
};

enum class RoomProfile : int {
    NeutralReference = 0,
    SmallStudio = 1,
    ProControlRoom = 2,
    LargeControlRoom = 3,
    DomesticRoom = 4
};

struct ParameterIDs {
    static constexpr const char* speakerField = "speaker_field";
    static constexpr const char* speakerSpread = "speaker_spread";
    static constexpr const char* roomProfile = "room_profile";
    static constexpr const char* roomAmount = "room_amount";
    static constexpr const char* headRotation = "head_rotation";
    static constexpr const char* headphoneModel = "headphone_model";
    static constexpr const char* calibrationEnabled = "calibration_enabled";
    static constexpr const char* calibrationPreampAuto = "calibration_preamp_auto";
    static constexpr const char* calibrationPreampGain = "calibration_preamp_gain";
    static constexpr const char* monitoringMode = "monitoring_mode";
    static constexpr const char* swapLREnabled = "swap_lr";
    static constexpr const char* dimEnabled = "dim_enabled";
    static constexpr const char* headphoneEqOnly = "headphone_eq_only";
    static constexpr const char* masterBypass = "master_bypass";
    static constexpr const char* outputTrim = "output_trim";
    static constexpr const char* loudnessMatch = "loudness_match";
};

class ParameterManager {
public:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
};

} // namespace fieldroom
