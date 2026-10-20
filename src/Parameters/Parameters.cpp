#include "Parameters.h"

namespace fieldroom {

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Speaker Field: Near (0), Mid (1), Far (2)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::speakerField, 1),
        "Speaker Field",
        juce::StringArray{"Nearfield", "Midfield", "Farfield"},
        1 // Default Midfield
    ));

    // Room Profile
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::roomProfile, 1),
        "Acoustic Room",
        juce::StringArray{
            "Neutral Reference",
            "Small Studio",
            "Professional Control Room",
            "Large Control Room",
            "Domestic Listening Room"
        },
        2 // Default Pro Control Room
    ));

    // Room Amount: 0.0 to 1.0 (0% to 100%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::roomAmount, 1),
        "Room Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.50f
    ));

    // Head Rotation: -90.0 deg to +90.0 deg
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::headRotation, 1),
        "Head Rotation",
        juce::NormalisableRange<float>(-90.0f, 90.0f, 0.5f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("deg")
    ));

    // Headphone Model index (0: Generic / Raw, 1..N: Calibrated models)
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(ParameterIDs::headphoneModel, 1),
        "Headphone Model",
        0, 32, 1 // Default 1 (e.g. Sennheiser HD 600)
    ));

    // Headphone Calibration Toggle
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::calibrationEnabled, 1),
        "Calibration Active",
        true
    ));

    // Automatic Preamp Toggle
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::calibrationPreampAuto, 1),
        "Auto Preamp Headroom",
        true
    ));

    // Manual Calibration Preamp Gain (-18 dB to +6 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::calibrationPreampGain, 1),
        "Preamp Gain",
        juce::NormalisableRange<float>(-18.0f, 6.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Monitoring Mode (Mutually exclusive: Normal, Mono, Side, LeftSolo, RightSolo)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID(ParameterIDs::monitoringMode, 1),
        "Monitoring Mode",
        juce::StringArray{"Normal Stereo", "Mono (-3dB)", "Side (L-R)", "Left Solo", "Right Solo"},
        0
    ));

    // L/R Channel Swap Toggle (Independent monitoring utility)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::swapLREnabled, 1),
        "Swap L/R",
        false
    ));

    // DIM Toggle (-18 dB)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::dimEnabled, 1),
        "DIM (-18dB)",
        false
    ));

    // Headphone EQ Only (bypasses room & virtual speakers, retains headphone correction)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::headphoneEqOnly, 1),
        "Headphone EQ Only",
        false
    ));

    // Master Bypass (true bypass of all processing)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::masterBypass, 1),
        "Master Bypass",
        false
    ));

    // Output Trim (-24 dB to +12 dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::outputTrim, 1),
        "Output Trim",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    // Perceived Loudness Match toggle
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::loudnessMatch, 1),
        "Perceived Loudness Match",
        true
    ));

    return layout;
}

} // namespace fieldroom
