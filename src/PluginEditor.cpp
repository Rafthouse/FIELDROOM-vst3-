#include "PluginEditor.h"

namespace fieldroom {

FieldroomAudioProcessorEditor::FieldroomAudioProcessorEditor(FieldroomAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&lookAndFeel);

    // Window size (860 x 580 px)
    setSize(860, 580);
    setResizable(true, true);
    setResizeLimits(760, 520, 1200, 800);

    // Title & Persistent Safety Warning
    titleLabel.setText("FIELDROOM", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textBright);
    addAndMakeVisible(titleLabel);

    warningLabel.setText("HEADPHONE MONITORING ONLY — DO NOT BOUNCE WITH MASTER", juce::dontSendNotification);
    warningLabel.setFont(juce::Font(10.0f, juce::Font::plain));
    warningLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::accentAmber);
    warningLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(warningLabel);

    addAndMakeVisible(bypassButton);

    // 3 Dominant Field Buttons
    nearButton.setClickingTogglesState(true);
    nearButton.setRadioGroupId(101);
    nearButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::speakerField)) {
            param->setValueNotifyingHost(0.0f);
        }
    };
    addAndMakeVisible(nearButton);

    midButton.setClickingTogglesState(true);
    midButton.setRadioGroupId(101);
    midButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::speakerField)) {
            param->setValueNotifyingHost(0.5f);
        }
    };
    addAndMakeVisible(midButton);

    farButton.setClickingTogglesState(true);
    farButton.setRadioGroupId(101);
    farButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::speakerField)) {
            param->setValueNotifyingHost(1.0f);
        }
    };
    addAndMakeVisible(farButton);

    // Synchronize initial state
    int initField = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerField)->load());
    if (initField == 0) nearButton.setToggleState(true, juce::dontSendNotification);
    else if (initField == 1) midButton.setToggleState(true, juce::dontSendNotification);
    else farButton.setToggleState(true, juce::dontSendNotification);

    addAndMakeVisible(speakerView);

    // Room Selector
    roomSelector.addItem("Neutral Reference", 1);
    roomSelector.addItem("Small Studio", 2);
    roomSelector.addItem("Professional Control Room", 3);
    roomSelector.addItem("Large Control Room", 4);
    roomSelector.addItem("Domestic Listening Room", 5);
    addAndMakeVisible(roomSelector);

    // Headphone Profiles Selector
    auto profiles = HeadphoneCalibration::getBundledProfiles();
    headphoneSelector.clear();
    for (size_t i = 0; i < profiles.size(); ++i) {
        juce::String label = profiles[i].manufacturer + " " + profiles[i].model;
        if (profiles[i].status == CalibrationStatus::CalibrationPending) {
            label += " [Calibration Pending]";
        }
        headphoneSelector.addItem(label, static_cast<int>(i + 1));
    }
    headphoneModelAttach = std::make_unique<ComboBoxAttachment>(processor.getAPVTS(), ParameterIDs::headphoneModel, headphoneSelector);
    addAndMakeVisible(headphoneSelector);

    addAndMakeVisible(calibrationToggle);
    addAndMakeVisible(autoPreampToggle);

    // Rotary Knobs: Room Amount, Head Rotation, Output Trim
    auto setupRotary = [&](juce::Slider& s, juce::Label& l, const juce::String& text, double defaultVal) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        s.setDoubleClickReturnValue(true, defaultVal);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::Font(11.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, CustomLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    setupRotary(roomAmountSlider, roomAmountLabel, "ROOM AMOUNT", 0.5);
    setupRotary(headRotationSlider, headRotationLabel, "HEAD ROTATION", 0.0);
    setupRotary(outputTrimSlider, outputTrimLabel, "OUTPUT TRIM", 0.0);

    // Mutually exclusive routing mode buttons (Radio Group 201)
    monoButton.setClickingTogglesState(true);
    monoButton.setRadioGroupId(201);
    monoButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(monoButton.getToggleState() ? (1.0f / 4.0f) : 0.0f);
        }
    };
    addAndMakeVisible(monoButton);

    sideButton.setClickingTogglesState(true);
    sideButton.setRadioGroupId(201);
    sideButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(sideButton.getToggleState() ? (2.0f / 4.0f) : 0.0f);
        }
    };
    addAndMakeVisible(sideButton);

    leftSoloButton.setClickingTogglesState(true);
    leftSoloButton.setRadioGroupId(201);
    leftSoloButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(leftSoloButton.getToggleState() ? (3.0f / 4.0f) : 0.0f);
        }
    };
    addAndMakeVisible(leftSoloButton);

    rightSoloButton.setClickingTogglesState(true);
    rightSoloButton.setRadioGroupId(201);
    rightSoloButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(rightSoloButton.getToggleState() ? (4.0f / 4.0f) : 0.0f);
        }
    };
    addAndMakeVisible(rightSoloButton);

    // Independent toggles: L/R Swap, DIM, EQ Only
    swapButton.setClickingTogglesState(true);
    addAndMakeVisible(swapButton);

    dimButton.setClickingTogglesState(true);
    addAndMakeVisible(dimButton);

    eqOnlyButton.setClickingTogglesState(true);
    addAndMakeVisible(eqOnlyButton);

    // Attachments
    roomAmountAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::roomAmount, roomAmountSlider);
    headRotationAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::headRotation, headRotationSlider);
    outputTrimAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::outputTrim, outputTrimSlider);

    bypassAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::masterBypass, bypassButton);
    calibrationAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::calibrationEnabled, calibrationToggle);
    autoPreampAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::calibrationPreampAuto, autoPreampToggle);
    swapAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::swapLREnabled, swapButton);
    dimAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::dimEnabled, dimButton);
    eqOnlyAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::headphoneEqOnly, eqOnlyButton);

    roomProfileAttach = std::make_unique<ComboBoxAttachment>(processor.getAPVTS(), ParameterIDs::roomProfile, roomSelector);

    // 30 Hz timer for meter refresh and geometry sync
    startTimerHz(30);
}

