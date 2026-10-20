#include "SpeakerViewComponent.h"
#include "CustomLookAndFeel.h"
#include "../DSP/SpeakerGeometry.h"
#include <cmath>

namespace fieldroom {

SpeakerViewComponent::SpeakerViewComponent() {
}

void SpeakerViewComponent::setSpeakerField(int fieldIndex) {
    if (currentField != fieldIndex) {
        currentField = fieldIndex;
        repaint();
    }
}

void SpeakerViewComponent::setHeadRotation(float rotationDegrees) {
    if (std::abs(currentRotationDeg - rotationDegrees) > 0.1f) {
        currentRotationDeg = rotationDegrees;
        repaint();
    }
}

void SpeakerViewComponent::setRoomProfile(int profileIndex) {
    if (currentProfile != profileIndex) {
        currentProfile = profileIndex;
        repaint();
    }
}

void SpeakerViewComponent::resized() {
}

void SpeakerViewComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();

    // Dark acoustic chamber background
    g.setColour(CustomLookAndFeel::darkCharcoal.darker(0.15f));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Subtle polar grid lines
    float headCenterX = w * 0.5f;
    float headCenterY = h * 0.82f;

    g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.25f));
    for (float r : {w * 0.25f, w * 0.45f, w * 0.65f}) {
        g.drawEllipse(headCenterX - r, headCenterY - r, r * 2.0f, r * 2.0f, 1.0f);
    }
    g.drawLine(headCenterX, headCenterY - (h * 0.75f), headCenterX, headCenterY + 10.0f, 1.0f);

    // Speaker Field Distances & Scale
    auto cfg = SpeakerGeometry::getFieldConfig(currentField);
    float distanceNorm = (currentField == 0) ? 0.35f : (currentField == 1 ? 0.55f : 0.78f);
    float speakerY = headCenterY - (h * distanceNorm);
    float speakerSpreadX = (h * distanceNorm) * std::tan(30.0f * juce::MathConstants<float>::pi / 180.0f);

    float spkLeftX = headCenterX - speakerSpreadX;
    float spkRightX = headCenterX + speakerSpreadX;

    // Direct sound acoustic rays (dashed line)
    g.setColour(CustomLookAndFeel::accentCyan.withAlpha(0.45f));
    float dashPattern[] = { 4.0f, 4.0f };
    g.drawDashedLine(juce::Line<float>(spkLeftX, speakerY, headCenterX, headCenterY), dashPattern, 2, 1.5f);
    g.drawDashedLine(juce::Line<float>(spkRightX, speakerY, headCenterX, headCenterY), dashPattern, 2, 1.5f);

    // Virtual Loudspeakers (Left & Right)
    auto drawSpeaker = [&](float sx, float sy, float angleDeg, const juce::String& label) {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(angleDeg * juce::MathConstants<float>::pi / 180.0f, sx, sy));

        // Speaker cabinet
        juce::Rectangle<float> spkBox(sx - 16.0f, sy - 10.0f, 32.0f, 20.0f);
        g.setColour(juce::Colour(0xff22252c));
        g.fillRoundedRectangle(spkBox, 2.0f);
        g.setColour(CustomLookAndFeel::accentAmber);
        g.drawRoundedRectangle(spkBox, 2.0f, 1.5f);

        // Woofer & Tweeter circles
        g.setColour(juce::Colour(0xff141518));
        g.fillEllipse(sx - 6.0f, sy - 4.0f, 12.0f, 12.0f);
        g.setColour(CustomLookAndFeel::textMuted);
        g.drawEllipse(sx - 6.0f, sy - 4.0f, 12.0f, 12.0f, 1.0f);

        g.restoreState();

        // Label
        g.setColour(CustomLookAndFeel::textBright);
        g.setFont(10.0f);
        g.drawText(label, static_cast<int>(sx - 20), static_cast<int>(sy - 24), 40, 12, juce::Justification::centred);
    };

    drawSpeaker(spkLeftX, speakerY, 30.0f, "SPK L");
    drawSpeaker(spkRightX, speakerY, -30.0f, "SPK R");

    // Distance Badge
    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(11.0f);
    juce::String distStr = juce::String(cfg.distanceMeters, 1) + " m (" + juce::String(static_cast<int>(cfg.angleDegrees)) + "°)";
    g.drawText(distStr, static_cast<int>(headCenterX - 60), static_cast<int>(speakerY + 12), 120, 16, juce::Justification::centred);

    // Listener Head at bottom center with Head Rotation
    g.saveState();
    float rotRad = currentRotationDeg * (juce::MathConstants<float>::pi / 180.0f);
    g.addTransform(juce::AffineTransform::rotation(rotRad, headCenterX, headCenterY));

    // Head circle
    float headR = 14.0f;
    g.setColour(juce::Colour(0xff2b303c));
    g.fillEllipse(headCenterX - headR, headCenterY - headR, headR * 2.0f, headR * 2.0f);
    g.setColour(CustomLookAndFeel::accentCyan);
    g.drawEllipse(headCenterX - headR, headCenterY - headR, headR * 2.0f, headR * 2.0f, 1.5f);

    // Ears
    g.setColour(CustomLookAndFeel::metalBorder);
    g.fillRoundedRectangle(headCenterX - headR - 4.0f, headCenterY - 4.0f, 4.0f, 8.0f, 2.0f);
    g.fillRoundedRectangle(headCenterX + headR, headCenterY - 4.0f, 4.0f, 8.0f, 2.0f);

    // Nose direction needle
    g.setColour(CustomLookAndFeel::accentCyan);
    g.drawLine(headCenterX, headCenterY - headR, headCenterX, headCenterY - headR - 6.0f, 2.0f);

    g.restoreState();

    // Rotation angle overlay
    if (std::abs(currentRotationDeg) > 0.5f) {
        g.setColour(CustomLookAndFeel::accentCyan);
        g.setFont(10.0f);
        juce::String rotStr = juce::String(currentRotationDeg, 0) + "°";
        g.drawText(rotStr, static_cast<int>(headCenterX + 20), static_cast<int>(headCenterY - 8), 40, 16, juce::Justification::left);
    }
}

} // namespace fieldroom
