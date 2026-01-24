#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace palace {

namespace ParamIDs {
    // Grain parameters
    inline constexpr const char* position    = "position";
    inline constexpr const char* grainSize   = "grainSize";
    inline constexpr const char* density     = "density";
    inline constexpr const char* pitch       = "pitch";
    inline constexpr const char* spray       = "spray";
    inline constexpr const char* panSpread   = "panSpread";
    inline constexpr const char* grainAttack = "grainAttack";
    inline constexpr const char* grainRelease = "grainRelease";

    // Voice ADSR
    inline constexpr const char* voiceAttack  = "voiceAttack";
    inline constexpr const char* voiceDecay   = "voiceDecay";
    inline constexpr const char* voiceSustain = "voiceSustain";
    inline constexpr const char* voiceRelease = "voiceRelease";

    // Output
    inline constexpr const char* mix    = "mix";
    inline constexpr const char* output = "output";
}

class Parameters {
public:
    Parameters(juce::AudioProcessorValueTreeState& apvts);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Grain parameters (atomic for thread-safe access)
    std::atomic<float>* position    = nullptr;
    std::atomic<float>* grainSize   = nullptr;
    std::atomic<float>* density     = nullptr;
    std::atomic<float>* pitch       = nullptr;
    std::atomic<float>* spray       = nullptr;
    std::atomic<float>* panSpread   = nullptr;
    std::atomic<float>* grainAttack = nullptr;
    std::atomic<float>* grainRelease = nullptr;

    // Voice ADSR
    std::atomic<float>* voiceAttack  = nullptr;
    std::atomic<float>* voiceDecay   = nullptr;
    std::atomic<float>* voiceSustain = nullptr;
    std::atomic<float>* voiceRelease = nullptr;

    // Output
    std::atomic<float>* mix    = nullptr;
    std::atomic<float>* output = nullptr;

private:
    void attachParameters(juce::AudioProcessorValueTreeState& apvts);
};

} // namespace palace
