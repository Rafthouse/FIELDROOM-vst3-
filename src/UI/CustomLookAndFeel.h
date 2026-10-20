#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace fieldroom {

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CustomLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    static const juce::Colour darkCharcoal;
    static const juce::Colour panelBackground;
    static const juce::Colour metalBorder;
    static const juce::Colour accentAmber;
    static const juce::Colour accentCyan;
    static const juce::Colour textMuted;
    static const juce::Colour textBright;
};

} // namespace fieldroom
