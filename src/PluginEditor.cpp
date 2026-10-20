#include "PluginEditor.h"
#include <cmath>

namespace fieldroom {

FieldroomAudioProcessorEditor::FieldroomAudioProcessorEditor(FieldroomAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&lookAndFeel);

    // High-Resolution Studio Canvas (960 x 570 px)
    setSize(960, 570);
    setResizable(true, true);
    setResizeLimits(880, 520, 1400, 840);

    // 1. Top Header Controls
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);

    // 2. Sub-Header: Presets & Hardware Segmented Field Selector
    presetSelector.addItem("Balanced Midfield (2.2m / Pro Studio / 60\xc2\xb0)", 1);
    presetSelector.addItem("Intimate Nearfield (1.1m / Small Studio / 60\xc2\xb0)", 2);
    presetSelector.addItem("Cinema Farfield (4.2m / Large Control / 60\xc2\xb0)", 3);
    presetSelector.addItem("Narrow Mastering Field (2.2m / 45\xc2\xb0)", 4);
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.onChange = [this]() {
        const int id = presetSelector.getSelectedId();
        auto setNorm = [this](const char* paramID, float normVal) {
            if (auto* param = processor.getAPVTS().getParameter(paramID)) {
                param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normVal));
            }
        };
        if (id == 1) { // Balanced Midfield
            setNorm(ParameterIDs::speakerField, 0.5f);
            setNorm(ParameterIDs::roomProfile, 2.0f / 4.0f);
            speakerSpreadSlider.setValue(60.0, juce::sendNotificationAsync);
            roomAmountSlider.setValue(0.50, juce::sendNotificationAsync);
            headRotationSlider.setValue(0.0, juce::sendNotificationAsync);
        } else if (id == 2) { // Intimate Nearfield
            setNorm(ParameterIDs::speakerField, 0.0f);
            setNorm(ParameterIDs::roomProfile, 1.0f / 4.0f);
            speakerSpreadSlider.setValue(60.0, juce::sendNotificationAsync);
            roomAmountSlider.setValue(0.30, juce::sendNotificationAsync);
            headRotationSlider.setValue(0.0, juce::sendNotificationAsync);
        } else if (id == 3) { // Cinema Farfield
            setNorm(ParameterIDs::speakerField, 1.0f);
            setNorm(ParameterIDs::roomProfile, 3.0f / 4.0f);
            speakerSpreadSlider.setValue(60.0, juce::sendNotificationAsync);
            roomAmountSlider.setValue(0.65, juce::sendNotificationAsync);
            headRotationSlider.setValue(0.0, juce::sendNotificationAsync);
        } else if (id == 4) { // Narrow Mastering Field
            setNorm(ParameterIDs::speakerField, 0.5f);
            setNorm(ParameterIDs::roomProfile, 0.0f);
            speakerSpreadSlider.setValue(45.0, juce::sendNotificationAsync);
            roomAmountSlider.setValue(0.25, juce::sendNotificationAsync);
            headRotationSlider.setValue(0.0, juce::sendNotificationAsync);
        }
    };
    addAndMakeVisible(presetSelector);

    slotAButton.setClickingTogglesState(true);
    slotAButton.setRadioGroupId(301);
    slotAButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(slotAButton);

    slotBButton.setClickingTogglesState(true);
    slotBButton.setRadioGroupId(301);
    addAndMakeVisible(slotBButton);

    // 3 Hardware Segmented Field Buttons (NEAR | MID | FAR)
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

    // Synchronize initial field toggle
    int initField = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerField)->load());
    if (initField == 0) nearButton.setToggleState(true, juce::dontSendNotification);
    else if (initField == 1) midButton.setToggleState(true, juce::dontSendNotification);
    else farButton.setToggleState(true, juce::dontSendNotification);

    // 3. Center Left Acoustic Stage Geometry
    speakerView.onSpreadChanged = [this](float s) {
        speakerSpreadSlider.setValue(s, juce::sendNotificationAsync);
    };
    speakerView.onRotationChanged = [this](float r) {
        headRotationSlider.setValue(r, juce::sendNotificationAsync);
    };
    speakerView.onResetRotation = [this]() {
        headRotationSlider.setValue(0.0, juce::sendNotificationAsync);
    };
    addAndMakeVisible(speakerView);

    // 4. Center Right Listening Environment & Knobs Panel
    roomSelector.addItem("Neutral Reference (RT60: 0.20s, Early: -14dB)", 1);
    roomSelector.addItem("Small Studio (RT60: 0.28s, Early: -11dB)", 2);
    roomSelector.addItem("Professional Control Room (RT60: 0.38s, Early: -9dB)", 3);
    roomSelector.addItem("Large Control Room (RT60: 0.55s, Early: -7dB)", 4);
    roomSelector.addItem("Domestic Listening Room (RT60: 0.45s, Early: -6dB)", 5);
    addAndMakeVisible(roomSelector);

    // Headphone profiles selector
    auto profiles = HeadphoneCalibration::getBundledProfiles();
    headphoneSelector.clear();
    for (size_t i = 0; i < profiles.size(); ++i) {
        juce::String label = profiles[i].manufacturer + " " + profiles[i].model + " (" + profiles[i].targetCurve + ")";
        if (profiles[i].status == CalibrationStatus::CalibrationPending) {
            label += " [Pending]";
        }
        headphoneSelector.addItem(label, static_cast<int>(i + 1));
    }
    addAndMakeVisible(headphoneSelector);

    calibrationToggle.setClickingTogglesState(true);
    addAndMakeVisible(calibrationToggle);

    // Setup 4 Precision Rotary Knobs
    auto setupRotary = [&](juce::Slider& s, juce::Label& l, const juce::String& text, double defaultVal, const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
        s.setTextValueSuffix(suffix);
        s.setDoubleClickReturnValue(true, defaultVal);
        addAndMakeVisible(s);

        l.setText(text, juce::dontSendNotification);
        l.setFont(juce::Font(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, CustomLookAndFeel::textMuted);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    setupRotary(speakerSpreadSlider, speakerSpreadLabel, "SPREAD", 60.0, "\xc2\xb0");
    setupRotary(roomAmountSlider, roomAmountLabel, "ROOM", 0.50, "%");
    setupRotary(headRotationSlider, headRotationLabel, "ROTATION", 0.0, "\xc2\xb0");
    setupRotary(outputTrimSlider, outputTrimLabel, "TRIM", 0.0, " dB");

    // 5. Bottom Monitoring Utilities & Diagnostics Row
    // Mutually exclusive routing mode buttons (Radio Group 201)
    stereoButton.setClickingTogglesState(true);
    stereoButton.setRadioGroupId(201);
    stereoButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(0.0f);
        }
    };
    addAndMakeVisible(stereoButton);

    monoButton.setClickingTogglesState(true);
    monoButton.setRadioGroupId(201);
    monoButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(1.0f / 4.0f);
        }
    };
    addAndMakeVisible(monoButton);

    sideButton.setClickingTogglesState(true);
    sideButton.setRadioGroupId(201);
    sideButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(2.0f / 4.0f);
        }
    };
    addAndMakeVisible(sideButton);

    leftSoloButton.setClickingTogglesState(true);
    leftSoloButton.setRadioGroupId(201);
    leftSoloButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(3.0f / 4.0f);
        }
    };
    addAndMakeVisible(leftSoloButton);

    rightSoloButton.setClickingTogglesState(true);
    rightSoloButton.setRadioGroupId(201);
    rightSoloButton.onClick = [this]() {
        if (auto* param = processor.getAPVTS().getParameter(ParameterIDs::monitoringMode)) {
            param->setValueNotifyingHost(4.0f / 4.0f);
        }
    };
    addAndMakeVisible(rightSoloButton);

    // Tactical Utility Toggles
    swapButton.setClickingTogglesState(true);
    addAndMakeVisible(swapButton);

    dimButton.setClickingTogglesState(true);
    addAndMakeVisible(dimButton);

    eqOnlyButton.setClickingTogglesState(true);
    addAndMakeVisible(eqOnlyButton);

    loudnessMatchButton.setClickingTogglesState(true);
    addAndMakeVisible(loudnessMatchButton);

    // Attach APVTS parameters
    if (processor.getAPVTS().getParameter(ParameterIDs::speakerSpread) != nullptr) {
        speakerSpreadAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::speakerSpread, speakerSpreadSlider);
    }
    roomAmountAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::roomAmount, roomAmountSlider);
    headRotationAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::headRotation, headRotationSlider);
    outputTrimAttach = std::make_unique<SliderAttachment>(processor.getAPVTS(), ParameterIDs::outputTrim, outputTrimSlider);

    bypassAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::masterBypass, bypassButton);
    calibrationAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::calibrationEnabled, calibrationToggle);
    swapAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::swapLREnabled, swapButton);
    dimAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::dimEnabled, dimButton);
    eqOnlyAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::headphoneEqOnly, eqOnlyButton);
    loudnessMatchAttach = std::make_unique<ButtonAttachment>(processor.getAPVTS(), ParameterIDs::loudnessMatch, loudnessMatchButton);

    roomProfileAttach = std::make_unique<ComboBoxAttachment>(processor.getAPVTS(), ParameterIDs::roomProfile, roomSelector);
    headphoneModelAttach = std::make_unique<ComboBoxAttachment>(processor.getAPVTS(), ParameterIDs::headphoneModel, headphoneSelector);

    // 30 Hz timer for smooth meter rendering and parameter synchronization
    startTimerHz(30);
}

