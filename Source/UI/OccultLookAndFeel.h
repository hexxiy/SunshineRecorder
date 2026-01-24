#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace palace {

class OccultLookAndFeel : public juce::LookAndFeel_V4 {
public:
    OccultLookAndFeel();

    // Colors
    static const juce::Colour backgroundDark;
    static const juce::Colour backgroundMid;
    static const juce::Colour panelDark;
    static const juce::Colour metalLight;
    static const juce::Colour metalDark;
    static const juce::Colour amber;
    static const juce::Colour amberDim;
    static const juce::Colour amberGlow;
    static const juce::Colour textLight;
    static const juce::Colour textDim;
    static const juce::Colour accent;

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont(juce::Label& label) override;

private:
    void drawKnobSigil(juce::Graphics& g, float centerX, float centerY, float radius);
    void drawMetalTexture(juce::Graphics& g, juce::Rectangle<float> bounds);
};

} // namespace palace
