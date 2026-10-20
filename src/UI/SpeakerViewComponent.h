#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace fieldroom {

class SpeakerViewComponent : public juce::Component {
public:
    SpeakerViewComponent();

    void setSpeakerField(int fieldIndex);
    void setHeadRotation(float rotationDegrees);
    void setRoomProfile(int profileIndex);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int currentField{1};
    float currentRotationDeg{0.0f};
    int currentProfile{2};
};

} // namespace fieldroom
