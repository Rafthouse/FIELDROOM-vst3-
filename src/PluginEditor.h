#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/SpeakerViewComponent.h"

namespace fieldroom {

/**
 * @brief High-end native hardware-inspired JUCE AudioProcessorEditor for FIELDROOM.
 *
 * Implements the precision dark-obsidian aesthetic, tactical segmented field switches,
 * interactive acoustic stage geometry, knurled metallic rotary controls,
 * and comprehensive real-time stereo diagnostics (correlometer and peak meters).
 */
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

    // 1. Top Header Bar
    juce::ToggleButton bypassButton{"ACTIVE"};

    // 2. Sub-Header Preset & Segmented Field Controls
    juce::ComboBox presetSelector;
    juce::TextButton slotAButton{"A"};
    juce::TextButton slotBButton{"B"};

    juce::TextButton nearButton{"NEARFIELD  1.1 m"};
    juce::TextButton midButton{"MIDFIELD  2.2 m"};
    juce::TextButton farButton{"FARFIELD  4.2 m"};

    // 3. Center Left Acoustic Stage Geometry
    SpeakerViewComponent speakerView;

    // 4. Center Right Environment & Rotary Controls Panel
    juce::ComboBox roomSelector;
    juce::ComboBox headphoneSelector;
    juce::ToggleButton calibrationToggle{"ACTIVE"};

    // 4 Precision Rotary Knobs
    juce::Slider speakerSpreadSlider;
    juce::Label speakerSpreadLabel;

    juce::Slider roomAmountSlider;
    juce::Label roomAmountLabel;

    juce::Slider headRotationSlider;
    juce::Label headRotationLabel;

    juce::Slider outputTrimSlider;
    juce::Label outputTrimLabel;

    // 5. Bottom Monitoring Utilities & Diagnostics Row
    juce::TextButton stereoButton{"STEREO"};
    juce::TextButton monoButton{"MONO"};
    juce::TextButton sideButton{"SIDE"};
    juce::TextButton leftSoloButton{"L"};
    juce::TextButton rightSoloButton{"R"};

    juce::TextButton swapButton{"SWAP"};
    juce::TextButton dimButton{"DIM -18dB"};
    juce::TextButton eqOnlyButton{"EQ ONLY"};
    juce::ToggleButton loudnessMatchButton{"LOUDNESS MATCH"};

    // Metering State Cached from Audio Engine
    MeterLevels latestMeters;

    // APVTS Attachments
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

    // Cached descriptions for current room profile
    juce::String getRoomDescription(int roomIdx) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FieldroomAudioProcessorEditor)
};

} // namespace fieldroom
