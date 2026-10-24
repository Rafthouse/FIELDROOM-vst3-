#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Presets/PresetManager.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/SpeakerViewComponent.h"

namespace fieldroom {

/**
 * FIELDROOM editor. Layout is a fixed grid: header, toolbar, stage and environment panel, routing bar,
 * footer. Cards and section titles are drawn in paint(); every control is an APVTS attachment or a
 * parameter write, so the host, presets and on-screen controls always show the same state.
 */
class FieldroomAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FieldroomAudioProcessorEditor(FieldroomAudioProcessor&);
    ~FieldroomAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void applyPreset(const Preset& preset);
    void syncPresetSelector();
    int findMatchingPresetIndex() const;
    void setParameterValue(const juce::String& paramId, float plainValue);

    void paintHeader(juce::Graphics& g, float w);
    void paintEnvironment(juce::Graphics& g, juce::Rectangle<float> panel);
    void paintRouting(juce::Graphics& g, juce::Rectangle<float> bar);
    void paintFooter(juce::Graphics& g, float w, float h);

    FieldroomAudioProcessor& processor;
    CustomLookAndFeel lookAndFeel;

    std::vector<Preset> presets;
    int customPresetId{0};

    // Header
    juce::ToggleButton bypassButton{"BYPASS"};

    // Toolbar
    juce::ComboBox presetSelector;
    juce::TextButton nearButton{"NEARFIELD"};
    juce::TextButton midButton{"MIDFIELD"};
    juce::TextButton farButton{"FARFIELD"};

    // Stage
    SpeakerViewComponent speakerView;

    // Environment panel
    juce::ComboBox roomSelector;
    juce::ComboBox headphoneSelector;
    juce::ToggleButton calibrationToggle{"CALIBRATION"};
    juce::ToggleButton loudnessMatchButton{"LOUDNESS MATCH"};

    juce::Slider speakerSpreadSlider;
    juce::Slider roomAmountSlider;
    juce::Slider headRotationSlider;
    juce::Slider outputTrimSlider;

    // Routing bar
    juce::TextButton stereoButton{"STEREO"};
    juce::TextButton monoButton{"MONO"};
    juce::TextButton sideButton{"SIDE"};
    juce::TextButton leftSoloButton{"L"};
    juce::TextButton rightSoloButton{"R"};

    juce::ToggleButton swapButton{"SWAP"};
    juce::ToggleButton dimButton{"DIM -18"};
    juce::ToggleButton eqOnlyButton{"EQ ONLY"};

    std::vector<HeadphoneProfile> headphoneProfiles;
    MeterLevels latestMeters;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> speakerSpreadAttach;
    std::unique_ptr<SliderAttachment> roomAmountAttach;
    std::unique_ptr<SliderAttachment> headRotationAttach;
    std::unique_ptr<SliderAttachment> outputTrimAttach;

    std::unique_ptr<ButtonAttachment> bypassAttach;
    std::unique_ptr<ButtonAttachment> calibrationAttach;
    std::unique_ptr<ButtonAttachment> swapAttach;
    std::unique_ptr<ButtonAttachment> dimAttach;
    std::unique_ptr<ButtonAttachment> eqOnlyAttach;
    std::unique_ptr<ButtonAttachment> loudnessMatchAttach;

    std::unique_ptr<ComboBoxAttachment> roomProfileAttach;
    std::unique_ptr<ComboBoxAttachment> headphoneModelAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldroomAudioProcessorEditor)
};

} // namespace fieldroom
