#include "CustomLookAndFeel.h"
#include <cmath>

namespace fieldroom {

const juce::Colour CustomLookAndFeel::darkCharcoal     = juce::Colour(0xff121316);
const juce::Colour CustomLookAndFeel::panelBackground  = juce::Colour(0xff181a1f);
const juce::Colour CustomLookAndFeel::metalBorder      = juce::Colour(0xff2d3139);
const juce::Colour CustomLookAndFeel::accentAmber      = juce::Colour(0xffd49836);
const juce::Colour CustomLookAndFeel::accentCyan       = juce::Colour(0xff36b9d4);
const juce::Colour CustomLookAndFeel::textMuted        = juce::Colour(0xff8a909d);
const juce::Colour CustomLookAndFeel::textBright       = juce::Colour(0xfff0f2f5);

CustomLookAndFeel::CustomLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, darkCharcoal);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider) {
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto center = bounds.getCentre();

    // Outer background circle
    g.setColour(panelBackground.darker(0.2f));
    g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

    // Subtle metallic rim
    g.setColour(metalBorder);
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // Value Arc Track
    auto arcRadius = radius - 4.0f;
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(metalBorder.darker(0.3f));
    g.strokePath(backgroundArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active Value Arc
    juce::Path valueArc;
    valueArc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(slider.isEnabled() ? accentAmber : textMuted);
    g.strokePath(valueArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Dial Needle indicator
    auto needleLength = radius * 0.72f;
    auto needleEnd = center.getPointOnCircumference(needleLength, toAngle);
    g.setColour(textBright);
    g.drawLine(center.x, center.y, needleEnd.x, needleEnd.y, 2.0f);

    // Inner dial cap
    float capRadius = radius * 0.35f;
    g.setColour(darkCharcoal);
    g.fillEllipse(center.x - capRadius, center.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
    g.setColour(metalBorder);
    g.drawEllipse(center.x - capRadius, center.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& backgroundColour,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    bool isToggled = button.getToggleState();

    juce::Colour bg = panelBackground;
    if (isToggled) {
        bg = accentAmber.withAlpha(0.22f);
    } else if (shouldDrawButtonAsHighlighted) {
        bg = metalBorder.withAlpha(0.4f);
    }

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Border
    g.setColour(isToggled ? accentAmber : (shouldDrawButtonAsHighlighted ? textMuted : metalBorder));
    g.drawRoundedRectangle(bounds, 3.0f, isToggled ? 1.5f : 1.0f);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int buttonX, int buttonY, int buttonW, int buttonH,
                                    juce::ComboBox& box) {
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH, box);
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    g.setColour(panelBackground);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(metalBorder);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // Arrow icon
    juce::Path arrow;
    float arrowX = width - 16.0f;
    float arrowY = height * 0.5f - 2.0f;
    arrow.startNewSubPath(arrowX, arrowY);
    arrow.lineTo(arrowX + 4.0f, arrowY + 5.0f);
    arrow.lineTo(arrowX + 8.0f, arrowY);
    g.setColour(textMuted);
    g.strokePath(arrow, juce::PathStrokeType(1.5f));
}

} // namespace fieldroom