FieldroomAudioProcessorEditor::~FieldroomAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

juce::String FieldroomAudioProcessorEditor::getRoomDescription(int roomIdx) const {
    switch (roomIdx) {
        case 0: return "Anechoic diffuse-field direct listening profile.";
        case 1: return "Tight, dry acoustic response for accurate transient tracking.";
        case 2: return "Balanced symmetrical mastering-grade control room.";
        case 3: return "Spacious studio with extended decay and natural bass bloom.";
        case 4: return "Typical consumer living environment for mix translation check.";
        default: return "Professional reference listening environment.";
    }
}

void FieldroomAudioProcessorEditor::timerCallback() {
    int curField = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerField)->load());
    float curRot = processor.getAPVTS().getRawParameterValue(ParameterIDs::headRotation)->load();
    int curRoom = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::roomProfile)->load());
    float curSpread = (processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerSpread) != nullptr)
                          ? processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerSpread)->load()
                          : 60.0f;

    speakerView.setSpeakerField(curField);
    speakerView.setSpeakerSpread(curSpread);
    speakerView.setHeadRotation(curRot);
    speakerView.setRoomProfile(curRoom);

    // Synchronize field buttons
    if (curField == 0 && !nearButton.getToggleState()) nearButton.setToggleState(true, juce::dontSendNotification);
    if (curField == 1 && !midButton.getToggleState())  midButton.setToggleState(true, juce::dontSendNotification);
    if (curField == 2 && !farButton.getToggleState())  farButton.setToggleState(true, juce::dontSendNotification);

    // Synchronize routing buttons
    int curMode = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::monitoringMode)->load());
    if (curMode == 0 && !stereoButton.getToggleState()) stereoButton.setToggleState(true, juce::dontSendNotification);
    else if (curMode == 1 && !monoButton.getToggleState()) monoButton.setToggleState(true, juce::dontSendNotification);
    else if (curMode == 2 && !sideButton.getToggleState()) sideButton.setToggleState(true, juce::dontSendNotification);
    else if (curMode == 3 && !leftSoloButton.getToggleState()) leftSoloButton.setToggleState(true, juce::dontSendNotification);
    else if (curMode == 4 && !rightSoloButton.getToggleState()) rightSoloButton.setToggleState(true, juce::dontSendNotification);

    // Fetch live audio levels
    latestMeters = processor.getMeterLevels();
    repaint();
}

