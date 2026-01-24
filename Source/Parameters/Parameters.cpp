#include "Parameters.h"

namespace palace {

Parameters::Parameters(juce::AudioProcessorValueTreeState& apvts) {
    attachParameters(apvts);
}

juce::AudioProcessorValueTreeState::ParameterLayout Parameters::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Position (0-1 normalized)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::position, 1},
        "Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Grain Size (10-2000ms)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::grainSize, 1},
        "Grain Size",
        juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.4f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Density (1-200 grains/sec)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::density, 1},
        "Density",
        juce::NormalisableRange<float>(1.0f, 200.0f, 0.1f, 0.5f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel("g/s")
    ));

    // Pitch (-48 to +48 semitones)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::pitch, 1},
        "Pitch",
        juce::NormalisableRange<float>(-48.0f, 48.0f, 0.01f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("st")
    ));

    // Spray (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::spray, 1},
        "Spray",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Pan Spread (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::panSpread, 1},
        "Pan Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Grain Attack (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::grainAttack, 1},
        "Grain Attack",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Grain Release (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::grainRelease, 1},
        "Grain Release",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        25.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Voice Attack (0-5s)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::voiceAttack, 1},
        "Attack",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f, 0.3f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Voice Decay (0-5s)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::voiceDecay, 1},
        "Decay",
        juce::NormalisableRange<float>(0.0f, 5000.0f, 1.0f, 0.3f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Voice Sustain (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::voiceSustain, 1},
        "Sustain",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        80.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Voice Release (0-10s)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::voiceRelease, 1},
        "Release",
        juce::NormalisableRange<float>(0.0f, 10000.0f, 1.0f, 0.3f),
        500.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")
    ));

    // Mix (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::mix, 1},
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")
    ));

    // Output (-60 to +6 dB)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ParamIDs::output, 1},
        "Output",
        juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f, 2.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")
    ));

    return {params.begin(), params.end()};
}

void Parameters::attachParameters(juce::AudioProcessorValueTreeState& apvts) {
    position     = apvts.getRawParameterValue(ParamIDs::position);
    grainSize    = apvts.getRawParameterValue(ParamIDs::grainSize);
    density      = apvts.getRawParameterValue(ParamIDs::density);
    pitch        = apvts.getRawParameterValue(ParamIDs::pitch);
    spray        = apvts.getRawParameterValue(ParamIDs::spray);
    panSpread    = apvts.getRawParameterValue(ParamIDs::panSpread);
    grainAttack  = apvts.getRawParameterValue(ParamIDs::grainAttack);
    grainRelease = apvts.getRawParameterValue(ParamIDs::grainRelease);

    voiceAttack  = apvts.getRawParameterValue(ParamIDs::voiceAttack);
    voiceDecay   = apvts.getRawParameterValue(ParamIDs::voiceDecay);
    voiceSustain = apvts.getRawParameterValue(ParamIDs::voiceSustain);
    voiceRelease = apvts.getRawParameterValue(ParamIDs::voiceRelease);

    mix    = apvts.getRawParameterValue(ParamIDs::mix);
    output = apvts.getRawParameterValue(ParamIDs::output);
}

} // namespace palace
