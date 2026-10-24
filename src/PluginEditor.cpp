#include "PluginEditor.h"
#include <cmath>

namespace fieldroom {

namespace {
using CLF = CustomLookAndFeel;

constexpr int kDefaultWidth = 960;
constexpr int kDefaultHeight = 620;
constexpr int kMinWidth = 880;
constexpr int kMinHeight = 620;
constexpr int kMaxWidth = 1400;
constexpr int kMaxHeight = 900;

constexpr float kMargin = 18.0f;
constexpr float kHeaderH = 60.0f;
constexpr float kToolbarY = 70.0f;
constexpr float kToolbarH = 28.0f;
constexpr float kStageTop = 112.0f;
constexpr float kPanelW = 330.0f;
constexpr float kRoutingH = 56.0f;
constexpr float kFooterH = 26.0f;
constexpr float kFieldGroupW = 360.0f;

void drawCard(juce::Graphics& g, juce::Rectangle<float> r, float radius) {
    juce::ColourGradient fill(CLF::surfaceRaised, r.getX(), r.getY(), CLF::surface, r.getX(), r.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(r, radius);
    g.setColour(CLF::hairline);
    g.drawRoundedRectangle(r, radius, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.035f));
    g.drawHorizontalLine(static_cast<int>(r.getY() + 1.0f), r.getX() + radius, r.getRight() - radius);
}

void drawSectionTitle(juce::Graphics& g, float x, float y, float width, const juce::String& text) {
    g.setColour(CLF::gold);
    g.fillEllipse(x, y + 4.0f, 4.0f, 4.0f);
    g.setColour(CLF::textTertiary);
    g.setFont(CLF::labelFont(7.5f));
    g.drawText(text, juce::Rectangle<float>(x + 10.0f, y, width - 10.0f, 12.0f), juce::Justification::centredLeft);
}

void drawRecessedGroup(juce::Graphics& g, juce::Rectangle<float> r) {
    g.setColour(CLF::inset);
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(CLF::hairline);
    g.drawRoundedRectangle(r, 7.0f, 1.0f);
}

void drawLevelBar(juce::Graphics& g, juce::Rectangle<float> r, float levelDb) {
    constexpr int segments = 32;
    const float segW = (r.getWidth() - (segments - 1) * 1.5f) / static_cast<float>(segments);
    const float litUpTo = juce::jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f) * segments;

    for (int i = 0; i < segments; ++i) {
        const float x = r.getX() + static_cast<float>(i) * (segW + 1.5f);
        const float fraction = static_cast<float>(i + 1);
        const bool lit = fraction <= litUpTo + 0.001f;
        const float segDb = -60.0f + (fraction / segments) * 60.0f;
        juce::Colour c = (segDb > -0.5f) ? CLF::ledClip : (segDb > -6.0f ? CLF::ledWarn : CLF::ledOn);
        g.setColour(lit ? c : CLF::hairline.withAlpha(0.7f));
        g.fillRoundedRectangle(x, r.getY(), segW, r.getHeight(), 1.0f);
    }
}
} // namespace

