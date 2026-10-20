#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace fieldroom {

/**
 * @brief Ultra-premium LookAndFeel for FIELDROOM VST3/AU native GUI.
 *
 * Emulates high-end studio mastering hardware (milled obsidian aluminum,
 * precision knurled rotary knobs with glass reflections, tactical segmented buttons,
 * analog LED indicators, and champagne-gold / phosphor-teal lighting).
 */
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    // Rotary Slider with precision milled knurling, glass glare, and glowing value track
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    // Tactical Push Buttons with LED status indicators, inset bevels, and tactile click states
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // Hardware Inset Dropdown ComboBoxes
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu, const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    // Studio Hardware Palette
    static const juce::Colour bgObsidian;       // #121618 deep charcoal chassis
    static const juce::Colour panelSurface;     // #161B1D satin panel
    static const juce::Colour panelRaised;      // #202729 card container
    static const juce::Colour panelMilled;      // #1A2123 recessed bezel
    static const juce::Colour metalBorder;      // #2A3437 milled aluminum seam
    static const juce::Colour metalBorderLight; // #3D4A4E highlight edge
    static const juce::Colour accentGold;       // #C4A46E champagne studio gold
    static const juce::Colour accentGoldBright; // #E5C992 gold highlight
    static const juce::Colour accentGoldDark;   // #8A6E3D deep gold shadow
    static const juce::Colour accentCyan;       // #83ADB4 phosphor teal
    static const juce::Colour ledGreen;         // #22C55E active power / ok
    static const juce::Colour ledAmber;         // #F59E0B warning / caution
    static const juce::Colour ledRed;           // #EF4444 clip indicator
    static const juce::Colour textBright;       // #E5E8E3 primary legibility
    static const juce::Colour textMuted;        // #9CA9A7 secondary technical
    static const juce::Colour textDim;          // #647371 etched labels
};

} // namespace fieldroom
