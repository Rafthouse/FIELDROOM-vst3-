#include "SpeakerViewComponent.h"
#include "CustomLookAndFeel.h"
#include "../DSP/SpeakerGeometry.h"
#include "../DSP/RoomEngine.h"
#include <cmath>

namespace fieldroom {

namespace {
constexpr float kHeaderH = 38.0f;
constexpr float kFooterH = 40.0f;
constexpr float kPi = juce::MathConstants<float>::pi;
} // namespace

SpeakerViewComponent::SpeakerViewComponent() {
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void SpeakerViewComponent::setSpeakerField(int fieldIndex) {
    if (currentField != fieldIndex) {
        currentField = fieldIndex;
        repaint();
    }
}

void SpeakerViewComponent::setSpeakerSpread(float spreadDegrees) {
    const float clamped = juce::jlimit(30.0f, 110.0f, spreadDegrees);
    if (std::abs(currentSpreadDeg - clamped) > 0.1f) {
        currentSpreadDeg = clamped;
        repaint();
    }
}

void SpeakerViewComponent::setHeadRotation(float rotationDegrees) {
    const float clamped = juce::jlimit(-90.0f, 90.0f, rotationDegrees);
    if (std::abs(currentRotationDeg - clamped) > 0.1f) {
        currentRotationDeg = clamped;
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
    const auto b = getLocalBounds();
    const int footerY = b.getBottom() - static_cast<int>(kFooterH) + 10;

    resetBtnBounds = juce::Rectangle<int>(96, footerY, 78, 20);
    spread45BtnBounds = juce::Rectangle<int>(b.getWidth() - 196, footerY, 52, 20);
    spread60BtnBounds = juce::Rectangle<int>(b.getWidth() - 138, footerY, 60, 20);
    spread75BtnBounds = juce::Rectangle<int>(b.getWidth() - 74, footerY, 52, 20);
}

juce::Point<float> SpeakerViewComponent::getHeadCenter() const {
    const auto b = getLocalBounds().toFloat();
    const float top = kHeaderH;
    const float bottom = b.getHeight() - kFooterH;
    return { b.getWidth() * 0.5f, bottom - (bottom - top) * 0.12f };
}

void SpeakerViewComponent::getSpeakerPositions(juce::Point<float>& spkL, juce::Point<float>& spkR, float& radiusPx) const {
    const auto head = getHeadCenter();
    const float stageH = getHeight() - kHeaderH - kFooterH;
    const float maxR = stageH * 0.86f;

    if (currentField == 0)      radiusPx = maxR * 0.42f; // 1.1 m
    else if (currentField == 1) radiusPx = maxR * 0.66f; // 2.2 m
    else                        radiusPx = maxR * 0.94f; // 4.2 m

    const float halfSpreadRad = (currentSpreadDeg * 0.5f) * kPi / 180.0f;
    spkL = { head.x - radiusPx * std::sin(halfSpreadRad), head.y - radiusPx * std::cos(halfSpreadRad) };
    spkR = { head.x + radiusPx * std::sin(halfSpreadRad), head.y - radiusPx * std::cos(halfSpreadRad) };
}

void SpeakerViewComponent::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto head = getHeadCenter();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // 1. Panel: recessed gradient with a hairline frame and a faint vignette toward the listener
    juce::ColourGradient panel(CustomLookAndFeel::inset.brighter(0.02f), w * 0.5f, 0.0f,
                               CustomLookAndFeel::canvas, w * 0.5f, h, false);
    g.setGradientFill(panel);
    g.fillRoundedRectangle(bounds, 12.0f);
    g.setColour(CustomLookAndFeel::hairline);
    g.drawRoundedRectangle(bounds, 12.0f, 1.0f);

    // 2. Header: field and telemetry
    const char* fieldName = (currentField == 0) ? "NEARFIELD" : (currentField == 1 ? "MIDFIELD" : "FARFIELD");
    const char* distance = (currentField == 0) ? "1.1 m" : (currentField == 1 ? "2.2 m" : "4.2 m");

    g.setColour(CustomLookAndFeel::gold);
    g.fillEllipse(bounds.getX() + 16.0f, bounds.getY() + 14.0f, 6.0f, 6.0f);
    g.setColour(CustomLookAndFeel::textPrimary);
    g.setFont(CustomLookAndFeel::labelFont(9.5f));
    g.drawText(juce::String(fieldName) + "  " + juce::String(distance) + "  " + juce::String(static_cast<int>(std::round(currentSpreadDeg))) + CustomLookAndFeel::degreeSign(),
               bounds.getX() + 28.0f, bounds.getY() + 10.0f, 320.0f, 16.0f, juce::Justification::centredLeft);

    const float drrDb = SpeakerGeometry::getFieldConfig(currentField).drrDb;
    const float rt60 = RoomEngine::getEffectiveRt60(currentProfile, currentField);
    const juce::String telemetry = "DRR " + juce::String(drrDb >= 0.0f ? "+" : "") + juce::String(drrDb, 1) + " dB    RT60 "
                                 + juce::String(rt60, 2) + " s    ROT " + juce::String(static_cast<int>(std::round(currentRotationDeg))) + CustomLookAndFeel::degreeSign();
    g.setColour(CustomLookAndFeel::textSecondary);
    g.setFont(CustomLookAndFeel::bodyFont(10.0f));
    g.drawText(telemetry, bounds.getRight() - 300.0f, bounds.getY() + 10.0f, 284.0f, 16.0f, juce::Justification::centredRight);

    g.setColour(CustomLookAndFeel::hairlineSoft);
    g.drawLine(bounds.getX(), bounds.getY() + kHeaderH, bounds.getRight(), bounds.getY() + kHeaderH, 1.0f);

    // 3. Distance rings: faint, labelled once each on the right
    float radiusPx = 0.0f;
    juce::Point<float> spkL, spkR;
    getSpeakerPositions(spkL, spkR, radiusPx);
    const float unitDistance = radiusPx / ((currentField == 0) ? 1.1f : (currentField == 1 ? 2.2f : 4.2f));
    const float ringSweep = 62.0f * kPi / 180.0f;

    for (int meter = 1; meter <= 4; ++meter) {
        const float r = unitDistance * static_cast<float>(meter);
        if (r < 24.0f || head.y - r < kHeaderH + 6.0f) continue;

        juce::Path ring;
        ring.addCentredArc(head.x, head.y, r, r, 0.0f, -ringSweep, ringSweep, true);
        g.setColour(CustomLookAndFeel::hairline.withAlpha(0.8f));
        g.strokePath(ring, juce::PathStrokeType(1.0f));

        const float labelAngle = 58.0f * kPi / 180.0f;
        const auto p = juce::Point<float>(head.x + r * std::sin(labelAngle), head.y - r * std::cos(labelAngle));
        g.setColour(CustomLookAndFeel::textTertiary);
        g.setFont(CustomLookAndFeel::bodyFont(8.5f));
        g.drawText(juce::String(meter) + " m", juce::Rectangle<float>(34.0f, 12.0f).withCentre(p), juce::Justification::centred);
    }

    // 4. Direct paths from both monitors to the listener
    const float dash[] = { 3.0f, 5.0f };
    g.setColour(CustomLookAndFeel::teal.withAlpha(0.35f));
    g.drawDashedLine(juce::Line<float>(spkL, head), dash, 2, 1.0f);
    g.drawDashedLine(juce::Line<float>(spkR, head), dash, 2, 1.0f);

    // 5. Active distance arc with a soft gold glow
    const float speakerHalfSweep = (currentSpreadDeg * 0.5f) * kPi / 180.0f;
    juce::Path activeArc;
    activeArc.addCentredArc(head.x, head.y, radiusPx, radiusPx, 0.0f, -speakerHalfSweep, speakerHalfSweep, true);
    g.setColour(CustomLookAndFeel::gold.withAlpha(0.14f));
    g.strokePath(activeArc, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(CustomLookAndFeel::gold.withAlpha(0.7f));
    g.strokePath(activeArc, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Spread angle at the listener
    const float halfSpr = (currentSpreadDeg * 0.5f) * kPi / 180.0f;
    const float spreadR = 46.0f;
    juce::Path spreadArc;
    spreadArc.addCentredArc(head.x, head.y, spreadR, spreadR, 0.0f, -halfSpr, halfSpr, true);
    g.setColour(CustomLookAndFeel::goldBright.withAlpha(0.55f));
    g.strokePath(spreadArc, juce::PathStrokeType(1.0f));

    // 6. Monitors: slim cabinets with a gold-ringed tweeter, toed in toward the listener
    auto drawMonitor = [&](juce::Point<float> pos, float toeInDeg, const juce::String& label) {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(toeInDeg * kPi / 180.0f, pos.x, pos.y));

        const auto cab = juce::Rectangle<float>(24.0f, 38.0f).withCentre(pos);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(cab.translated(0.0f, 3.0f), 5.0f);

        juce::ColourGradient face(CustomLookAndFeel::surfaceRaised.brighter(0.06f), cab.getX(), cab.getY(),
                                  CustomLookAndFeel::surface.darker(0.2f), cab.getX(), cab.getBottom(), false);
        g.setGradientFill(face);
        g.fillRoundedRectangle(cab, 5.0f);
        g.setColour(CustomLookAndFeel::hairline);
        g.drawRoundedRectangle(cab, 5.0f, 1.0f);

        g.setColour(CustomLookAndFeel::inset);
        g.fillEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre({ pos.x, pos.y - 8.0f }));
        g.setColour(CustomLookAndFeel::gold.withAlpha(0.8f));
        g.drawEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre({ pos.x, pos.y - 8.0f }), 1.0f);
        g.setColour(CustomLookAndFeel::gold);
        g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre({ pos.x, pos.y - 8.0f }));

        g.setColour(CustomLookAndFeel::teal);
        g.fillEllipse(juce::Rectangle<float>(4.0f, 4.0f).withCentre({ pos.x, pos.y + 13.0f }));

        g.restoreState();

        g.setColour(CustomLookAndFeel::textSecondary);
        g.setFont(CustomLookAndFeel::labelFont(8.0f));
        g.drawText(label, juce::Rectangle<float>(60.0f, 12.0f).withCentre({ pos.x, pos.y - 34.0f }), juce::Justification::centred);
    };