FieldroomAudioProcessorEditor::FieldroomAudioProcessorEditor(FieldroomAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&lookAndFeel);

    setSize(kDefaultWidth, kDefaultHeight);
    setResizable(true, true);
    setResizeLimits(kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);

    auto& apvts = processor.getAPVTS();

    // Header
    bypassButton.setClickingTogglesState(true);
    bypassAttach = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::masterBypass, bypassButton);
    addAndMakeVisible(bypassButton);

    // Toolbar: presets
    presets = PresetManager::getFactoryPresets();
    int itemId = 1;
    for (const auto& preset : presets) {
        presetSelector.addItem(juce::String(preset.name), itemId++);
    }
    customPresetId = itemId;
    presetSelector.addItem("Custom", customPresetId);
    presetSelector.onChange = [this]() {
        const int selected = presetSelector.getSelectedId();
        if (selected >= 1 && selected < customPresetId) {
            applyPreset(presets[static_cast<size_t>(selected - 1)]);
        }
    };
    addAndMakeVisible(presetSelector);

    // Toolbar: field segments
    auto setupSegment = [this](juce::TextButton& button, int groupId) {
        button.setClickingTogglesState(true);
        button.setRadioGroupId(groupId);
        button.getProperties().set("segment", true);
        addAndMakeVisible(button);
    };
    setupSegment(nearButton, 101);
    setupSegment(midButton, 101);
    setupSegment(farButton, 101);
    nearButton.onClick = [this]() { setParameterValue(ParameterIDs::speakerField, 0.0f); };
    midButton.onClick = [this]() { setParameterValue(ParameterIDs::speakerField, 1.0f); };
    farButton.onClick = [this]() { setParameterValue(ParameterIDs::speakerField, 2.0f); };

    // Stage
    speakerView.onSpreadChanged = [this](float s) { speakerSpreadSlider.setValue(s, juce::sendNotificationAsync); };
    speakerView.onRotationChanged = [this](float r) { headRotationSlider.setValue(r, juce::sendNotificationAsync); };
    speakerView.onResetRotation = [this]() { headRotationSlider.setValue(0.0, juce::sendNotificationAsync); };
    addAndMakeVisible(speakerView);

    // Environment panel
    for (int i = 0; i < 5; ++i) {
        const auto room = RoomEngine::getRoomParameters(i);
        roomSelector.addItem(juce::String(room.name), i + 1);
    }
    addAndMakeVisible(roomSelector);
    roomProfileAttach = std::make_unique<ComboBoxAttachment>(apvts, ParameterIDs::roomProfile, roomSelector);

    headphoneProfiles = HeadphoneCalibration::getBundledProfiles();
    for (size_t i = 0; i < headphoneProfiles.size(); ++i) {
        const auto& profile = headphoneProfiles[i];
        juce::String label = juce::String(profile.manufacturer) + " " + juce::String(profile.model);
        if (profile.status == CalibrationStatus::CalibrationPending) {
            label += "  (pending)";
        }
        headphoneSelector.addItem(label, static_cast<int>(i + 1));
    }
    addAndMakeVisible(headphoneSelector);
    headphoneModelAttach = std::make_unique<ComboBoxAttachment>(apvts, ParameterIDs::headphoneModel, headphoneSelector);

    calibrationToggle.setClickingTogglesState(true);
    calibrationAttach = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::calibrationEnabled, calibrationToggle);
    addAndMakeVisible(calibrationToggle);

    loudnessMatchButton.setClickingTogglesState(true);
    loudnessMatchAttach = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::loudnessMatch, loudnessMatchButton);
    addAndMakeVisible(loudnessMatchButton);

    // Knobs: titles are drawn in paint(); the slider draws its own value readout below
    auto setupKnob = [this](juce::Slider& s, double defaultVal, const juce::String& suffix) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 18);
        s.setTextValueSuffix(suffix);
        s.setDoubleClickReturnValue(true, defaultVal);
        addAndMakeVisible(s);
    };
    setupKnob(speakerSpreadSlider, 60.0, CLF::degreeSign());
    setupKnob(roomAmountSlider, 0.50, "%");
    setupKnob(headRotationSlider, 0.0, CLF::degreeSign());
    setupKnob(outputTrimSlider, 0.0, " dB");

    speakerSpreadAttach = std::make_unique<SliderAttachment>(apvts, ParameterIDs::speakerSpread, speakerSpreadSlider);
    roomAmountAttach = std::make_unique<SliderAttachment>(apvts, ParameterIDs::roomAmount, roomAmountSlider);
    headRotationAttach = std::make_unique<SliderAttachment>(apvts, ParameterIDs::headRotation, headRotationSlider);
    outputTrimAttach = std::make_unique<SliderAttachment>(apvts, ParameterIDs::outputTrim, outputTrimSlider);

    // Routing bar
    auto setupMode = [this](juce::TextButton& button, int modeIndex) {
        button.setClickingTogglesState(true);
        button.setRadioGroupId(201);
        button.getProperties().set("segment", true);
        button.onClick = [this, modeIndex]() {
            setParameterValue(ParameterIDs::monitoringMode, static_cast<float>(modeIndex));
        };
        addAndMakeVisible(button);
    };
    setupMode(stereoButton, 0);
    setupMode(monoButton, 1);
    setupMode(sideButton, 2);
    setupMode(leftSoloButton, 3);
    setupMode(rightSoloButton, 4);

    swapButton.setClickingTogglesState(true);
    swapAttach = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::swapLREnabled, swapButton);
    addAndMakeVisible(swapButton);

    dimButton.setClickingTogglesState(true);
    dimAttach = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::dimEnabled, dimButton);
    addAndMakeVisible(dimButton);

    eqOnlyButton.setClickingTogglesState(true);
    eqOnlyAttach = std::make_unique<ButtonAttachment>(apvts, ParameterIDs::headphoneEqOnly, eqOnlyButton);
    addAndMakeVisible(eqOnlyButton);

    timerCallback();
    startTimerHz(30);
}

