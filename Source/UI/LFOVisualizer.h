#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "OccultLookAndFeel.h"

namespace palace {

class LFOVisualizer : public juce::Component {
public:
    LFOVisualizer() = default;
    ~LFOVisualizer() override = default;

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);

        // Draw background
        g.setColour(OccultLookAndFeel::backgroundDark);
        g.fillRoundedRectangle(bounds, 4.0f);

        // Draw border
        g.setColour(OccultLookAndFeel::metalDark);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        // Draw waveform
        const float padding = 6.0f;
        auto waveArea = bounds.reduced(padding);

        // Draw center line
        g.setColour(OccultLookAndFeel::metalDark.withAlpha(0.5f));
        g.drawHorizontalLine(static_cast<int>(waveArea.getCentreY()),
                            waveArea.getX(), waveArea.getRight());

        // Draw waveform path
        juce::Path wavePath;
        const int numPoints = static_cast<int>(waveArea.getWidth());

        for (int i = 0; i <= numPoints; ++i) {
            float x = waveArea.getX() + static_cast<float>(i);
            float normalizedX = static_cast<float>(i) / static_cast<float>(numPoints);
            float value = getWaveformValue(normalizedX);
            float y = waveArea.getCentreY() - value * (waveArea.getHeight() * 0.4f);

            if (i == 0)
                wavePath.startNewSubPath(x, y);
            else
                wavePath.lineTo(x, y);
        }

        g.setColour(OccultLookAndFeel::amber.withAlpha(0.7f));
        g.strokePath(wavePath, juce::PathStrokeType(1.5f));

        // Draw phase indicator (vertical line)
        float phaseX = waveArea.getX() + phase * waveArea.getWidth();
        float phaseValue = getWaveformValue(phase);
        float phaseY = waveArea.getCentreY() - phaseValue * (waveArea.getHeight() * 0.4f);

        // Vertical line at phase position
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.3f));
        g.drawVerticalLine(static_cast<int>(phaseX), waveArea.getY(), waveArea.getBottom());

        // Draw phase dot
        g.setColour(OccultLookAndFeel::amber);
        g.fillEllipse(phaseX - 4.0f, phaseY - 4.0f, 8.0f, 8.0f);

        // Glow effect on dot
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.3f));
        g.fillEllipse(phaseX - 6.0f, phaseY - 6.0f, 12.0f, 12.0f);
    }

    void setWaveform(int waveformIndex) {
        waveform = waveformIndex;
        repaint();
    }

    void setPhase(float newPhase) {
        phase = newPhase;
        repaint();
    }

    void setLfoValue(float value) {
        currentValue = value;
    }

private:
    float getWaveformValue(float normalizedPhase) const {
        switch (waveform) {
            case 0: // Sine
                return std::sin(normalizedPhase * juce::MathConstants<float>::twoPi);
            case 1: // Triangle
            {
                float t = normalizedPhase * 4.0f;
                if (t < 1.0f) return t;
                if (t < 3.0f) return 2.0f - t;
                return t - 4.0f;
            }
            case 2: // Square
                return normalizedPhase < 0.5f ? 1.0f : -1.0f;
            case 3: // Noise - use hash for consistent random look
            {
                int seed = static_cast<int>(normalizedPhase * 1000.0f);
                return (static_cast<float>((seed * 1103515245 + 12345) & 0x7fffffff) / 0x7fffffff) * 2.0f - 1.0f;
            }
            case 4: // Stepped Noise (S&H) - 8 steps per cycle
            {
                int step = static_cast<int>(normalizedPhase * 8.0f);
                int seed = step * 12345;
                return (static_cast<float>((seed * 1103515245 + 12345) & 0x7fffffff) / 0x7fffffff) * 2.0f - 1.0f;
            }
            default:
                return 0.0f;
        }
    }

    int waveform = 0;      // 0=sine, 1=triangle, 2=square, 3=noise, 4=S&H
    float phase = 0.0f;    // 0.0 to 1.0
    float currentValue = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFOVisualizer)
};

} // namespace palace
