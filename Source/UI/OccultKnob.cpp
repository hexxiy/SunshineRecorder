#include "OccultKnob.h"
#include "OccultLookAndFeel.h"

namespace palace {

OccultKnob::OccultKnob(const juce::String& labelText) {
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    slider.setBufferedToImage(true);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, OccultLookAndFeel::textDim);
    addAndMakeVisible(label);
}

OccultKnob::~OccultKnob() = default;

void OccultKnob::paint(juce::Graphics& g) {
    if (midiLearnHighlight) {
        // Draw pulsing highlight border for MIDI learn
        float pulse = 0.5f + 0.5f * std::sin(juce::Time::getMillisecondCounterHiRes() / 150.0f);
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.3f + 0.4f * pulse));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 4.0f, 2.0f);
    }

    // Draw modulation ring overlay
    if (modulationActive && std::abs(modulationAmount) > 0.001f) {
        auto sliderBounds = slider.getBounds().toFloat();

        // Calculate knob center and radius (matching the look and feel dimensions)
        float size = std::min(sliderBounds.getWidth(), sliderBounds.getHeight() - 16.0f);
        float radius = size * 0.5f;
        float centreX = sliderBounds.getCentreX();
        float centreY = sliderBounds.getY() + radius + 2.0f;  // Slight offset for visual alignment

        // Outer ring radius (slightly larger than knob)
        float outerRadius = radius + 4.0f;
        float ringThickness = 3.0f;

        // Get current slider position (0-1)
        float sliderPos = static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));

        // Calculate angles (rotary knob typically goes from about 7 o'clock to 5 o'clock)
        const float rotaryStartAngle = juce::MathConstants<float>::pi * 1.2f;
        const float rotaryEndAngle = juce::MathConstants<float>::pi * 2.8f;
        const float angleRange = rotaryEndAngle - rotaryStartAngle;

        // Base angle from slider position
        float baseAngle = rotaryStartAngle + sliderPos * angleRange;

        // Modulated angle
        float modAngle = baseAngle + modulationAmount * angleRange * 0.5f;

        // Clamp modulated angle to valid range
        modAngle = juce::jlimit(rotaryStartAngle, rotaryEndAngle, modAngle);

        // Draw the arc from base position to modulated position
        juce::Path modArc;
        float startAngle = std::min(baseAngle, modAngle);
        float endAngle = std::max(baseAngle, modAngle);

        // Only draw if there's a visible arc
        if (endAngle - startAngle > 0.01f) {
            modArc.addCentredArc(centreX, centreY, outerRadius, outerRadius,
                                 0.0f, startAngle, endAngle, true);

            // Draw glow effect
            g.setColour(OccultLookAndFeel::amber.withAlpha(0.2f));
            g.strokePath(modArc, juce::PathStrokeType(ringThickness + 4.0f));

            // Draw main arc
            g.setColour(OccultLookAndFeel::amber.withAlpha(0.8f));
            g.strokePath(modArc, juce::PathStrokeType(ringThickness));
        }

        // Draw modulated position indicator dot
        float dotX = centreX + std::sin(modAngle) * outerRadius;
        float dotY = centreY - std::cos(modAngle) * outerRadius;
        g.setColour(OccultLookAndFeel::amber);
        g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
    }
}

void OccultKnob::setHighlighted(bool highlighted) {
    if (midiLearnHighlight != highlighted) {
        midiLearnHighlight = highlighted;
        repaint();
    }
}

void OccultKnob::setModulationAmount(float amount) {
    if (std::abs(modulationAmount - amount) > 0.001f) {
        modulationAmount = amount;
        repaint();
    }
}

void OccultKnob::setModulationActive(bool active) {
    if (modulationActive != active) {
        modulationActive = active;
        repaint();
    }
}

void OccultKnob::resized() {
    auto bounds = getLocalBounds();

    const int labelHeight = 18;
    const int labelPadding = 8;  // Extra space between label and knob for modulation ring
    label.setBounds(bounds.removeFromTop(labelHeight));
    bounds.removeFromTop(labelPadding);

    slider.setBounds(bounds);
}

void OccultKnob::setAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId) {
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
}

} // namespace palace
