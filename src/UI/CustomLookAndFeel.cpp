#include "CustomLookAndFeel.h"
#include <cmath>

namespace fieldroom {

const juce::Colour CustomLookAndFeel::bgObsidian       = juce::Colour(0xff121618);
const juce::Colour CustomLookAndFeel::panelSurface     = juce::Colour(0xff161b1d);
const juce::Colour CustomLookAndFeel::panelRaised      = juce::Colour(0xff202729);
const juce::Colour CustomLookAndFeel::panelMilled      = juce::Colour(0xff1a2123);
const juce::Colour CustomLookAndFeel::metalBorder      = juce::Colour(0xff2a3437);
const juce::Colour CustomLookAndFeel::metalBorderLight = juce::Colour(0xff3d4a4e);
const juce::Colour CustomLookAndFeel::accentGold       = juce::Colour(0xffc4a46e);
const juce::Colour CustomLookAndFeel::accentGoldBright = juce::Colour(0xffe5c992);
const juce::Colour CustomLookAndFeel::accentGoldDark   = juce::Colour(0xff8a6e3d);
const juce::Colour CustomLookAndFeel::accentCyan       = juce::Colour(0xff83adb4);
const juce::Colour CustomLookAndFeel::ledGreen         = juce::Colour(0xff22c55e);
const juce::Colour CustomLookAndFeel::ledAmber         = juce::Colour(0xfff59e0b);
const juce::Colour CustomLookAndFeel::ledRed           = juce::Colour(0xffef4444);
const juce::Colour CustomLookAndFeel::textBright       = juce::Colour(0xffe5e8e3);
const juce::Colour CustomLookAndFeel::textMuted        = juce::Colour(0xff9ca9a7);
const juce::Colour CustomLookAndFeel::textDim          = juce::Colour(0xff647371);

CustomLookAndFeel::CustomLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, bgObsidian);
    setColour(juce::ComboBox::backgroundColourId, panelMilled);
    setColour(juce::ComboBox::textColourId, textBright);
    setColour(juce::ComboBox::outlineColourId, metalBorder);
    setColour(juce::ComboBox::arrowColourId, accentCyan);
    setColour(juce::PopupMenu::backgroundColourId, panelRaised);
    setColour(juce::PopupMenu::textColourId, textBright);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, panelMilled);
    setColour(juce::PopupMenu::highlightedTextColourId, accentGold);
    setColour(juce::TextButton::textColourOffId, textMuted);
    setColour(juce::TextButton::textColourOnId, accentGold);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider) {
    juce::ignoreUnused(slider);
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto center = bounds.getCentre();
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // 1. Ambient Drop Shadow (Soft multi-layer dark blur)
    for (int i = 3; i >= 1; --i) {
        float shadowRadius = radius + static_cast<float>(i) * 1.5f;
        g.setColour(juce::Colours::black.withAlpha(0.18f / static_cast<float>(i)));
        g.fillEllipse(center.x - shadowRadius, center.y - shadowRadius + 1.5f,
                      shadowRadius * 2.0f, shadowRadius * 2.0f);
    }

    // 2. Milled Outer Bezel (Aluminum chamfer)
    juce::ColourGradient bezelGrad(panelMilled.brighter(0.15f), center.x - radius, center.y - radius,
                                   panelMilled.darker(0.3f), center.x + radius, center.y + radius, false);
    g.setGradientFill(bezelGrad);
    g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(metalBorderLight.withAlpha(0.7f));
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // 3. Recessed Arc Track (Background dark groove)
    auto trackRadius = radius - 5.5f;
    juce::Path bgTrack;
    bgTrack.addCentredArc(center.x, center.y, trackRadius, trackRadius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff0e1214));
    g.strokePath(bgTrack, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(metalBorder.withAlpha(0.5f));
    g.strokePath(bgTrack, juce::PathStrokeType(1.0f));

    // 4. Glowing Active Value Arc (Champagne Gold / Phosphor Cyan)
    if (sliderPosProportional > 0.001f) {
        juce::Path activeTrack;
        activeTrack.addCentredArc(center.x, center.y, trackRadius, trackRadius, 0.0f,
                                 rotaryStartAngle, toAngle, true);

        // Soft bloom around track
        g.setColour(accentGold.withAlpha(0.25f));
        g.strokePath(activeTrack, juce::PathStrokeType(6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Core bright track
        juce::ColourGradient arcGrad(accentGoldDark, center.x - trackRadius, center.y + trackRadius,
                                     accentGoldBright, center.x + trackRadius, center.y - trackRadius, false);
        g.setGradientFill(arcGrad);
        g.strokePath(activeTrack, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 5. Knob Body (Milled Dark Anodized Aluminum with concentric knurling)
    auto knobRadius = radius - 10.0f;
    juce::ColourGradient knobGrad(panelRaised.brighter(0.08f), center.x, center.y - knobRadius,
                                  panelMilled.darker(0.35f), center.x, center.y + knobRadius, false);
    g.setGradientFill(knobGrad);
    g.fillEllipse(center.x - knobRadius, center.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

    // Fine concentric knurling groove
    g.setColour(metalBorder.withAlpha(0.6f));
    g.drawEllipse(center.x - (knobRadius - 2.0f), center.y - (knobRadius - 2.0f),
                  (knobRadius - 2.0f) * 2.0f, (knobRadius - 2.0f) * 2.0f, 0.8f);

    // 6. Specular Curved Glass Glare (Upper hemisphere luxury sheen)
    juce::Path glassGlare;
    glassGlare.addCentredArc(center.x, center.y, knobRadius - 1.0f, knobRadius - 1.0f,
                            0.0f, -juce::MathConstants<float>::pi * 0.75f,
                            -juce::MathConstants<float>::pi * 0.25f, true);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.strokePath(glassGlare, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 7. Precision Pointer Indicator (Illuminated line with gold LED point)
    auto innerPoint = center.getPointOnCircumference(knobRadius * 0.28f, toAngle);
    auto outerPoint = center.getPointOnCircumference(knobRadius * 0.76f, toAngle);

    // Needle shadow
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawLine(innerPoint.x + 0.5f, innerPoint.y + 1.0f, outerPoint.x + 0.5f, outerPoint.y + 1.0f, 2.0f);

    // Needle body
    g.setColour(accentGoldBright);
    g.drawLine(innerPoint.x, innerPoint.y, outerPoint.x, outerPoint.y, 2.0f);

    // Needle glowing tip dot
    g.setColour(accentGold);
    g.fillEllipse(outerPoint.x - 1.5f, outerPoint.y - 1.5f, 3.0f, 3.0f);

    // 8. Recessed Center Screw Cap
    auto capRadius = knobRadius * 0.26f;
    g.setColour(juce::Colour(0xff101416));
    g.fillEllipse(center.x - capRadius, center.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
    g.setColour(metalBorderLight.withAlpha(0.4f));
    g.drawEllipse(center.x - capRadius, center.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 0.8f);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                             const juce::Colour& backgroundColour,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    bool isToggled = button.getToggleState();

    // 1. Button Background Gradient
    if (isToggled || shouldDrawButtonAsDown) {
        juce::ColourGradient activeGrad(panelMilled.darker(0.2f), bounds.getX(), bounds.getY(),
                                       panelRaised.brighter(0.04f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(activeGrad);
        g.fillRoundedRectangle(bounds, 3.5f);

        g.setColour(accentGold.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds, 3.5f, 1.2f);

        g.setColour(accentGold.withAlpha(0.22f));
        g.drawRoundedRectangle(bounds.expanded(1.0f), 4.5f, 1.0f);
    } else {
        juce::Colour topCol = shouldDrawButtonAsHighlighted ? panelRaised.brighter(0.12f) : panelRaised;
        juce::Colour botCol = shouldDrawButtonAsHighlighted ? panelMilled.brighter(0.05f) : panelMilled;

        juce::ColourGradient idleGrad(topCol, bounds.getX(), bounds.getY(),
                                     botCol, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(idleGrad);
        g.fillRoundedRectangle(bounds, 3.5f);

        g.setColour(metalBorderLight.withAlpha(shouldDrawButtonAsHighlighted ? 0.9f : 0.45f));
        g.drawLine(bounds.getX() + 2.0f, bounds.getY() + 0.5f, bounds.getRight() - 2.0f, bounds.getY() + 0.5f, 1.0f);

        g.setColour(shouldDrawButtonAsHighlighted ? metalBorderLight : metalBorder);
        g.drawRoundedRectangle(bounds, 3.5f, 1.0f);
    }
}

void CustomLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                       bool shouldDrawButtonAsHighlighted,
                                       bool shouldDrawButtonAsDown) {
    juce::ignoreUnused(shouldDrawButtonAsDown);
    auto bounds = button.getLocalBounds();
    bool isToggled = button.getToggleState();

    juce::Colour textCol = textMuted;
    if (isToggled) {
        textCol = accentGoldBright;
    } else if (shouldDrawButtonAsHighlighted) {
        textCol = textBright;
    }

    g.setColour(textCol);
    g.setFont(juce::Font(11.0f, isToggled ? juce::Font::bold : juce::Font::plain));
    g.drawFittedText(button.getButtonText(), bounds, juce::Justification::centred, 1);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box) {
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH, box);
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    g.setColour(panelMilled);
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(metalBorder);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    float arrowX = static_cast<float>(width) - 16.0f;
    float arrowY = static_cast<float>(height) * 0.5f - 2.0f;
    juce::Path arrow;
    arrow.startNewSubPath(arrowX, arrowY);
    arrow.lineTo(arrowX + 4.0f, arrowY + 4.5f);
    arrow.lineTo(arrowX + 8.0f, arrowY);

    g.setColour(accentCyan);
    g.strokePath(arrow, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(0.5f);

    g.setColour(panelRaised);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(metalBorderLight);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted,
                                          bool isTicked, bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColour) {
    juce::ignoreUnused(hasSubMenu, shortcutKeyText, icon, textColour);

    if (isSeparator) {
        g.setColour(metalBorder);
        g.drawLine(static_cast<float>(area.getX() + 8), static_cast<float>(area.getCentreY()),
                   static_cast<float>(area.getRight() - 8), static_cast<float>(area.getCentreY()), 1.0f);
        return;
    }

    auto itemBounds = area.toFloat().reduced(2.0f, 1.0f);

    if (isHighlighted && isActive) {
        g.setColour(panelMilled.brighter(0.1f));
        g.fillRoundedRectangle(itemBounds, 3.0f);
        g.setColour(accentGold.withAlpha(0.6f));
        g.drawRoundedRectangle(itemBounds, 3.0f, 1.0f);
    }

    juce::Colour col = textBright;
    if (!isActive) col = textDim;
    else if (isHighlighted) col = accentGoldBright;

    g.setColour(col);
    g.setFont(juce::Font(12.0f, juce::Font::plain));

    auto textArea = area.reduced(12, 0);
    if (isTicked) {
        g.setColour(accentGold);
        g.fillEllipse(static_cast<float>(area.getX() + 4), static_cast<float>(area.getCentreY() - 2), 4.0f, 4.0f);
    }

    g.drawFittedText(text, textArea, juce::Justification::centredLeft, 1);
}

} // namespace fieldroom