FieldroomAudioProcessorEditor::~FieldroomAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void FieldroomAudioProcessorEditor::setParameterValue(const juce::String& paramId, float plainValue) {
    if (auto* param = processor.getAPVTS().getParameter(paramId)) {
        param->setValueNotifyingHost(param->convertTo0to1(plainValue));
    }
}

void FieldroomAudioProcessorEditor::applyPreset(const Preset& preset) {
    setParameterValue(ParameterIDs::speakerField, static_cast<float>(preset.speakerField));
    setParameterValue(ParameterIDs::roomProfile, static_cast<float>(preset.roomProfile));
    setParameterValue(ParameterIDs::roomAmount, preset.roomAmount);
    setParameterValue(ParameterIDs::speakerSpread, preset.spreadDeg);
    setParameterValue(ParameterIDs::headRotation, 0.0f);
    setParameterValue(ParameterIDs::headphoneEqOnly, preset.eqOnly ? 1.0f : 0.0f);
}

int FieldroomAudioProcessorEditor::findMatchingPresetIndex() const {
    auto& apvts = processor.getAPVTS();
    auto raw = [&apvts](const juce::String& id) { return apvts.getRawParameterValue(id)->load(); };

    const int field = static_cast<int>(raw(ParameterIDs::speakerField));
    const int room = static_cast<int>(raw(ParameterIDs::roomProfile));
    const float amount = raw(ParameterIDs::roomAmount);
    const float spread = raw(ParameterIDs::speakerSpread);
    const float rotation = raw(ParameterIDs::headRotation);
    const bool eqOnly = raw(ParameterIDs::headphoneEqOnly) > 0.5f;

    for (size_t i = 0; i < presets.size(); ++i) {
        const auto& p = presets[i];
        if (p.speakerField == field && p.roomProfile == room
            && std::abs(p.roomAmount - amount) < 0.005f
            && std::abs(p.spreadDeg - spread) < 0.5f
            && std::abs(rotation) < 0.5f
            && p.eqOnly == eqOnly) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void FieldroomAudioProcessorEditor::syncPresetSelector() {
    const int match = findMatchingPresetIndex();
    const int wanted = (match >= 0) ? match + 1 : customPresetId;
    if (presetSelector.getSelectedId() != wanted) {
        presetSelector.setSelectedId(wanted, juce::dontSendNotification);
    }
}

void FieldroomAudioProcessorEditor::timerCallback() {
    auto& apvts = processor.getAPVTS();
    auto raw = [&apvts](const juce::String& id) { return apvts.getRawParameterValue(id)->load(); };

    const int curField = static_cast<int>(raw(ParameterIDs::speakerField));
    speakerView.setSpeakerField(curField);
    speakerView.setSpeakerSpread(raw(ParameterIDs::speakerSpread));
    speakerView.setHeadRotation(raw(ParameterIDs::headRotation));
    speakerView.setRoomProfile(static_cast<int>(raw(ParameterIDs::roomProfile)));

    juce::TextButton* fieldButtons[] = { &nearButton, &midButton, &farButton };
    for (int i = 0; i < 3; ++i) {
        if (i == curField && !fieldButtons[i]->getToggleState()) {
            fieldButtons[i]->setToggleState(true, juce::dontSendNotification);
        }
    }

    const int curMode = static_cast<int>(raw(ParameterIDs::monitoringMode));
    juce::TextButton* modeButtons[] = { &stereoButton, &monoButton, &sideButton, &leftSoloButton, &rightSoloButton };
    for (int i = 0; i < 5; ++i) {
        if (i == curMode && !modeButtons[i]->getToggleState()) {
            modeButtons[i]->setToggleState(true, juce::dontSendNotification);
        }
    }

    syncPresetSelector();
    latestMeters = processor.getMeterLevels();
    repaint();
}

void FieldroomAudioProcessorEditor::paint(juce::Graphics& g) {
    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());

    // Canvas: a faint top light, darker toward the bottom
    juce::ColourGradient canvas(CLF::canvas.brighter(0.05f), w * 0.5f, 0.0f, CLF::canvas.darker(0.25f), w * 0.5f, h, false);
    g.setGradientFill(canvas);
    g.fillAll();

    paintHeader(g, w);

    // Toolbar
    g.setColour(CLF::textTertiary);
    g.setFont(CLF::labelFont(7.5f));
    g.drawText("PRESET", kMargin, kToolbarY + 8.0f, 60.0f, 12.0f, juce::Justification::centredLeft);
    drawRecessedGroup(g, juce::Rectangle<float>(w - kMargin - kFieldGroupW, kToolbarY, kFieldGroupW, kToolbarH));

    // Stage: painted by SpeakerViewComponent. Environment panel beside it:
    const float stageH = h - kStageTop - kRoutingH - kFooterH - 36.0f;
    const juce::Rectangle<float> panel(w - kMargin - kPanelW, kStageTop, kPanelW, stageH);
    paintEnvironment(g, panel);

    // Routing bar
    const juce::Rectangle<float> bar(kMargin, h - kFooterH - 14.0f - kRoutingH, w - kMargin * 2.0f, kRoutingH);
    paintRouting(g, bar);

    paintFooter(g, w, h);
}

void FieldroomAudioProcessorEditor::paintHeader(juce::Graphics& g, float w) {
    // Logo mark: rounded tile with a gold ring (listener at the centre)
    const auto logo = juce::Rectangle<float>(kMargin, 15.0f, 30.0f, 30.0f);
    juce::ColourGradient tile(CLF::surfaceRaised, logo.getX(), logo.getY(), CLF::surface, logo.getX(), logo.getBottom(), false);
    g.setGradientFill(tile);
    g.fillRoundedRectangle(logo, 8.0f);
    g.setColour(CLF::hairline);
    g.drawRoundedRectangle(logo, 8.0f, 1.0f);
    g.setColour(CLF::gold);
    g.drawEllipse(logo.reduced(8.0f), 1.5f);
    g.fillEllipse(logo.getCentreX() - 2.0f, logo.getCentreY() - 2.0f, 4.0f, 4.0f);

    g.setColour(CLF::textPrimary);
    g.setFont(CLF::titleFont(15.0f));
    g.drawText("FIELDROOM", juce::Rectangle<float>(58.0f, 13.0f, 220.0f, 18.0f), juce::Justification::centredLeft);
    g.setColour(CLF::textTertiary);
    g.setFont(CLF::labelFont(7.5f));
    g.drawText("BINAURAL VIRTUAL LOUDSPEAKER MONITORING", juce::Rectangle<float>(58.0f, 33.0f, 300.0f, 12.0f), juce::Justification::centredLeft);

    // Headphone block, right of the header
    const int curH = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::headphoneModel)->load());
    const bool hasProfile = curH >= 0 && curH < static_cast<int>(headphoneProfiles.size());
    const juce::String modelName = hasProfile ? juce::String(headphoneProfiles[static_cast<size_t>(curH)].manufacturer) + " "
                                                + juce::String(headphoneProfiles[static_cast<size_t>(curH)].model)
                                              : juce::String("No headphone profile");

    const bool calibrationOn = processor.getAPVTS().getRawParameterValue(ParameterIDs::calibrationEnabled)->load() > 0.5f;
    const bool verified = hasProfile && headphoneProfiles[static_cast<size_t>(curH)].status == CalibrationStatus::VerifiedCalibrated;
    const juce::String statusText = !calibrationOn ? "CALIBRATION OFF" : (verified ? "CALIBRATED" : "NOT VERIFIED");
    const juce::Colour statusColour = !calibrationOn ? CLF::textTertiary : (verified ? CLF::ledOn : CLF::ledWarn);

    const float blockRight = w - 144.0f;
    g.setColour(CLF::textPrimary);
    g.setFont(CLF::bodyFont(11.0f, true));
    g.drawText(modelName, juce::Rectangle<float>(blockRight - 240.0f, 13.0f, 240.0f, 16.0f), juce::Justification::centredRight);

    const auto statusFont = CLF::labelFont(7.5f);
    const float statusW = statusFont.getStringWidthFloat(statusText);
    g.setColour(statusColour);
    g.fillEllipse(blockRight - statusW - 17.0f, 35.0f, 5.0f, 5.0f);
    g.setFont(statusFont);
    g.drawText(statusText, juce::Rectangle<float>(blockRight - 240.0f, 31.0f, 240.0f, 12.0f), juce::Justification::centredRight);

    g.setColour(CLF::hairline);
    g.drawVerticalLine(static_cast<int>(w - 134.0f), 14.0f, 46.0f);

    // Header rule
    g.setColour(CLF::hairline);
    g.drawHorizontalLine(static_cast<int>(kHeaderH), 0.0f, w);
}

