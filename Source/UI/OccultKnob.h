#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace palace {

class OccultKnob : public juce::Component {
public:
    OccultKnob(const juce::String& labelText = "");
    ~OccultKnob() override;

    void resized() override;

    juce::Slider& getSlider() { return slider; }

    void setAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

private:
    juce::Slider slider;
    juce::Label label;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OccultKnob)
};

} // namespace palace
