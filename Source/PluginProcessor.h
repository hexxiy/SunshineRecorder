#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/SampleBuffer.h"
#include "DSP/Voice.h"
#include "Parameters/Parameters.h"
#include <array>

namespace palace {

class PalaceAudioProcessor : public juce::AudioProcessor {
public:
    static constexpr int NUM_VOICES = 8;

    PalaceAudioProcessor();
    ~PalaceAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Sample management
    void loadSample(const juce::File& file);
    SampleBuffer& getSampleBuffer() { return sampleBuffer; }
    const SampleBuffer& getSampleBuffer() const { return sampleBuffer; }

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    Parameters& getParameters() { return parameters; }

    // Keyboard MIDI input
    void addKeyboardNoteOn(int midiNote, float velocity);
    void addKeyboardNoteOff(int midiNote);

private:
    void handleMidiEvent(const juce::MidiMessage& message);
    void updateVoiceParameters();
    Voice* findFreeVoice();
    Voice* findVoiceForNote(int midiNote);
    Voice* stealVoice();

    juce::AudioProcessorValueTreeState apvts;
    Parameters parameters;

    SampleBuffer sampleBuffer;
    std::array<Voice, NUM_VOICES> voices;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Keyboard MIDI buffer (thread-safe)
    juce::MidiBuffer keyboardMidiBuffer;
    juce::CriticalSection keyboardMidiLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PalaceAudioProcessor)
};

} // namespace palace