void FieldroomAudioProcessorEditor::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    // 1. Brushed Obsidian Chassis Background
    juce::ColourGradient bgGrad(CustomLookAndFeel::panelSurface.darker(0.1f), w * 0.5f, 0.0f,
                               CustomLookAndFeel::bgObsidian.darker(0.3f), w * 0.5f, h, false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // 2. Top Header Bar (44px)
    g.setColour(CustomLookAndFeel::panelSurface);
    g.fillRect(0.0f, 0.0f, w, 44.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawLine(0.0f, 44.0f, w, 44.0f, 1.0f);

    // Studio Emblem "FR"
    juce::Rectangle<float> emblemBox(14.0f, 8.0f, 28.0f, 28.0f);
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(emblemBox, 4.0f);
    g.setColour(CustomLookAndFeel::metalBorderLight);
    g.drawRoundedRectangle(emblemBox, 4.0f, 1.0f);
    g.setColour(CustomLookAndFeel::accentGold);
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("FR", emblemBox, juce::Justification::centred);

    // FIELDROOM Wordmark & VST3/AU Chip
    g.setColour(CustomLookAndFeel::textBright);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("FIELDROOM", 50.0f, 8.0f, 96.0f, 16.0f, juce::Justification::centredLeft);

    // VST3 / AU chip badge
    juce::Rectangle<float> chipBox(152.0f, 8.0f, 54.0f, 16.0f);
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(chipBox, 2.5f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(chipBox, 2.5f, 0.8f);
    g.setColour(CustomLookAndFeel::accentCyan);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText("VST3 / AU", chipBox, juce::Justification::centred);

    // Subtitle
    g.setColour(CustomLookAndFeel::textDim);
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText("Binaural Virtual Loudspeaker & Psychoacoustic Monitoring", 50.0f, 24.0f, 320.0f, 14.0f, juce::Justification::centredLeft);

    // Right Active Headphone Badge
    int curH = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::headphoneModel)->load());
    auto profiles = HeadphoneCalibration::getBundledProfiles();
    juce::String hpModelName = (curH >= 0 && curH < static_cast<int>(profiles.size()))
                                   ? juce::String(profiles[static_cast<size_t>(curH)].model)
                                   : juce::String("M-200");

    juce::Rectangle<float> hpBadge(w - 210.0f, 8.0f, 96.0f, 28.0f);
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(hpBadge, 3.5f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(hpBadge, 3.5f, 0.8f);

    g.setColour(CustomLookAndFeel::textBright);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(hpModelName, hpBadge.getX(), hpBadge.getY() + 3.0f, hpBadge.getWidth(), 12.0f, juce::Justification::centred);

    g.setColour(CustomLookAndFeel::accentGold);
    g.setFont(juce::Font(8.5f, juce::Font::bold));
    g.drawText("CALIBRATED", hpBadge.getX(), hpBadge.getY() + 15.0f, hpBadge.getWidth(), 10.0f, juce::Justification::centred);

    // 3. Sub-Header Preset & Segmented Field Strip Background
    float subHeaderY = 45.0f;
    float subHeaderH = 50.0f;
    g.setColour(CustomLookAndFeel::bgObsidian.brighter(0.04f));
    g.fillRect(0.0f, subHeaderY, w, subHeaderH);
    g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.6f));
    g.drawLine(0.0f, subHeaderY + subHeaderH, w, subHeaderY + subHeaderH, 1.0f);

    // PRESET: label
    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("PRESET:", 16.0f, subHeaderY + 16.0f, 54.0f, 16.0f, juce::Justification::centredLeft);

    // Far Right Sub-Header Plaque
    juce::Rectangle<float> hpPlaque(w - 150.0f, subHeaderY + 12.0f, 136.0f, 26.0f);
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(hpPlaque, 3.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(hpPlaque, 3.0f, 0.8f);
    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(9.5f, juce::Font::plain));
    g.drawText("HEADPHONES: " + hpModelName, hpPlaque, juce::Justification::centred);

    // 4. Center Right Environment & Knobs Panel Background
    auto rightPanelBounds = juce::Rectangle<float>(w - 380.0f, 102.0f, 366.0f, 386.0f);
    g.setColour(CustomLookAndFeel::panelSurface);
    g.fillRoundedRectangle(rightPanelBounds, 6.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(rightPanelBounds, 6.0f, 1.0f);

    // Right Panel Section Titles
    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("LISTENING ENVIRONMENT:", rightPanelBounds.getX() + 14.0f, rightPanelBounds.getY() + 10.0f, 200.0f, 14.0f, juce::Justification::centredLeft);
    g.drawText("HEADPHONE CALIBRATION:", rightPanelBounds.getX() + 14.0f, rightPanelBounds.getY() + 66.0f, 200.0f, 14.0f, juce::Justification::centredLeft);

    // Inset Bay for the 4 Precision Knobs
    juce::Rectangle<float> knobsBay(rightPanelBounds.getX() + 8.0f, rightPanelBounds.getY() + 124.0f, rightPanelBounds.getWidth() - 16.0f, 138.0f);
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(knobsBay, 4.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(knobsBay, 4.0f, 0.8f);

    // Inset Engraved Plaque at Bottom of Right Panel: ACOUSTIC SPACE
    juce::Rectangle<float> spacePlaque(rightPanelBounds.getX() + 8.0f, rightPanelBounds.getBottom() - 96.0f, rightPanelBounds.getWidth() - 16.0f, 86.0f);
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(spacePlaque, 4.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(spacePlaque, 4.0f, 0.8f);

    g.setColour(CustomLookAndFeel::accentGoldBright);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("ACOUSTIC SPACE:", spacePlaque.getX() + 10.0f, spacePlaque.getY() + 8.0f, 200.0f, 14.0f, juce::Justification::centredLeft);

    int curRoom = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::roomProfile)->load());
    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawFittedText(getRoomDescription(curRoom), spacePlaque.reduced(10.0f, 24.0f).toNearestInt(), juce::Justification::topLeft, 3);

    // 5. Bottom Monitoring Utilities & Diagnostics Bar (Height: 52px)
    float utilY = h - 68.0f;
    auto utilBounds = juce::Rectangle<float>(14.0f, utilY, w - 28.0f, 44.0f);
    g.setColour(CustomLookAndFeel::panelSurface);
    g.fillRoundedRectangle(utilBounds, 5.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(utilBounds, 5.0f, 1.0f);

    // Group Title: ROUTING
    g.setColour(CustomLookAndFeel::textDim);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("ROUTING", utilBounds.getX() + 10.0f, utilBounds.getCentreY() - 8.0f, 54.0f, 16.0f, juce::Justification::centredLeft);

    // Phase Correlometer (-1 to +1)
    float corrX = utilBounds.getRight() - 250.0f;
    float corrY = utilBounds.getY() + 8.0f;
    float corrW = 86.0f;
    float corrH = 8.0f;

    g.setColour(CustomLookAndFeel::textDim);
    g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.drawText("-1", corrX - 14.0f, corrY - 2.0f, 12.0f, 12.0f, juce::Justification::centredRight);
    g.drawText("PHASE", corrX, corrY - 7.0f, corrW, 10.0f, juce::Justification::centred);
    g.drawText("+1", corrX + corrW + 2.0f, corrY - 2.0f, 14.0f, 12.0f, juce::Justification::centredLeft);

    // Track
    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(corrX, corrY + 5.0f, corrW, corrH, 2.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(corrX, corrY + 5.0f, corrW, corrH, 2.0f, 0.8f);

    // Needle based on correlation (-1 to +1)
    float corrNorm = juce::jlimit(0.0f, 1.0f, (latestMeters.correlation + 1.0f) * 0.5f);
    float needleX = corrX + corrNorm * corrW;
    g.setColour(CustomLookAndFeel::accentGoldBright);
    g.drawLine(needleX, corrY + 4.0f, needleX, corrY + 14.0f, 2.0f);

    // Stereo Peak Meters (L & R)
    float meterX = corrX + corrW + 24.0f;
    float meterY = utilBounds.getY() + 10.0f;
    float meterW = 64.0f;
    float meterH = 5.0f;

    g.setColour(CustomLookAndFeel::textDim);
    g.setFont(juce::Font(8.0f, juce::Font::plain));
    g.drawText("L", meterX - 10.0f, meterY - 2.0f, 8.0f, 10.0f, juce::Justification::centred);
    g.drawText("R", meterX - 10.0f, meterY + 9.0f, 8.0f, 10.0f, juce::Justification::centred);

    auto drawMeterBar = [&](float yPos, float levelDb) {
        g.setColour(CustomLookAndFeel::panelMilled);
        g.fillRoundedRectangle(meterX, yPos, meterW, meterH, 1.5f);
        float norm = juce::jlimit(0.0f, 1.0f, (levelDb + 48.0f) / 48.0f);
        if (norm > 0.01f) {
            juce::Colour meterCol = (levelDb > -0.5f) ? CustomLookAndFeel::ledRed : ((levelDb > -6.0f) ? CustomLookAndFeel::ledAmber : CustomLookAndFeel::ledGreen);
            g.setColour(meterCol);
            g.fillRoundedRectangle(meterX, yPos, meterW * norm, meterH, 1.5f);
        }
    };
    drawMeterBar(meterY, latestMeters.peakLeftDb);
    drawMeterBar(meterY + 11.0f, latestMeters.peakRightDb);

    // 6. Bottom Status Footer Line (18px)
    float footerLineY = h - 18.0f;
    g.setColour(CustomLookAndFeel::textDim);
    g.setFont(juce::Font(9.0f, juce::Font::plain));
    juce::String footText = "FIELDROOM v1.0.0 - VST3 / AU Compatible - Sample Rate: 48.0 kHz - Latency: 0 ms";
    g.drawText(footText, 16.0f, footerLineY, 500.0f, 14.0f, juce::Justification::centredLeft);

    g.setColour(CustomLookAndFeel::ledGreen);
    g.fillEllipse(w - 220.0f, footerLineY + 3.0f, 6.0f, 6.0f);
    g.setColour(CustomLookAndFeel::textMuted);
    g.drawText("Real-Time Safe Audio Thread Ready", w - 208.0f, footerLineY, 192.0f, 14.0f, juce::Justification::centredLeft);
}

void FieldroomAudioProcessorEditor::resized() {
    auto b = getLocalBounds();
    int w = b.getWidth();
    int h = b.getHeight();

    // 1. Top Header (44px)
    bypassButton.setBounds(w - 96, 8, 82, 28);

    // 2. Sub-Header (45px to 95px)
    presetSelector.setBounds(76, 56, 260, 28);
    slotAButton.setBounds(342, 56, 24, 28);
    slotBButton.setBounds(368, 56, 24, 28);

    // Hardware Segmented Field Selector: 3 Prominent Buttons
    int fieldBtnW = 118;
    int fieldStart = (w - (fieldBtnW * 3 + 8)) / 2 + 30;
    nearButton.setBounds(fieldStart, 54, fieldBtnW, 32);
    midButton.setBounds(fieldStart + fieldBtnW + 4, 54, fieldBtnW, 32);
    farButton.setBounds(fieldStart + (fieldBtnW + 4) * 2, 54, fieldBtnW, 32);

    // 3. Center Left Stage Geometry (occupies full left workspace)
    int stageRight = w - 388;
    speakerView.setBounds(14, 102, stageRight - 14, 386);

    // 4. Center Right Listening Environment & Knobs Panel
    int rightX = w - 380;
    roomSelector.setBounds(rightX + 12, 126, 342, 28);
    headphoneSelector.setBounds(rightX + 12, 186, 260, 28);
    calibrationToggle.setBounds(rightX + 276, 186, 78, 28);

    // 4 Precision Knobs in Milled Bay
    int knobBayY = 232;
    int knobW = 82;
    int kGap = 4;
    int kStartX = rightX + 12;

    speakerSpreadLabel.setBounds(kStartX, knobBayY + 4, knobW, 14);
    speakerSpreadSlider.setBounds(kStartX, knobBayY + 20, knobW, 96);

    roomAmountLabel.setBounds(kStartX + knobW + kGap, knobBayY + 4, knobW, 14);
    roomAmountSlider.setBounds(kStartX + knobW + kGap, knobBayY + 20, knobW, 96);

    headRotationLabel.setBounds(kStartX + (knobW + kGap) * 2, knobBayY + 4, knobW, 14);
    headRotationSlider.setBounds(kStartX + (knobW + kGap) * 2, knobBayY + 20, knobW, 96);

    outputTrimLabel.setBounds(kStartX + (knobW + kGap) * 3, knobBayY + 4, knobW, 14);
    outputTrimSlider.setBounds(kStartX + (knobW + kGap) * 3, knobBayY + 20, knobW, 96);

    // 5. Bottom Monitoring Utilities Bar (utilY: h - 68)
    int utilY = h - 64;
    int rX = 76;
    stereoButton.setBounds(rX, utilY, 52, 28);
    monoButton.setBounds(rX + 54, utilY, 48, 28);
    sideButton.setBounds(rX + 104, utilY, 46, 28);
    leftSoloButton.setBounds(rX + 152, utilY, 30, 28);
    rightSoloButton.setBounds(rX + 184, utilY, 30, 28);

    int uX = rX + 226;
    swapButton.setBounds(uX, utilY, 52, 28);
    dimButton.setBounds(uX + 56, utilY, 74, 28);
    eqOnlyButton.setBounds(uX + 134, utilY, 70, 28);

    loudnessMatchButton.setBounds(w - 146, utilY, 128, 28);
}

} // namespace fieldroom