void FieldroomAudioProcessorEditor::paintEnvironment(juce::Graphics& g, juce::Rectangle<float> panel) {
    drawCard(g, panel, 12.0f);

    const float x = panel.getX() + 16.0f;
    const float innerW = panel.getWidth() - 32.0f;

    drawSectionTitle(g, x, panel.getY() + 14.0f, innerW, "ENVIRONMENT");

    // Stat tiles: values come from the same tables as the DSP
    const int room = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::roomProfile)->load());
    const int field = static_cast<int>(processor.getAPVTS().getRawParameterValue(ParameterIDs::speakerField)->load());
    const auto roomParams = RoomEngine::getRoomParameters(room);
    const float drr = SpeakerGeometry::getFieldConfig(field).drrDb;

    const juce::String tileLabels[3] = { "RT60", "EARLY", "DRR" };
    const juce::String tileValues[3] = {
        juce::String(RoomEngine::getEffectiveRt60(room, field), 2) + " s",
        juce::String(roomParams.earlyReflGainDb, 0) + " dB",
        (drr >= 0.0f ? juce::String("+") : juce::String()) + juce::String(drr, 1) + " dB"
    };
    const float tileW = (innerW - 2.0f * 8.0f) / 3.0f;
    for (int i = 0; i < 3; ++i) {
        const auto tile = juce::Rectangle<float>(x + i * (tileW + 8.0f), panel.getY() + 72.0f, tileW, 46.0f);
        drawRecessedGroup(g, tile);
        g.setColour(CLF::textTertiary);
        g.setFont(CLF::labelFont(7.0f));
        g.drawText(tileLabels[i], tile.reduced(10.0f, 0.0f).withTrimmedTop(7.0f).withHeight(10.0f), juce::Justification::centredLeft);
        g.setColour(CLF::textPrimary);
        g.setFont(CLF::bodyFont(13.0f, true));
        g.drawText(tileValues[i], tile.reduced(10.0f, 0.0f).withTrimmedTop(18.0f), juce::Justification::centredLeft);
    }

    drawSectionTitle(g, x, panel.getY() + 132.0f, innerW, "HEADPHONES");

    drawSectionTitle(g, x, panel.getY() + 226.0f, innerW, "CONTROLS");

    // Knob titles, centred over each knob
    const juce::String knobTitles[4] = { "SPREAD", "ROOM", "ROTATION", "TRIM" };
    for (int i = 0; i < 4; ++i) {
        const float cx = x + 37.0f + i * 75.0f;
        g.setColour(CLF::textTertiary);
        g.setFont(CLF::labelFont(7.0f));
        g.drawText(knobTitles[i], juce::Rectangle<float>(cx - 37.0f, panel.getY() + 244.0f, 74.0f, 12.0f), juce::Justification::centred);
    }

    // Caption: the room's description, two lines at most
    const juce::String descriptions[5] = {
        "Anechoic reference: direct sound, almost no room.",
        "Tight and dry, for transient accuracy.",
        "Balanced, mastering-grade control room.",
        "Spacious studio with long decay and bloom.",
        "Living-room check for how a mix travels."
    };
    g.setColour(CLF::textSecondary);
    g.setFont(CLF::bodyFont(9.5f));
    g.drawFittedText(descriptions[juce::jlimit(0, 4, room)],
                     juce::Rectangle<int>(static_cast<int>(x), static_cast<int>(panel.getBottom() - 34.0f),
                                          static_cast<int>(innerW), 28), juce::Justification::centredLeft, 2);
}

