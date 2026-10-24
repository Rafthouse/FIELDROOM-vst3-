#include "CustomLookAndFeel.h"
#include <cmath>

namespace fieldroom {

const juce::Colour CustomLookAndFeel::canvas        = juce::Colour(0xff0a0c0d);
const juce::Colour CustomLookAndFeel::surface       = juce::Colour(0xff121619);
const juce::Colour CustomLookAndFeel::surfaceRaised = juce::Colour(0xff1a1f23);
const juce::Colour CustomLookAndFeel::inset         = juce::Colour(0xff0c0f11);
const juce::Colour CustomLookAndFeel::hairline      = juce::Colour(0xff252c31);
const juce::Colour CustomLookAndFeel::hairlineSoft  = juce::Colour(0xff1c2226);
const juce::Colour CustomLookAndFeel::gold          = juce::Colour(0xffcfae78);
const juce::Colour CustomLookAndFeel::goldBright    = juce::Colour(0xffeed7a4);
const juce::Colour CustomLookAndFeel::goldDeep      = juce::Colour(0xff7e6540);
const juce::Colour CustomLookAndFeel::teal          = juce::Colour(0xff8dbac0);
const juce::Colour CustomLookAndFeel::textPrimary   = juce::Colour(0xfff1eee6);
const juce::Colour CustomLookAndFeel::textSecondary = juce::Colour(0xffa9b1b5);
const juce::Colour CustomLookAndFeel::textTertiary  = juce::Colour(0xff838c91);
const juce::Colour CustomLookAndFeel::ledOn         = juce::Colour(0xff6cd6a0);
const juce::Colour CustomLookAndFeel::ledWarn       = juce::Colour(0xffe3a94e);
const juce::Colour CustomLookAndFeel::ledClip       = juce::Colour(0xffe5574e);

juce::Font CustomLookAndFeel::titleFont(float size) {
    return juce::Font(size, juce::Font::bold).withExtraKerningFactor(0.06f);
}

juce::Font CustomLookAndFeel::bodyFont(float size, bool bold) {
    return juce::Font(size, bold ? juce::Font::bold : juce::Font::plain);
}

juce::Font CustomLookAndFeel::labelFont(float size) {
    return juce::Font(size, juce::Font::bold).withExtraKerningFactor(0.14f);
}

juce::String CustomLookAndFeel::degreeSign() {
    return juce::String::fromUTF8("\xc2\xb0");
}

CustomLookAndFeel::CustomLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, canvas);
    setColour(juce::ComboBox::backgroundColourId, inset);
    setColour(juce::ComboBox::textColourId, textPrimary);
    setColour(juce::ComboBox::outlineColourId, hairline);
    setColour(juce::ComboBox::arrowColourId, gold);
    setColour(juce::PopupMenu::backgroundColourId, surfaceRaised);
    setColour(juce::PopupMenu::textColourId, textPrimary);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, inset);
    setColour(juce::PopupMenu::highlightedTextColourId, goldBright);
    setColour(juce::Slider::textBoxTextColourId, textPrimary);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, gold.withAlpha(0.3f));
}

