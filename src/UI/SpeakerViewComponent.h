#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace fieldroom {

/**
 * @brief High-precision native acoustic control room geometry component.
 *
 * Renders the virtual listening stage with studio wall diffusers/absorbers,
 * polar concentric distance arcs, precision studio monitor loudspeakers,
 * ray beam propagation lines, and an anatomical binaural listener head with headphones.
 * Supports direct mouse drag interaction for speaker spread and head orientation.
 */
class SpeakerViewComponent : public juce::Component {
public:
    SpeakerViewComponent();
    ~SpeakerViewComponent() override = default;

    void setSpeakerField(int fieldIndex);
    void setSpeakerSpread(float spreadDegrees);
    void setHeadRotation(float rotationDegrees);
    void setRoomProfile(int profileIndex);

    std::function<void(float spreadDeg)> onSpreadChanged;
    std::function<void(float rotDeg)> onRotationChanged;
    std::function<void()> onResetRotation;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    int currentField{1};             // 0: Near, 1: Mid, 2: Far
    float currentSpreadDeg{60.0f};   // 30° to 110°
    float currentRotationDeg{0.0f};  // -90° to +90°
    int currentProfile{2};           // Room profile

    enum class DragTarget {
        None,
        LeftSpeaker,
        RightSpeaker,
        ListenerHead
    };
    DragTarget activeDragTarget{DragTarget::None};

    juce::Point<float> getHeadCenter() const;
    void getSpeakerPositions(juce::Point<float>& spkL, juce::Point<float>& spkR, float& radiusPx) const;

    juce::Rectangle<int> resetBtnBounds;
    juce::Rectangle<int> spread45BtnBounds;
    juce::Rectangle<int> spread60BtnBounds;
    juce::Rectangle<int> spread75BtnBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpeakerViewComponent)
};

} // namespace fieldroom
