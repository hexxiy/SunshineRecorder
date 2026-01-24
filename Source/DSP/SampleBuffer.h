#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <mutex>

namespace palace {

class SampleBuffer {
public:
    SampleBuffer();
    ~SampleBuffer() = default;

    // Load audio file (thread-safe, can be called from any thread)
    bool loadFromFile(const juce::File& file);

    // Clear the buffer
    void clear();

    // Get sample at position with linear interpolation
    float getSampleInterpolated(int channel, double position) const;

    // Get raw sample (no interpolation)
    float getSample(int channel, int position) const;

    // Buffer info
    int getNumSamples() const { return numSamples.load(); }
    int getNumChannels() const { return numChannels.load(); }
    double getSampleRate() const { return sampleRate.load(); }
    bool isLoaded() const { return numSamples.load() > 0; }

    // Get the loaded file path
    juce::String getFilePath() const;
    void setFilePath(const juce::String& path);

    // Access raw buffer for visualization (read-only)
    const juce::AudioBuffer<float>& getBuffer() const { return buffer; }

private:
    juce::AudioBuffer<float> buffer;
    std::atomic<int> numSamples{0};
    std::atomic<int> numChannels{0};
    std::atomic<double> sampleRate{44100.0};
    juce::String filePath;
    mutable std::mutex mutex;
    juce::AudioFormatManager formatManager;
};

} // namespace palace
