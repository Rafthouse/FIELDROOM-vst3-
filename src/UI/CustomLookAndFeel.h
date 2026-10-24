#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace fieldroom {

/**
 * Visual system for FIELDROOM: graphite chassis, champagne-gold accent, one type scale.
 *
 * Components are styled by role rather than by one-off drawing:
 *  - segmented buttons: TextButton with the "segment" property (one group, shared container drawn by the editor)
 *  - pills: ToggleButton, with an LED that lights when the state is on
 *  - rotary knobs: arc track, gold value arc, and a readout that is painted by drawLabel()
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    // ---- Palette -------------------------------------------------------------------------
    static const juce::Colour canvas;        // window background
    static const juce::Colour surface;       // card background
    static const juce::Colour surfaceRaised; // elevated element
    static const juce::Colour inset;         // recessed well (combo boxes, meters, stage)
    static const juce::Colour hairline;      // 1 px dividers and borders
    static const juce::Colour hairlineSoft;  // subtle borders
    static const juce::Colour gold;          // accent
    static const juce::Colour goldBright;    // accent highlight
    static const juce::Colour goldDeep;      // accent shadow
    static const juce::Colour teal;          // informative accent (routing, listener)
    static const juce::Colour textPrimary;
    static const juce::Colour textSecondary;
    static const juce::Colour textTertiary;  // labels; >= 4.5:1 on surface
    static const juce::Colour ledOn;         // positive state
    static const juce::Colour ledWarn;       // bypass, not verified
    static const juce::Colour ledClip;       // clipping

    // ---- Type scale ----------------------------------------------------------------------
    static juce::Font titleFont(float size = 15.0f);
    static juce::Font bodyFont(float size = 11.0f, bool bold = false);
    static juce::Font labelFont(float size = 8.5f);   // uppercase, letter-spaced
    static juce::String degreeSign();

    // ---- LookAndFeel overrides -----------------------------------------------------------
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

private:
    static void drawLed(juce::Graphics& g, juce::Point<float> centre, float diameter,
                        juce::Colour colour, bool lit);
};

} // namespace fieldroom