    drawMonitor(spkL, currentSpreadDeg * 0.5f, "L");
    drawMonitor(spkR, -currentSpreadDeg * 0.5f, "R");

    // 7. Listener: a clean head with ear cups, nose tick shows the orientation
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(currentRotationDeg * kPi / 180.0f, head.x, head.y));

    g.setColour(CustomLookAndFeel::teal.withAlpha(0.10f));
    g.fillEllipse(juce::Rectangle<float>(52.0f, 52.0f).withCentre(head));
    g.setColour(CustomLookAndFeel::surfaceRaised);
    g.fillEllipse(juce::Rectangle<float>(28.0f, 28.0f).withCentre(head));
    g.setColour(CustomLookAndFeel::teal);
    g.drawEllipse(juce::Rectangle<float>(28.0f, 28.0f).withCentre(head), 1.2f);

    juce::Path nose;
    nose.startNewSubPath(head.x, head.y - 22.0f);
    nose.lineTo(head.x - 3.5f, head.y - 15.0f);
    nose.lineTo(head.x + 3.5f, head.y - 15.0f);
    nose.closeSubPath();
    g.setColour(CustomLookAndFeel::teal);
    g.fillPath(nose);

    g.setColour(CustomLookAndFeel::gold);
    g.fillRoundedRectangle(juce::Rectangle<float>(5.0f, 11.0f).withCentre({ head.x - 16.0f, head.y }), 2.0f);
    g.fillRoundedRectangle(juce::Rectangle<float>(5.0f, 11.0f).withCentre({ head.x + 16.0f, head.y }), 2.0f);
    g.restoreState();

    // Spread value chip above the listener
    const auto chip = juce::Rectangle<float>(48.0f, 16.0f).withCentre({ head.x, head.y - spreadR - 10.0f });
    g.setColour(CustomLookAndFeel::surfaceRaised);
    g.fillRoundedRectangle(chip, 8.0f);
    g.setColour(CustomLookAndFeel::hairline);
    g.drawRoundedRectangle(chip, 8.0f, 1.0f);
    g.setColour(CustomLookAndFeel::goldBright);
    g.setFont(CustomLookAndFeel::labelFont(8.5f));
    g.drawText(juce::String(static_cast<int>(std::round(currentSpreadDeg))) + CustomLookAndFeel::degreeSign(), chip, juce::Justification::centred);

    // 8. Footer: listener reset and spread presets
    const float footerY = bounds.getBottom() - kFooterH;
    g.setColour(CustomLookAndFeel::hairlineSoft);
    g.drawLine(bounds.getX(), footerY, bounds.getRight(), footerY, 1.0f);

    g.setColour(CustomLookAndFeel::textTertiary);
    g.setFont(CustomLookAndFeel::labelFont(8.0f));
    g.drawText("LISTENER", bounds.getX() + 16.0f, footerY + 12.0f, 70.0f, 16.0f, juce::Justification::centredLeft);
    g.drawText("SPREAD", spread45BtnBounds.getX() - 62.0f, footerY + 12.0f, 56.0f, 16.0f, juce::Justification::centredRight);

    auto drawPill = [&](const juce::Rectangle<int>& r, const juce::String& text, bool active) {
        const auto f = r.toFloat();
        g.setColour(active ? CustomLookAndFeel::gold.withAlpha(0.14f) : CustomLookAndFeel::inset);
        g.fillRoundedRectangle(f, f.getHeight() * 0.5f);
        g.setColour(active ? CustomLookAndFeel::gold.withAlpha(0.7f) : CustomLookAndFeel::hairline);
        g.drawRoundedRectangle(f, f.getHeight() * 0.5f, 1.0f);
        g.setColour(active ? CustomLookAndFeel::goldBright : CustomLookAndFeel::textSecondary);
        g.setFont(CustomLookAndFeel::labelFont(8.0f));
        g.drawText(text, r, juce::Justification::centred);
    };

    drawPill(resetBtnBounds, "RESET 0" + CustomLookAndFeel::degreeSign(), std::abs(currentRotationDeg) < 0.5f);
    drawPill(spread45BtnBounds, "45" + CustomLookAndFeel::degreeSign(), std::abs(currentSpreadDeg - 45.0f) < 1.0f);
    drawPill(spread60BtnBounds, "60" + CustomLookAndFeel::degreeSign() + " ITU", std::abs(currentSpreadDeg - 60.0f) < 1.0f);
    drawPill(spread75BtnBounds, "75" + CustomLookAndFeel::degreeSign(), std::abs(currentSpreadDeg - 75.0f) < 1.0f);
}

