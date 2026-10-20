#include "SpeakerViewComponent.h"
#include "CustomLookAndFeel.h"
#include "../DSP/SpeakerGeometry.h"
#include <cmath>

namespace fieldroom {

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
    float clamped = juce::jlimit(30.0f, 110.0f, spreadDegrees);
    if (std::abs(currentSpreadDeg - clamped) > 0.1f) {
        currentSpreadDeg = clamped;
        repaint();
    }
}

void SpeakerViewComponent::setHeadRotation(float rotationDegrees) {
    float clamped = juce::jlimit(-90.0f, 90.0f, rotationDegrees);
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
    auto b = getLocalBounds();
    int footerY = b.getBottom() - 26;

    resetBtnBounds = juce::Rectangle<int>(80, footerY + 2, 68, 20);
    spread45BtnBounds = juce::Rectangle<int>(205, footerY + 2, 38, 20);
    spread60BtnBounds = juce::Rectangle<int>(248, footerY + 2, 54, 20);
    spread75BtnBounds = juce::Rectangle<int>(307, footerY + 2, 38, 20);
}

juce::Point<float> SpeakerViewComponent::getHeadCenter() const {
    auto b = getLocalBounds().toFloat();
    return { b.getWidth() * 0.5f, b.getHeight() * 0.77f };
}

void SpeakerViewComponent::getSpeakerPositions(juce::Point<float>& spkL, juce::Point<float>& spkR, float& radiusPx) const {
    auto head = getHeadCenter();
    auto b = getLocalBounds().toFloat();
    float maxH = b.getHeight() * 0.70f;

    // Radius scaling according to selected field
    if (currentField == 0)      radiusPx = maxH * 0.44f; // 1.1 m
    else if (currentField == 1) radiusPx = maxH * 0.68f; // 2.2 m
    else                        radiusPx = maxH * 0.94f; // 4.2 m

    float halfSpreadRad = (currentSpreadDeg * 0.5f) * juce::MathConstants<float>::pi / 180.0f;
    spkL = { head.x - radiusPx * std::sin(halfSpreadRad), head.y - radiusPx * std::cos(halfSpreadRad) };
    spkR = { head.x + radiusPx * std::sin(halfSpreadRad), head.y - radiusPx * std::cos(halfSpreadRad) };
}

void SpeakerViewComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    auto head = getHeadCenter();

    // 1. Dark Acoustic Chamber Shell Background
    juce::ColourGradient bgGrad(CustomLookAndFeel::panelMilled.darker(0.3f), w * 0.5f, 0.0f,
                               CustomLookAndFeel::bgObsidian.darker(0.2f), w * 0.5f, h, false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Milled Outer Seam
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // 2. Stage Header Telemetry Banner
    g.setColour(CustomLookAndFeel::panelSurface.withAlpha(0.7f));
    g.fillRect(bounds.getX() + 1.0f, bounds.getY() + 1.0f, bounds.getWidth() - 2.0f, 26.0f);
    g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.6f));
    g.drawLine(bounds.getX(), bounds.getY() + 27.0f, bounds.getRight(), bounds.getY() + 27.0f, 1.0f);

    // Status LED dot + Field Name
    g.setColour(CustomLookAndFeel::accentGold);
    g.fillEllipse(bounds.getX() + 12.0f, bounds.getY() + 10.0f, 6.0f, 6.0f);
    g.setColour(CustomLookAndFeel::accentGold.withAlpha(0.3f));
    g.fillEllipse(bounds.getX() + 10.0f, bounds.getY() + 8.0f, 10.0f, 10.0f);

    juce::String fieldName = (currentField == 0) ? "NEARFIELD (1.1m / " : (currentField == 1 ? "MIDFIELD (2.2m / " : "FARFIELD (4.2m / ");
    fieldName += juce::String(static_cast<int>(std::round(currentSpreadDeg))) + "\xc2\xb0)";

    g.setColour(CustomLookAndFeel::accentGoldBright);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("GEOMETRY: " + fieldName, bounds.getX() + 24.0f, bounds.getY() + 6.0f, 240.0f, 16.0f, juce::Justification::centredLeft);

    // Telemetry stats on right
    juce::String drrStr = (currentField == 0) ? "+14.0 dB" : (currentField == 1 ? "+7.0 dB" : "+1.5 dB");
    juce::String rt60Str = (currentProfile == 0) ? "0.20s" : (currentProfile == 1 ? "0.28s" : (currentProfile == 2 ? "0.38s" : (currentProfile == 3 ? "0.55s" : "0.45s")));
    juce::String teleStr = "DRR: " + drrStr + "   RT60: " + rt60Str + "   ROT: " + juce::String(static_cast<int>(currentRotationDeg)) + "\xc2\xb0";

    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.drawText(teleStr, bounds.getRight() - 250.0f, bounds.getY() + 6.0f, 238.0f, 16.0f, juce::Justification::centredRight);

    // 3. Studio Acoustic Treatment Diffusers & Bass Traps along perimeter
    auto roomWall = bounds.reduced(14.0f, 32.0f);
    g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.35f));
    g.drawRoundedRectangle(roomWall, 4.0f, 1.0f);

    // Wall Acoustic Absorbers / Diffuser Slats (Left & Right Walls)
    for (int i = 0; i < 7; ++i) {
        float slatY = roomWall.getY() + 20.0f + static_cast<float>(i) * 26.0f;

        g.setColour(CustomLookAndFeel::panelRaised);
        g.fillRoundedRectangle(roomWall.getX() - 6.0f, slatY, 6.0f, 18.0f, 1.5f);
        g.setColour(CustomLookAndFeel::metalBorder);
        g.drawRoundedRectangle(roomWall.getX() - 6.0f, slatY, 6.0f, 18.0f, 1.5f, 0.8f);

        g.setColour(CustomLookAndFeel::panelRaised);
        g.fillRoundedRectangle(roomWall.getRight(), slatY, 6.0f, 18.0f, 1.5f);
        g.setColour(CustomLookAndFeel::metalBorder);
        g.drawRoundedRectangle(roomWall.getRight(), slatY, 6.0f, 18.0f, 1.5f, 0.8f);
    }

    // Front Wall Slats
    for (int i = 0; i < 9; ++i) {
        float slatX = roomWall.getCentreX() - 140.0f + static_cast<float>(i) * 35.0f;
        g.setColour(CustomLookAndFeel::panelRaised);
        g.fillRoundedRectangle(slatX, roomWall.getY() - 6.0f, 22.0f, 6.0f, 1.5f);
        g.setColour(CustomLookAndFeel::metalBorder);
        g.drawRoundedRectangle(slatX, roomWall.getY() - 6.0f, 22.0f, 6.0f, 1.5f, 0.8f);
    }

    // Corner Bass Traps (Angled 45 deg chamfers)
    auto drawCornerTrap = [&](float cx, float cy, float dx, float dy) {
        juce::Path p;
        p.startNewSubPath(cx, cy);
        p.lineTo(cx + dx, cy);
        p.lineTo(cx, cy + dy);
        p.closeSubPath();
        g.setColour(CustomLookAndFeel::panelSurface.darker(0.2f));
        g.fillPath(p);
        g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.6f));
        g.strokePath(p, juce::PathStrokeType(1.0f));
    };
    drawCornerTrap(roomWall.getX(), roomWall.getY(), 22.0f, 22.0f);
    drawCornerTrap(roomWall.getRight(), roomWall.getY(), -22.0f, 22.0f);

    // 4. Concentric Polar Distance Arcs & Distance Badges
    float radiusPx = 0.0f;
    juce::Point<float> spkL, spkR;
    getSpeakerPositions(spkL, spkR, radiusPx);

    float unitDistance = (currentField == 0) ? (radiusPx / 1.1f) : (currentField == 1 ? (radiusPx / 2.2f) : (radiusPx / 4.2f));
    float arcSweep = 75.0f * juce::MathConstants<float>::pi / 180.0f;

    for (int meter = 1; meter <= 4; ++meter) {
        float r = unitDistance * static_cast<float>(meter);
        if (r < 20.0f || r > h * 0.95f) continue;

        juce::Path arcPath;
        arcPath.addCentredArc(head.x, head.y, r, r, 0.0f, -arcSweep, arcSweep, true);

        g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.35f));
        g.strokePath(arcPath, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float labelAngle = 55.0f * juce::MathConstants<float>::pi / 180.0f;
        float lx = head.x + r * std::sin(labelAngle);
        float ly = head.y - r * std::cos(labelAngle);
        g.setColour(CustomLookAndFeel::textDim);
        g.setFont(juce::Font(9.0f, juce::Font::plain));
        g.drawText(juce::String(meter) + ".0 m", lx - 14.0f, ly - 6.0f, 28.0f, 12.0f, juce::Justification::centred);
    }

    // 5. Active Distance Arc with Luminous Golden Glow
    juce::Path activeArc;
    activeArc.addCentredArc(head.x, head.y, radiusPx, radiusPx, 0.0f, -arcSweep * 0.95f, arcSweep * 0.95f, true);

    g.setColour(CustomLookAndFeel::accentGold.withAlpha(0.20f));
    g.strokePath(activeArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(CustomLookAndFeel::accentGold);
    g.strokePath(activeArc, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::String distText = (currentField == 0) ? "1.1 METERS" : (currentField == 1 ? "2.2 METERS" : "4.2 METERS");
    float badgeY = head.y - radiusPx - 8.0f;
    g.setColour(CustomLookAndFeel::panelSurface);
    g.fillRoundedRectangle(head.x - 38.0f, badgeY, 76.0f, 16.0f, 3.0f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(head.x - 38.0f, badgeY, 76.0f, 16.0f, 3.0f, 0.8f);
    g.setColour(CustomLookAndFeel::accentGoldBright);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText(distText, head.x - 38.0f, badgeY, 76.0f, 16.0f, juce::Justification::centred);

    // 6. Direct Sound Acoustic Ray Beams (Left & Right)
    float beamDash[] = { 4.0f, 4.0f };
    g.setColour(CustomLookAndFeel::accentCyan.withAlpha(0.55f));
    g.drawDashedLine(juce::Line<float>(spkL.x, spkL.y, head.x - 8.0f, head.y), beamDash, 2, 1.4f);
    g.drawDashedLine(juce::Line<float>(spkR.x, spkR.y, head.x + 8.0f, head.y), beamDash, 2, 1.4f);

    // Opening angle arc & badge near listener head
    float arcAngleR = 48.0f;
    float halfSpr = (currentSpreadDeg * 0.5f) * juce::MathConstants<float>::pi / 180.0f;
    juce::Path angleArc;
    angleArc.addCentredArc(head.x, head.y, arcAngleR, arcAngleR, 0.0f, -halfSpr, halfSpr, true);
    g.setColour(CustomLookAndFeel::accentGold.withAlpha(0.6f));
    g.strokePath(angleArc, juce::PathStrokeType(1.0f));

    g.setColour(CustomLookAndFeel::panelMilled);
    g.fillRoundedRectangle(head.x - 16.0f, head.y - arcAngleR - 14.0f, 32.0f, 14.0f, 2.5f);
    g.setColour(CustomLookAndFeel::metalBorder);
    g.drawRoundedRectangle(head.x - 16.0f, head.y - arcAngleR - 14.0f, 32.0f, 14.0f, 2.5f, 0.8f);
    g.setColour(CustomLookAndFeel::accentGoldBright);
    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.drawText(juce::String(static_cast<int>(std::round(currentSpreadDeg))) + "\xc2\xb0",
               head.x - 16.0f, head.y - arcAngleR - 14.0f, 32.0f, 14.0f, juce::Justification::centred);

    // 7. Virtual Studio Monitor Loudspeakers
    auto drawMonitor = [&](juce::Point<float> pos, float toeInDeg, const juce::String& label) {
        g.saveState();
        g.addTransform(juce::AffineTransform::rotation(toeInDeg * juce::MathConstants<float>::pi / 180.0f, pos.x, pos.y));

        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(pos.x - 16.0f, pos.y - 23.0f, 32.0f, 46.0f, 3.0f);

        juce::ColourGradient cabGrad(CustomLookAndFeel::panelRaised.brighter(0.1f), pos.x - 15.0f, pos.y - 22.0f,
                                    CustomLookAndFeel::panelMilled.darker(0.3f), pos.x + 15.0f, pos.y + 22.0f, false);
        g.setGradientFill(cabGrad);
        g.fillRoundedRectangle(pos.x - 15.0f, pos.y - 22.0f, 30.0f, 44.0f, 2.5f);

        g.setColour(CustomLookAndFeel::metalBorderLight);
        g.drawRoundedRectangle(pos.x - 15.0f, pos.y - 22.0f, 30.0f, 44.0f, 2.5f, 1.0f);

        g.setColour(juce::Colour(0xff121517));
        g.fillRoundedRectangle(pos.x - 13.0f, pos.y - 20.0f, 26.0f, 40.0f, 1.5f);

        g.setColour(juce::Colour(0xff1e2225));
        g.fillEllipse(pos.x - 9.0f, pos.y - 14.0f, 18.0f, 18.0f);
        g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.6f));
        g.drawEllipse(pos.x - 9.0f, pos.y - 14.0f, 18.0f, 18.0f, 0.8f);
        g.setColour(juce::Colour(0xff101315));
        g.fillEllipse(pos.x - 4.0f, pos.y - 9.0f, 8.0f, 8.0f);

        g.setColour(CustomLookAndFeel::accentGoldDark);
        g.fillEllipse(pos.x - 5.0f, pos.y + 7.0f, 10.0f, 10.0f);
        g.setColour(CustomLookAndFeel::accentGoldBright);
        g.drawEllipse(pos.x - 5.0f, pos.y + 7.0f, 10.0f, 10.0f, 0.8f);

        g.setColour(CustomLookAndFeel::ledGreen);
        g.fillEllipse(pos.x - 1.5f, pos.y + 16.0f, 3.0f, 3.0f);
        g.setColour(CustomLookAndFeel::ledGreen.withAlpha(0.3f));
        g.fillEllipse(pos.x - 2.5f, pos.y + 15.0f, 5.0f, 5.0f);

        g.restoreState();

        g.setColour(CustomLookAndFeel::textBright);
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText(label, pos.x - 24.0f, pos.y - 34.0f, 48.0f, 12.0f, juce::Justification::centred);
    };

    float toeInL = (currentSpreadDeg * 0.5f);
    float toeInR = -(currentSpreadDeg * 0.5f);
    drawMonitor(spkL, toeInL, "SPK L");
    drawMonitor(spkR, toeInR, "SPK R");

    // 8. Listener Binaural Head & Reference Headphones
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(currentRotationDeg * juce::MathConstants<float>::pi / 180.0f, head.x, head.y));

    g.setColour(CustomLookAndFeel::accentCyan.withAlpha(0.12f));
    g.fillEllipse(head.x - 22.0f, head.y - 22.0f, 44.0f, 44.0f);

    g.setColour(juce::Colour(0xff1e2629));
    g.fillEllipse(head.x - 13.0f, head.y - 13.0f, 26.0f, 26.0f);
    g.setColour(CustomLookAndFeel::accentCyan);
    g.drawEllipse(head.x - 13.0f, head.y - 13.0f, 26.0f, 26.0f, 1.2f);

    juce::Path noseArrow;
    noseArrow.startNewSubPath(head.x, head.y - 19.0f);
    noseArrow.lineTo(head.x - 3.5f, head.y - 14.0f);
    noseArrow.lineTo(head.x + 3.5f, head.y - 14.0f);
    noseArrow.closeSubPath();
    g.setColour(CustomLookAndFeel::accentCyan);
    g.fillPath(noseArrow);

    g.setColour(juce::Colour(0xff121618));
    g.fillRoundedRectangle(head.x - 18.0f, head.y - 6.0f, 5.0f, 12.0f, 1.5f);
    g.setColour(CustomLookAndFeel::accentGold);
    g.drawRoundedRectangle(head.x - 18.0f, head.y - 6.0f, 5.0f, 12.0f, 1.5f, 0.8f);

    g.setColour(juce::Colour(0xff121618));
    g.fillRoundedRectangle(head.x + 13.0f, head.y - 6.0f, 5.0f, 12.0f, 1.5f);
    g.setColour(CustomLookAndFeel::accentGold);
    g.drawRoundedRectangle(head.x + 13.0f, head.y - 6.0f, 5.0f, 12.0f, 1.5f, 0.8f);

    juce::Path headband;
    headband.addCentredArc(head.x, head.y, 16.0f, 16.0f, 0.0f, -juce::MathConstants<float>::pi * 0.45f, juce::MathConstants<float>::pi * 0.45f, true);
    g.setColour(CustomLookAndFeel::metalBorderLight);
    g.strokePath(headband, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(CustomLookAndFeel::accentCyan);
    g.fillEllipse(head.x - 2.0f, head.y - 2.0f, 4.0f, 4.0f);

    g.restoreState();

    // 9. Interactive Stage Footer Toolbar
    float footerY = bounds.getBottom() - 26.0f;
    g.setColour(CustomLookAndFeel::panelSurface.withAlpha(0.85f));
    g.fillRect(bounds.getX() + 1.0f, footerY, bounds.getWidth() - 2.0f, 25.0f);
    g.setColour(CustomLookAndFeel::metalBorder.withAlpha(0.5f));
    g.drawLine(bounds.getX(), footerY, bounds.getRight(), footerY, 1.0f);

    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("LISTENER:", bounds.getX() + 14.0f, footerY + 4.0f, 62.0f, 16.0f, juce::Justification::centredLeft);

    auto drawSubButton = [&](const juce::Rectangle<int>& r, const juce::String& text, bool active) {
        g.setColour(active ? CustomLookAndFeel::panelRaised.brighter(0.1f) : CustomLookAndFeel::panelMilled);
        g.fillRoundedRectangle(r.toFloat(), 2.5f);
        g.setColour(active ? CustomLookAndFeel::accentGold : CustomLookAndFeel::metalBorder);
        g.drawRoundedRectangle(r.toFloat(), 2.5f, active ? 1.2f : 0.8f);
        g.setColour(active ? CustomLookAndFeel::accentGoldBright : CustomLookAndFeel::textMuted);
        g.setFont(juce::Font(9.0f, active ? juce::Font::bold : juce::Font::plain));
        g.drawText(text, r, juce::Justification::centred);
    };

    drawSubButton(resetBtnBounds, "RESET 0\xc2\xb0", std::abs(currentRotationDeg) < 0.5f);

    g.setColour(CustomLookAndFeel::textMuted);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("SPREAD:", spread45BtnBounds.getX() - 52, footerY + 4.0f, 48, 16, juce::Justification::centredRight);

    drawSubButton(spread45BtnBounds, "45\xc2\xb0", std::abs(currentSpreadDeg - 45.0f) < 1.0f);
    drawSubButton(spread60BtnBounds, "60\xc2\xb0 ITU", std::abs(currentSpreadDeg - 60.0f) < 1.0f);
    drawSubButton(spread75BtnBounds, "75\xc2\xb0", std::abs(currentSpreadDeg - 75.0f) < 1.0f);

    g.setColour(CustomLookAndFeel::textDim);
    g.setFont(juce::Font(9.0f, juce::Font::plain));
    g.drawText("DRAG MONITORS / HEAD", bounds.getRight() - 170.0f, footerY + 4.0f, 158.0f, 16.0f, juce::Justification::centredRight);
}

