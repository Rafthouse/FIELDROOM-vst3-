#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/SpeakerViewComponent.h"

namespace fieldroom {

class FieldroomAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FieldroomAudioProcessorEditor(FieldroomAudioProcessor&);
    ~FieldroomAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    FieldroomAudioProcessor& processor;
    CustomLookAndFeel lookAndFeel;

    // Header controls
    juce::Label titleLabel;
    juce::Label warningLabel;
    juce::ToggleButton bypassButton{"Bypass"};

    // Speaker Field Buttons (NEAR | MID | FAR)
    juce::TextButton nearButton{"NEAR"};
    juce::TextButton midButton{"MID"};
    juce::TextButton farButton{"FAR"};

    // Visual speaker position component
    SpeakerViewComponent speakerView;

    // Selectors
    juce::ComboBox roomSelector;
    juce::ComboBox headphoneSelector;
    juce::ToggleButton calibrationToggle{"Calibration ON"};
    juce::ToggleButton autoPreampToggle{"Auto Preamp"};

    // Rotary Knobs
    juce::Slider roomAmountSlider;
    juce::Label roomAmountLabel;

    juce::Slider headRotationSlider;
    juce::Label headRotationLabel;

    juce::Slider outputTrimSlider;
    juce::Label outputTrimLabel;

    // Monitoring Utilities
    juce::TextButton monoButton{"MONO"};
    juce::TextButton sideButton{"SIDE"};
    juce::TextButton leftSoloButton{"L"};
    juce::TextButton rightSoloButton{"R"};
    juce::TextButton swapButton{"SWAP"};
    juce::TextButton dimButton{"DIM -18dB"};
    juce::TextButton eqOnlyButton{"EQ ONLY"};

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> roomAmountAttach;
    std::unique_ptr<SliderAttachment> headRotationAttach;
    std::unique_ptr<SliderAttachment> outputTrimAttach;

    std::unique_ptr<ButtonAttachment> bypassAttach;
    std::unique_ptr<ButtonAttachment> calibrationAttach;
    std::unique_ptr<ButtonAttachment> autoPreampAttach;
    std::unique_ptr<ButtonAttachment> dimAttach;
    std::unique_ptr<ButtonAttachment> eqOnlyAttach;
    std::unique_ptr<ButtonAttachment> swapAttach;

    std::unique_ptr<ComboBoxAttachment> roomProfileAttach;
    std::unique_ptr<ComboBoxAttachment> headphoneModelAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldroomAudioProcessorEditor)
};

} // namespace fieldroom