void SpeakerViewComponent::mouseDown(const juce::MouseEvent& e) {
    const auto pos = e.position;
    const auto head = getHeadCenter();
    juce::Point<float> spkL, spkR;
    float radiusPx = 0.0f;
    getSpeakerPositions(spkL, spkR, radiusPx);

    if (resetBtnBounds.contains(e.getPosition())) {
        if (onResetRotation) onResetRotation();
        else setHeadRotation(0.0f);
        return;
    }
    if (spread45BtnBounds.contains(e.getPosition())) {
        if (onSpreadChanged) onSpreadChanged(45.0f); else setSpeakerSpread(45.0f);
        return;
    }
    if (spread60BtnBounds.contains(e.getPosition())) {
        if (onSpreadChanged) onSpreadChanged(60.0f); else setSpeakerSpread(60.0f);
        return;
    }
    if (spread75BtnBounds.contains(e.getPosition())) {
        if (onSpreadChanged) onSpreadChanged(75.0f); else setSpeakerSpread(75.0f);
        return;
    }

    if (pos.getDistanceFrom(spkL) < 26.0f) {
        activeDragTarget = DragTarget::LeftSpeaker;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else if (pos.getDistanceFrom(spkR) < 26.0f) {
        activeDragTarget = DragTarget::RightSpeaker;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else if (pos.getDistanceFrom(head) < 28.0f) {
        activeDragTarget = DragTarget::ListenerHead;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    } else {
        activeDragTarget = DragTarget::None;
    }
}

void SpeakerViewComponent::mouseDrag(const juce::MouseEvent& e) {
    const auto pos = e.position;
    const auto head = getHeadCenter();

    if (activeDragTarget == DragTarget::LeftSpeaker || activeDragTarget == DragTarget::RightSpeaker) {
        const float dx = std::abs(pos.x - head.x);
        const float dy = std::max(10.0f, head.y - pos.y);
        const float halfAngleRad = std::atan2(dx, dy);
        const float newSpreadDeg = juce::jlimit(30.0f, 110.0f, (halfAngleRad * 2.0f) * 180.0f / kPi);

        if (onSpreadChanged) onSpreadChanged(newSpreadDeg); else setSpeakerSpread(newSpreadDeg);
    } else if (activeDragTarget == DragTarget::ListenerHead) {
        const float angleRad = std::atan2(pos.x - head.x, head.y - pos.y);
        const float newRotDeg = juce::jlimit(-90.0f, 90.0f, angleRad * 180.0f / kPi);

        if (onRotationChanged) onRotationChanged(newRotDeg); else setHeadRotation(newRotDeg);
    }
}

void SpeakerViewComponent::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    activeDragTarget = DragTarget::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

} // namespace fieldroom
