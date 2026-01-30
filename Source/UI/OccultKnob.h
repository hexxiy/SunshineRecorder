#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace palace {

class OccultKnob : public juce::Component {
public:
    OccultKnob(const juce::String& labelText = "");
    ~OccultKnob() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Slider& getSlider() { return slider; }

    void setAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

    // MIDI learn highlight
    void setHighlighted(bool highlighted);
    bool isHighlighted() const { return midiLearnHighlight; }

    // LFO modulation visualization
    void setModulationAmount(float amount);  // -1.0 to 1.0
    void setModulationActive(bool active);
    bool isModulationActive() const { return modulationActive; }

private:
    juce::Slider slider;
    juce::Label label;
    bool midiLearnHighlight = false;

    // Modulation display
    bool modulationActive = false;
    float modulationAmount = 0.0f;  // -1.0 to 1.0

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OccultKnob)
};

} // namespace palace
