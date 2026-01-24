#include "OccultKnob.h"
#include "OccultLookAndFeel.h"

namespace palace {

OccultKnob::OccultKnob(const juce::String& labelText) {
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, OccultLookAndFeel::textDim);
    addAndMakeVisible(label);
}

OccultKnob::~OccultKnob() = default;

void OccultKnob::resized() {
    auto bounds = getLocalBounds();

    const int labelHeight = 18;
    label.setBounds(bounds.removeFromTop(labelHeight));

    slider.setBounds(bounds);
}

void OccultKnob::setAttachment(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId) {
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
}

} // namespace palace