void SpeakerViewComponent::mouseDown(const juce::MouseEvent& e) {
    auto pos = e.position;
    auto head = getHeadCenter();
    float radiusPx = 0.0f;
    juce::Point<float> spkL, spkR;
    getSpeakerPositions(spkL, spkR, radiusPx);

    if (resetBtnBounds.contains(e.getPosition())) {
        if (onResetRotation) onResetRotation();
        else setHeadRotation(0.0f);
        return;
    }
    if (spread45BtnBounds.contains(e.getPosition())) {
        if (onSpreadChanged) onSpreadChanged(45.0f);
        else setSpeakerSpread(45.0f);
        return;
    }
    if (spread60BtnBounds.contains(e.getPosition())) {
        if (onSpreadChanged) onSpreadChanged(60.0f);
        else setSpeakerSpread(60.0f);
        return;
    }
    if (spread75BtnBounds.contains(e.getPosition())) {
        if (onSpreadChanged) onSpreadChanged(75.0f);
        else setSpeakerSpread(75.0f);
        return;
    }

    if (pos.getDistanceFrom(spkL) < 24.0f) {
        activeDragTarget = DragTarget::LeftSpeaker;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else if (pos.getDistanceFrom(spkR) < 24.0f) {
        activeDragTarget = DragTarget::RightSpeaker;
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else if (pos.getDistanceFrom(head) < 26.0f) {
        activeDragTarget = DragTarget::ListenerHead;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    } else {
        activeDragTarget = DragTarget::None;
    }
}

void SpeakerViewComponent::mouseDrag(const juce::MouseEvent& e) {
    auto pos = e.position;
    auto head = getHeadCenter();

    if (activeDragTarget == DragTarget::LeftSpeaker || activeDragTarget == DragTarget::RightSpeaker) {
        float dx = std::abs(pos.x - head.x);
        float dy = std::max(10.0f, head.y - pos.y);
        float halfAngleRad = std::atan2(dx, dy);
        float newSpreadDeg = juce::jlimit(30.0f, 110.0f, (halfAngleRad * 2.0f) * 180.0f / juce::MathConstants<float>::pi);

        if (onSpreadChanged) onSpreadChanged(newSpreadDeg);
        else setSpeakerSpread(newSpreadDeg);
    } else if (activeDragTarget == DragTarget::ListenerHead) {
        float dx = pos.x - head.x;
        float dy = head.y - pos.y;
        float angleRad = std::atan2(dx, dy);
        float newRotDeg = juce::jlimit(-90.0f, 90.0f, angleRad * 180.0f / juce::MathConstants<float>::pi);

        if (onRotationChanged) onRotationChanged(newRotDeg);
        else setHeadRotation(newRotDeg);
    }
}

void SpeakerViewComponent::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    activeDragTarget = DragTarget::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

} // namespace fieldroom
