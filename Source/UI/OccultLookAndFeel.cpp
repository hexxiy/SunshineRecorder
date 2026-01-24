#include "OccultLookAndFeel.h"

namespace palace {

// Color definitions - Dark industrial with amber accents
const juce::Colour OccultLookAndFeel::backgroundDark{0xff0a0a0c};
const juce::Colour OccultLookAndFeel::backgroundMid{0xff151518};
const juce::Colour OccultLookAndFeel::panelDark{0xff1a1a1e};
const juce::Colour OccultLookAndFeel::metalLight{0xff3a3a40};
const juce::Colour OccultLookAndFeel::metalDark{0xff252528};
const juce::Colour OccultLookAndFeel::amber{0xffff9d00};
const juce::Colour OccultLookAndFeel::amberDim{0xff8b5500};
const juce::Colour OccultLookAndFeel::amberGlow{0x40ff9d00};
const juce::Colour OccultLookAndFeel::textLight{0xffc0c0c0};
const juce::Colour OccultLookAndFeel::textDim{0xff707070};
const juce::Colour OccultLookAndFeel::accent{0xff2a6b9e};

OccultLookAndFeel::OccultLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, backgroundDark);
    setColour(juce::Label::textColourId, textLight);
    setColour(juce::Slider::textBoxTextColourId, amber);
    setColour(juce::Slider::textBoxBackgroundColourId, panelDark);
    setColour(juce::Slider::textBoxOutlineColourId, metalDark);
    setColour(juce::TextButton::buttonColourId, panelDark);
    setColour(juce::TextButton::textColourOnId, amber);
    setColour(juce::TextButton::textColourOffId, textLight);
}

void OccultLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPos,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider) {
    const float radius = static_cast<float>(juce::jmin(width, height)) * 0.4f;
    const float centerX = static_cast<float>(x + width) * 0.5f;
    const float centerY = static_cast<float>(y + height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Draw outer ring shadow
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillEllipse(centerX - radius - 2, centerY - radius - 2, radius * 2 + 4, radius * 2 + 4);

    // Draw metal base
    juce::ColourGradient metalGradient(metalLight, centerX, centerY - radius,
                                       metalDark, centerX, centerY + radius, false);
    g.setGradientFill(metalGradient);
    g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);

    // Draw inner darker area
    const float innerRadius = radius * 0.75f;
    g.setColour(panelDark);
    g.fillEllipse(centerX - innerRadius, centerY - innerRadius, innerRadius * 2, innerRadius * 2);

    // Draw sigil decoration
    drawKnobSigil(g, centerX, centerY, innerRadius * 0.8f);

    // Draw value arc (amber glow)
    const float arcRadius = radius * 0.85f;
    juce::Path arcPath;
    arcPath.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                          0.0f, rotaryStartAngle, angle, true);
    g.setColour(amberDim);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f));

    // Draw glowing arc overlay
    g.setColour(amber);
    g.strokePath(arcPath, juce::PathStrokeType(1.5f));

    // Draw pointer
    const float pointerLength = radius * 0.6f;
    const float pointerThickness = 2.0f;
    juce::Path pointer;
    pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centerX, centerY));

    g.setColour(amber);
    g.fillPath(pointer);

    // Draw center cap
    const float capRadius = radius * 0.15f;
    g.setColour(metalDark);
    g.fillEllipse(centerX - capRadius, centerY - capRadius, capRadius * 2, capRadius * 2);
    g.setColour(metalLight.brighter(0.2f));
    g.drawEllipse(centerX - capRadius, centerY - capRadius, capRadius * 2, capRadius * 2, 1.0f);

    // Draw ambient glow when value is high
    if (sliderPos > 0.5f) {
        const float glowAlpha = (sliderPos - 0.5f) * 0.3f;
        g.setColour(amber.withAlpha(glowAlpha));
        g.fillEllipse(centerX - radius * 1.2f, centerY - radius * 1.2f,
                      radius * 2.4f, radius * 2.4f);
    }
}

void OccultLookAndFeel::drawKnobSigil(juce::Graphics& g, float centerX, float centerY, float radius) {
    g.setColour(metalDark.brighter(0.1f));

    // Draw mystical triangle
    juce::Path triangle;
    for (int i = 0; i < 3; ++i) {
        const float angle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 3.0f - juce::MathConstants<float>::halfPi;
        const float px = centerX + std::cos(angle) * radius * 0.6f;
        const float py = centerY + std::sin(angle) * radius * 0.6f;
        if (i == 0)
            triangle.startNewSubPath(px, py);
        else
            triangle.lineTo(px, py);
    }
    triangle.closeSubPath();
    g.strokePath(triangle, juce::PathStrokeType(0.5f));

    // Draw inner circle
    g.drawEllipse(centerX - radius * 0.3f, centerY - radius * 0.3f,
                  radius * 0.6f, radius * 0.6f, 0.5f);
}

void OccultLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    g.fillAll(label.findColour(juce::Label::backgroundColourId));

    if (!label.isBeingEdited()) {
        const juce::Font font = getLabelFont(label);
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(font);

        const juce::Rectangle<int> textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());

        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                         juce::jmax(1, static_cast<int>(textArea.getHeight() / font.getHeight())),
                         label.getMinimumHorizontalScale());
    }
}

void OccultLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown) {
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    juce::Colour baseColour = backgroundColour;

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.brighter(0.1f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.05f);

    // Draw background
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Draw border
    g.setColour(metalLight);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // Draw amber glow on toggle buttons when on
    if (button.getToggleState()) {
        g.setColour(amber.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 2.0f);
    }
}

juce::Font OccultLookAndFeel::getLabelFont(juce::Label& label) {
    return juce::Font(juce::FontOptions(12.0f));
}

void OccultLookAndFeel::drawMetalTexture(juce::Graphics& g, juce::Rectangle<float> bounds) {
    // Simple aged metal texture effect
    juce::Random random;
    g.setColour(juce::Colours::white.withAlpha(0.02f));

    for (int i = 0; i < 50; ++i) {
        const float x = bounds.getX() + random.nextFloat() * bounds.getWidth();
        const float y = bounds.getY() + random.nextFloat() * bounds.getHeight();
        g.fillEllipse(x, y, 1.0f, 1.0f);
    }
}

} // namespace palace
