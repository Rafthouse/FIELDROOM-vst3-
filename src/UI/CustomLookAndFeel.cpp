#include "CustomLookAndFeel.h"
#include <cmath>

namespace fieldroom {

const juce::Colour CustomLookAndFeel::darkCharcoal     = juce::Colour(0xff121316);
const juce::Colour CustomLookAndFeel::panelBackground  = juce::Colour(0xff181a1f);
const juce::Colour CustomLookAndFeel::metalBorder      = juce::Colour(0xff2d3139);
const juce::Colour CustomLookAndFeel::accentAmber      = juce::Colour(0xffc4a46e);
const juce::Colour CustomLookAndFeel::accentCyan       = juce::Colour(0xff36b9d4);
const juce::Colour CustomLookAndFeel::textMuted        = juce::Colour(0xff8a909d);
const juce::Colour CustomLookAndFeel::textBright       = juce::Colour(0xfff0f2f5);

namespace {

void drawSoftShadow (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    // Layered ambient occlusion / soft drop shadow beneath the knob body.
    for (int i = 6; i >= 1; --i)
    {
        const float spread = radius + (float) i * 2.0f;
        const float alpha  = 0.030f + (float) (6 - i) * 0.012f;
        g.setColour (juce::Colours::black.withAlpha (alpha));
        g.fillEllipse (centre.x - spread, centre.y - spread + (float) i * 0.8f,
                       spread * 2.0f, spread * 2.0f);
    }
}

void drawMilledBezel (juce::Graphics& g, juce::Point<float> centre, float outerRadius)
{
    const auto inner = outerRadius - 3.0f;

    // Brushed aluminium bezel: light top-left, dark bottom-right.
    juce::ColourGradient bezel (juce::Colour (0xff9aa0aa), centre.x - outerRadius * 0.55f, centre.y - outerRadius * 0.75f,
                                juce::Colour (0xff34383f), centre.x + outerRadius * 0.55f, centre.y + outerRadius * 0.65f, false);
    bezel.addColour (0.45, juce::Colour (0xff5d626b));
    g.setGradientFill (bezel);
    g.fillEllipse (centre.x - outerRadius, centre.y - outerRadius, outerRadius * 2.0f, outerRadius * 2.0f);

    // Specular rim on the top edge of the bezel.
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    juce::Path rim;
    rim.addCentredArc (centre.x, centre.y, outerRadius - 0.5f, outerRadius - 0.5f, 0.0f,
                       juce::MathConstants<float>::pi * 1.05f, juce::MathConstants<float>::pi * 1.95f, true);
    g.strokePath (rim, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Recessed step between bezel and cap.
    g.setColour (juce::Colour (0xff0c0d10));
    g.drawEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f, 1.6f);
}

void drawKnurling (juce::Graphics& g, juce::Point<float> centre, float radius, int ticks)
{
    // Radial knurling machined into the cap edge.
    for (int i = 0; i < ticks; ++i)
    {
        const float angle = juce::MathConstants<float>::twoPi * (float) i / (float) ticks;
        const auto dir = juce::Point<float> (std::cos (angle), std::sin (angle));
        const auto a = centre + dir * (radius * 0.90f);
        const auto b = centre + dir * (radius * 0.975f);

        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine ({ a.x - 0.5f, a.y - 0.5f, b.x - 0.5f, b.y - 0.5f }, 0.7f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine ({ a.x + 0.5f, a.y + 0.5f, b.x + 0.5f, b.y + 0.5f }, 0.7f);
    }
}

void drawGlassGlare (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    // Curved lens highlight across the upper third of the cap.
    const auto glare = juce::Rectangle<float> (centre.x - radius * 0.78f, centre.y - radius * 0.92f,
                                               radius * 1.56f, radius * 0.92f);

    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.26f), glare.getCentreX(), glare.getY(),
                                juce::Colours::white.withAlpha (0.0f), glare.getCentreX(), glare.getBottom(), false);
    g.setGradientFill (sheen);
    g.fillEllipse (glare);
}

} // namespace

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, darkCharcoal);
    setColour (juce::Slider::rotarySliderFillColourId, accentAmber);
    setColour (juce::Slider::thumbColourId, accentAmber);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff20232a));
    setColour (juce::ComboBox::outlineColourId, metalBorder);
    setColour (juce::ComboBox::textColourId, textBright);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff22252c));
    setColour (juce::TextButton::textColourOffId, textBright);
}

void CustomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    drawSoftShadow (g, centre, radius);
    drawMilledBezel (g, centre, radius);

    const auto capRadius = radius - 8.0f;

    // Darkened cap with a soft radial falloff.
    juce::ColourGradient cap (juce::Colour (0xff3a3e46), centre.x, centre.y - capRadius * 0.6f,
                              juce::Colour (0xff15171b), centre.x, centre.y + capRadius, true);
    g.setGradientFill (cap);
    g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    drawKnurling (g, centre, capRadius, 72);

    // Recessed arc groove.
    const auto arcRadius = capRadius - 6.0f;
    juce::Path groove;
    groove.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff0b0c0e));
    g.strokePath (groove, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // LED arc in champagne gold, with a glow pass.
    if (slider.isEnabled())
    {
        juce::Path active;
        active.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                              rotaryStartAngle, toAngle, true);

        g.setColour (accentAmber.withAlpha (0.22f));
        g.strokePath (active, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (accentAmber);
        g.strokePath (active, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Pointer notch.
    const auto pointer = centre.getPointOnCircumference (capRadius * 0.62f, toAngle);
    g.setColour (juce::Colour (0xffe8e2d2).withAlpha (slider.isEnabled() ? 0.92f : 0.35f));
    g.drawLine ({ centre.x + (pointer.x - centre.x) * 0.42f, centre.y + (pointer.y - centre.y) * 0.42f,
                  pointer.x, pointer.y }, 2.4f);

    drawGlassGlare (g, centre, capRadius);
}

void CustomLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto corner = 6.0f;

    // Segmented tactile switch: raised when off, inset when on.
    juce::Colour top = backgroundColour.brighter (0.10f);
    juce::Colour bottom = backgroundColour.darker (0.35f);

    if (button.getToggleState())
    {
        top = backgroundColour.darker (0.20f);
        bottom = backgroundColour.brighter (0.04f);
    }
    if (shouldDrawButtonAsDown)
    {
        top = backgroundColour.darker (0.30f);
        bottom = backgroundColour.darker (0.10f);
    }

    g.setGradientFill (juce::ColourGradient (top, bounds.getCentreX(), bounds.getY(),
                                             bottom, bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (juce::Colours::black.withAlpha (button.getToggleState() ? 0.55f : 0.35f));
    g.drawRoundedRectangle (bounds, corner, 1.2f);

    if (! button.getToggleState())
    {
        g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsHighlighted ? 0.14f : 0.07f));
        g.drawLine (bounds.getX() + corner, bounds.getY() + 1.0f,
                    bounds.getRight() - corner, bounds.getY() + 1.0f, 1.0f);
    }
}

void CustomLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff262a31), 0.0f, 0.0f,
                                             juce::Colour (0xff171a1f), 0.0f, (float) height, false));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (metalBorder);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

    juce::Path arrow;
    const auto cx = (float) (buttonX + buttonW / 2);
    const auto cy = (float) (buttonY + buttonH / 2);
    arrow.startNewSubPath (cx - 4.5f, cy - 2.0f);
    arrow.lineTo (cx, cy + 3.0f);
    arrow.lineTo (cx + 4.5f, cy - 2.0f);
    g.setColour (textMuted);
    g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::ignoreUnused (isButtonDown);
}

} // namespace fieldroom
