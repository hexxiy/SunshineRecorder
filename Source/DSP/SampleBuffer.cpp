#include "SampleBuffer.h"

namespace palace {

SampleBuffer::SampleBuffer() {
    formatManager.registerBasicFormats();
}

bool SampleBuffer::loadFromFile(const juce::File& file) {
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    juce::AudioBuffer<float> newBuffer;
    newBuffer.setSize(static_cast<int>(reader->numChannels),
                      static_cast<int>(reader->lengthInSamples));
    reader->read(&newBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    {
        std::lock_guard<std::mutex> lock(mutex);
        buffer = std::move(newBuffer);
        numSamples.store(buffer.getNumSamples());
        numChannels.store(buffer.getNumChannels());
        sampleRate.store(reader->sampleRate);
        filePath = file.getFullPathName();
    }

    return true;
}

void SampleBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    buffer.setSize(0, 0);
    numSamples.store(0);
    numChannels.store(0);
    filePath.clear();
}

float SampleBuffer::getSampleInterpolated(int channel, double position) const {
    const int samples = numSamples.load();
    if (samples == 0 || channel >= numChannels.load())
        return 0.0f;

    // Wrap position to valid range
    while (position < 0.0)
        position += samples;
    while (position >= samples)
        position -= samples;

    const int index0 = static_cast<int>(position);
    const int index1 = (index0 + 1) % samples;
    const float frac = static_cast<float>(position - index0);

    const float sample0 = buffer.getSample(channel, index0);
    const float sample1 = buffer.getSample(channel, index1);

    return sample0 + frac * (sample1 - sample0);
}

float SampleBuffer::getSample(int channel, int position) const {
    const int samples = numSamples.load();
    if (samples == 0 || channel >= numChannels.load())
        return 0.0f;

    // Wrap position to valid range
    position = position % samples;
    if (position < 0)
        position += samples;

    return buffer.getSample(channel, position);
}

juce::String SampleBuffer::getFilePath() const {
    std::lock_guard<std::mutex> lock(mutex);
    return filePath;
}

void SampleBuffer::setFilePath(const juce::String& path) {
    std::lock_guard<std::mutex> lock(mutex);
    filePath = path;
}

} // namespace palace