void FieldroomAudioProcessorEditor::paintRouting(juce::Graphics& g, juce::Rectangle<float> bar) {
    drawCard(g, bar, 12.0f);

    // Group the segment strip and the pill toggles, then the meters on the right
    drawRecessedGroup(g, juce::Rectangle<float>(bar.getX() + 12.0f, bar.getY() + 14.0f, 290.0f, 28.0f));

    // Correlometer and peak meters, right-aligned inside the bar
    const float meterW = 120.0f;
    const float meterX = bar.getRight() - 14.0f - meterW;
    const float corrW = 100.0f;
    const float corrX = meterX - 28.0f - corrW;

    g.setColour(CLF::textTertiary);
    g.setFont(CLF::labelFont(7.0f));
    g.drawText("PHASE", juce::Rectangle<float>(corrX, bar.getY() + 7.0f, corrW, 10.0f), juce::Justification::centred);

    const auto track = juce::Rectangle<float>(corrX, bar.getY() + 22.0f, corrW, 4.0f);
    g.setColour(CLF::inset);
    g.fillRoundedRectangle(track, 2.0f);
    g.setColour(CLF::hairline);
    g.drawVerticalLine(static_cast<int>(track.getCentreX()), track.getY() - 2.0f, track.getBottom() + 2.0f);
    const float needleX = track.getX() + juce::jlimit(0.0f, 1.0f, (latestMeters.correlation + 1.0f) * 0.5f) * track.getWidth();
    g.setColour(CLF::goldBright);
    g.fillEllipse(needleX - 3.0f, track.getCentreY() - 3.0f, 6.0f, 6.0f);
    g.setColour(CLF::textTertiary);
    g.setFont(CLF::bodyFont(7.5f));
    g.drawText("-1", juce::Rectangle<float>(corrX, bar.getBottom() - 16.0f, 20.0f, 10.0f), juce::Justification::left);
    g.drawText("+1", juce::Rectangle<float>(corrX + corrW - 20.0f, bar.getBottom() - 16.0f, 20.0f, 10.0f), juce::Justification::right);

    g.setColour(CLF::textTertiary);
    g.setFont(CLF::labelFont(7.0f));
    g.drawText("L", juce::Rectangle<float>(meterX - 12.0f, bar.getY() + 13.0f, 10.0f, 10.0f), juce::Justification::centredLeft);
    g.drawText("R", juce::Rectangle<float>(meterX - 12.0f, bar.getY() + 31.0f, 10.0f, 10.0f), juce::Justification::centredLeft);
    drawLevelBar(g, juce::Rectangle<float>(meterX, bar.getY() + 14.0f, meterW, 6.0f), latestMeters.peakLeftDb);
    drawLevelBar(g, juce::Rectangle<float>(meterX, bar.getY() + 32.0f, meterW, 6.0f), latestMeters.peakRightDb);
}