FieldroomAudioProcessorEditor::~FieldroomAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void FieldroomAudioProcessorEditor::timerCallback() {
    int curField = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerField)->load());
    float curRot = processor.getAPVTS().getRawParameterValue(ParameterIDs::headRotation)->load();
    int curRoom = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::roomProfile)->load());

    speakerView.setSpeakerField(curField);
    speakerView.setHeadRotation(curRot);
    speakerView.setRoomProfile(curRoom);

    if (curField == 0 && !nearButton.getToggleState()) nearButton.setToggleState(true, juce::dontSendNotification);
    if (curField == 1 && !midButton.getToggleState())  midButton.setToggleState(true, juce::dontSendNotification);
    if (curField == 2 && !farButton.getToggleState())  farButton.setToggleState(true, juce::dontSendNotification);

    // Sync routing mode buttons with processor parameter
    int curMode = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::monitoringMode)->load());
    if (curMode == 0) {
        if (monoButton.getToggleState()) monoButton.setToggleState(false, juce::dontSendNotification);
        if (sideButton.getToggleState()) sideButton.setToggleState(false, juce::dontSendNotification);
        if (leftSoloButton.getToggleState()) leftSoloButton.setToggleState(false, juce::dontSendNotification);
        if (rightSoloButton.getToggleState()) rightSoloButton.setToggleState(false, juce::dontSendNotification);
    } else if (curMode == 1 && !monoButton.getToggleState()) {
        monoButton.setToggleState(true, juce::dontSendNotification);
    } else if (curMode == 2 && !sideButton.getToggleState()) {
        sideButton.setToggleState(true, juce::dontSendNotification);
    } else if (curMode == 3 && !leftSoloButton.getToggleState()) {
        leftSoloButton.setToggleState(true, juce::dontSendNotification);
    } else if (curMode == 4 && !rightSoloButton.getToggleState()) {
        rightSoloButton.setToggleState(true, juce::dontSendNotification);
    }

    repaint();
}

void FieldroomAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(CustomLookAndFeel::darkCharcoal);

    // Top Header bar separator
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawHorizontalLine(44, 0.0f, static_cast<float>(getWidth()));

    // Bottom utilities bar separator
    g.drawHorizontalLine(getHeight() - 74, 0.0f, static_cast<float>(getWidth()));

    // Meter levels readout
    auto levels = processor.getMeterLevels();
    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(10.0f);
    juce::String meterStr = "L: " + juce::String(levels.rmsLeftDb, 1) + " dB  |  R: " +
                            juce::String(levels.rmsRightDb, 1) + " dB  |  Corr: " +
                            juce::String(levels.correlation, 2);
    g.drawText(meterStr, getWidth() - 260, getHeight() - 28, 240, 16, juce::Justification::right);
}

void FieldroomAudioProcessorEditor::resized() {
    auto area = getLocalBounds();

    // 1. Header (44px)
    auto headerArea = area.removeFromTop(44).reduced(12, 6);
    titleLabel.setBounds(headerArea.removeFromLeft(160));
    bypassButton.setBounds(headerArea.removeFromRight(80));
    warningLabel.setBounds(headerArea);

    // 2. Bottom Utility Bar (74px)
    auto bottomArea = area.removeFromBottom(74).reduced(12, 10);
    auto utilRow = bottomArea.removeFromTop(28);
    monoButton.setBounds(utilRow.removeFromLeft(60));
    utilRow.removeFromLeft(6);
    sideButton.setBounds(utilRow.removeFromLeft(60));
    utilRow.removeFromLeft(6);
    leftSoloButton.setBounds(utilRow.removeFromLeft(36));
    utilRow.removeFromLeft(4);
    rightSoloButton.setBounds(utilRow.removeFromLeft(36));
    utilRow.removeFromLeft(6);
    swapButton.setBounds(utilRow.removeFromLeft(60));
    utilRow.removeFromLeft(12);
    dimButton.setBounds(utilRow.removeFromLeft(90));
    utilRow.removeFromLeft(6);
    eqOnlyButton.setBounds(utilRow.removeFromLeft(90));

    // 3. Main Center Workspace
    auto mainArea = area.reduced(14, 10);

    // Top Field Buttons Row (42px)
    auto fieldRow = mainArea.removeFromTop(42);
    int btnWidth = 140;
    int gap = 12;
    int totalFieldWidth = btnWidth * 3 + gap * 2;
    auto fieldCenterArea = fieldRow.withSizeKeepingCentre(totalFieldWidth, 38);

    nearButton.setBounds(fieldCenterArea.removeFromLeft(btnWidth));
    fieldCenterArea.removeFromLeft(gap);
    midButton.setBounds(fieldCenterArea.removeFromLeft(btnWidth));
    fieldCenterArea.removeFromLeft(gap);
    farButton.setBounds(fieldCenterArea.removeFromLeft(btnWidth));

    mainArea.removeFromTop(10);

    // Middle Split: Left side = SpeakerViewComponent; Right side = Selectors & Rotary Knobs
    int rightPanelWidth = 320;
    auto rightArea = mainArea.removeFromRight(rightPanelWidth).reduced(8, 0);
    speakerView.setBounds(mainArea);

    // Right Controls Panel
    roomSelector.setBounds(rightArea.removeFromTop(28));
    rightArea.removeFromTop(8);
    headphoneSelector.setBounds(rightArea.removeFromTop(28));
    rightArea.removeFromTop(6);

    auto togglesRow = rightArea.removeFromTop(24);
    calibrationToggle.setBounds(togglesRow.removeFromLeft(140));
    autoPreampToggle.setBounds(togglesRow.removeFromLeft(140));

    rightArea.removeFromTop(16);

    // Knobs Row
    auto knobsArea = rightArea.removeFromTop(110);
    int knobW = 96;
    auto k1 = knobsArea.removeFromLeft(knobW);
    roomAmountLabel.setBounds(k1.removeFromTop(16));
    roomAmountSlider.setBounds(k1);

    knobsArea.removeFromLeft(8);
    auto k2 = knobsArea.removeFromLeft(knobW);
    headRotationLabel.setBounds(k2.removeFromTop(16));
    headRotationSlider.setBounds(k2);

    knobsArea.removeFromLeft(8);
    auto k3 = knobsArea.removeFromLeft(knobW);
    outputTrimLabel.setBounds(k3.removeFromTop(16));
    outputTrimSlider.setBounds(k3);
}

} // namespace fieldroom