void CustomLookAndFeel::drawLed(juce::Graphics& g, juce::Point<float> c, float d, juce::Colour colour, bool lit) {
    if (lit) {
        g.setColour(colour.withAlpha(0.25f));
        g.fillEllipse(juce::Rectangle<float>(d * 2.4f, d * 2.4f).withCentre(c));
    }
    g.setColour(lit ? colour : hairline);
    g.fillEllipse(juce::Rectangle<float>(d, d).withCentre(c));
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider) {
    juce::ignoreUnused(slider);
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height)).reduced(5.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Body: soft radial gradient, hairline rim
    juce::ColourGradient body(surfaceRaised.brighter(0.05f), centre.x, centre.y - radius * 0.6f,
                              inset, centre.x, centre.y + radius, true);
    g.setGradientFill(body);
    g.fillEllipse(bounds);
    g.setColour(hairline);
    g.drawEllipse(bounds, 1.0f);

    // Track and value arc
    const float trackR = radius - 4.0f;
    juce::Path track;
    track.addCentredArc(centre.x, centre.y, trackR, trackR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(hairlineSoft);
    g.strokePath(track, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPosProportional > 0.002f) {
        juce::Path value;
        value.addCentredArc(centre.x, centre.y, trackR, trackR, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(gold.withAlpha(0.18f));
        g.strokePath(value, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(gold);
        g.strokePath(value, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Cap with a fine concentric finish
    const float capR = radius * 0.66f;
    juce::ColourGradient cap(surfaceRaised.brighter(0.10f), centre.x, centre.y - capR,
                             surface.darker(0.2f), centre.x, centre.y + capR, false);
    g.setGradientFill(cap);
    g.fillEllipse(juce::Rectangle<float>(capR * 2.0f, capR * 2.0f).withCentre(centre));
    g.setColour(hairlineSoft.withAlpha(0.7f));
    g.drawEllipse(juce::Rectangle<float>(capR * 1.5f, capR * 1.5f).withCentre(centre), 0.8f);

    // Pointer
    const auto inner = centre.getPointOnCircumference(capR * 0.35f, angle);
    const auto outer = centre.getPointOnCircumference(capR * 0.88f, angle);
    g.setColour(goldBright);
    g.drawLine(juce::Line<float>(inner, outer), 2.0f);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(backgroundColour);
    // Segment of a shared container: the container itself is painted by the editor
    const auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    if (on) {
        g.setColour(gold.withAlpha(0.16f));
        g.fillRect(bounds.reduced(1.0f, 2.0f));
        g.setColour(gold);
        g.fillRect(juce::Rectangle<float>(bounds.getX() + 10.0f, bounds.getBottom() - 2.0f, bounds.getWidth() - 20.0f, 2.0f));
    } else if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted) {
        g.setColour(textPrimary.withAlpha(0.05f));
        g.fillRect(bounds.reduced(1.0f, 2.0f));
    }
}

void CustomLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(shouldDrawButtonAsDown);
    const bool on = button.getToggleState();
    g.setColour(on ? goldBright : (shouldDrawButtonAsHighlighted ? textPrimary : textSecondary));
    g.setFont(labelFont(9.5f));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 0), juce::Justification::centred, 1);
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(shouldDrawButtonAsDown);
    const bool on = button.getToggleState();
    auto pill = button.getLocalBounds().toFloat().reduced(0.5f);
    const float radius = pill.getHeight() * 0.5f;

    g.setColour(on ? gold.withAlpha(0.12f) : inset);
    g.fillRoundedRectangle(pill, radius);
    g.setColour(on ? gold.withAlpha(0.7f) : (shouldDrawButtonAsHighlighted ? textTertiary : hairline));
    g.drawRoundedRectangle(pill, radius, 1.0f);

    const auto led = juce::Point<float>(pill.getX() + 13.0f, pill.getCentreY());
    drawLed(g, led, 6.0f, ledOn, on);

    g.setColour(on ? goldBright : textSecondary);
    g.setFont(labelFont(9.0f));
    g.drawFittedText(button.getButtonText(), pill.toNearestInt().withTrimmedLeft(24).reduced(4, 0),
                     juce::Justification::centredLeft, 1);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box) {
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH, box);
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);

    g.setColour(isButtonDown ? surfaceRaised : inset);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(box.hasKeyboardFocus(false) ? gold.withAlpha(0.6f) : hairline);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Chevron
    const float cx = static_cast<float>(width) - 16.0f;
    const float cy = static_cast<float>(height) * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
    chevron.lineTo(cx, cy + 2.0f);
    chevron.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(gold);
    g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
    label.setBounds(12, 1, box.getWidth() - 36, box.getHeight() - 2);
    label.setFont(bodyFont(11.0f, false));
    label.setJustificationType(juce::Justification::centredLeft);
}

void CustomLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    // Used for slider value readouts: transparent, centred, primary colour
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(bodyFont(11.0f, true));
    g.drawFittedText(label.getText(), label.getLocalBounds().reduced(2, 0), juce::Justification::centred, 1);
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    g.setColour(surfaceRaised);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(hairline);
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted,
                                          bool isTicked, bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColour) {
    juce::ignoreUnused(hasSubMenu, shortcutKeyText, icon, textColour);

    if (isSeparator) {
        g.setColour(hairline);
        g.drawLine(static_cast<float>(area.getX() + 10), static_cast<float>(area.getCentreY()),
                   static_cast<float>(area.getRight() - 10), static_cast<float>(area.getCentreY()), 1.0f);
        return;
    }

    auto item = area.toFloat().reduced(4.0f, 1.0f);
    if (isHighlighted && isActive) {
        g.setColour(gold.withAlpha(0.12f));
        g.fillRoundedRectangle(item, 5.0f);
    }

    g.setColour(!isActive ? textTertiary : (isHighlighted ? goldBright : textPrimary));
    g.setFont(bodyFont(11.0f, isTicked));
    g.drawFittedText(text, area.reduced(16, 0).withTrimmedLeft(isTicked ? 10 : 0), juce::Justification::centredLeft, 1);

    if (isTicked) {
        g.setColour(gold);
        g.fillEllipse(static_cast<float>(area.getX() + 8), static_cast<float>(area.getCentreY() - 2), 4.0f, 4.0f);
    }
}

} // namespace fieldroom