void FieldroomAudioProcessorEditor::paintFooter(juce::Graphics& g, float w, float h) {
    const float y = h - kFooterH;
    g.setColour(CLF::textTertiary);
    g.setFont(CLF::bodyFont(9.0f));

    const double sampleRate = processor.getCurrentSampleRate();
    const juce::String srText = sampleRate > 0.0 ? juce::String(sampleRate / 1000.0, 1) + " kHz" : juce::String("-- kHz");
    g.drawText("FIELDROOM 1.0.0   /   VST3 + AU   /   " + srText + "   /   latency "
                   + juce::String(processor.getLatencySamples()) + " samples",
               juce::Rectangle<float>(kMargin, y, 560.0f, 14.0f), juce::Justification::centredLeft);

    const bool engineBypassed = processor.getAPVTS().getRawParameterValue(ParameterIDs::masterBypass)->load() > 0.5f;
    g.setColour(engineBypassed ? CLF::ledWarn : CLF::ledOn);
    g.fillEllipse(w - 170.0f, y + 4.0f, 6.0f, 6.0f);
    g.setColour(CLF::textSecondary);
    g.drawText(engineBypassed ? "Engine bypassed" : "Engine active",
               juce::Rectangle<float>(w - 158.0f, y, 140.0f, 14.0f), juce::Justification::centredLeft);
}

void FieldroomAudioProcessorEditor::resized() {
    const float fw = static_cast<float>(getWidth());
    const float fh = static_cast<float>(getHeight());
    const int w = getWidth();
    const int h = getHeight();

    // Header
    bypassButton.setBounds(w - static_cast<int>(kMargin) - 110, 16, 110, 28);

    // Toolbar
    presetSelector.setBounds(84, static_cast<int>(kToolbarY), 300, static_cast<int>(kToolbarH));
    const int groupX = static_cast<int>(fw - kMargin - kFieldGroupW);
    const int segW = static_cast<int>((kFieldGroupW - 4.0f) / 3.0f);
    nearButton.setBounds(groupX + 2, static_cast<int>(kToolbarY) + 2, segW, static_cast<int>(kToolbarH) - 4);
    midButton.setBounds(groupX + 2 + segW, static_cast<int>(kToolbarY) + 2, segW, static_cast<int>(kToolbarH) - 4);
    farButton.setBounds(groupX + 2 + segW * 2, static_cast<int>(kToolbarY) + 2, segW, static_cast<int>(kToolbarH) - 4);

    // Stage
    const int stageH = static_cast<int>(fh - kStageTop - kRoutingH - kFooterH - 36.0f);
    const int stageW = static_cast<int>(fw - kMargin * 2.0f - kPanelW - 14.0f);
    speakerView.setBounds(static_cast<int>(kMargin), static_cast<int>(kStageTop), stageW, stageH);

    // Environment panel
    const int px = static_cast<int>(fw - kMargin - kPanelW);
    const int py = static_cast<int>(kStageTop);
    roomSelector.setBounds(px + 16, py + 30, static_cast<int>(kPanelW - 32.0f), 30);
    headphoneSelector.setBounds(px + 16, py + 148, static_cast<int>(kPanelW - 32.0f), 30);
    calibrationToggle.setBounds(px + 16, py + 186, 146, 26);
    loudnessMatchButton.setBounds(px + 168, py + 186, 146, 26);

    const int knobY = py + 258;
    speakerSpreadSlider.setBounds(px + 16 + 0 * 75, knobY, 74, 96);
    roomAmountSlider.setBounds(px + 16 + 1 * 75, knobY, 74, 96);
    headRotationSlider.setBounds(px + 16 + 2 * 75, knobY, 74, 96);
    outputTrimSlider.setBounds(px + 16 + 3 * 75, knobY, 74, 96);

    // Routing bar
    const int barY = h - static_cast<int>(kFooterH) - 14 - static_cast<int>(kRoutingH);
    const int segBarY = barY + 14;
    const int segStep = 58;
    stereoButton.setBounds(static_cast<int>(kMargin) + 14 + 0 * segStep, segBarY + 2, segStep, 24);
    monoButton.setBounds(static_cast<int>(kMargin) + 14 + 1 * segStep, segBarY + 2, segStep, 24);
    sideButton.setBounds(static_cast<int>(kMargin) + 14 + 2 * segStep, segBarY + 2, segStep, 24);
    leftSoloButton.setBounds(static_cast<int>(kMargin) + 14 + 3 * segStep, segBarY + 2, segStep, 24);
    rightSoloButton.setBounds(static_cast<int>(kMargin) + 14 + 4 * segStep, segBarY + 2, segStep, 24);

    const int toggleX = static_cast<int>(kMargin) + 12 + 290 + 12;
    swapButton.setBounds(toggleX, segBarY, 66, 28);
    dimButton.setBounds(toggleX + 74, segBarY, 78, 28);
    eqOnlyButton.setBounds(toggleX + 160, segBarY, 92, 28);
}

} // namespace fieldroom
