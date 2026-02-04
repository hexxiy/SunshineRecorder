#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/SampleBuffer.h"
#include "DSP/TapeDisintegrationEngine.h"
#include "DSP/Voice.h"
#include "DSP/LFO.h"
#include "DSP/TapeDelay.h"
#include "Parameters/Parameters.h"
#include <array>
#include <map>
#include <set>

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

    // LFO modulation routing
    void setLfoModulation(const juce::String& paramId, bool enabled);
    bool isLfoModulated(const juce::String& paramId) const;
    const std::set<juce::String>& getLfoModulatedParams() const { return lfoModulatedParams; }
    float getCurrentLfoValue() const { return currentLfoValue.load(); }
    float getCurrentLfoPhase() const { return currentLfoPhase.load(); }
    int getCurrentLfoWaveform() const { return static_cast<int>(lfo.getWaveformIndex()); }

    // MIDI CC mapping
    void setMidiMapping(int ccNumber, const juce::String& paramId);
    void removeMidiMapping(int ccNumber);
    void clearAllMidiMappings();
    const std::map<int, juce::String>& getMidiMappings() const { return midiMappings; }
    juce::String getParameterForCC(int ccNumber) const;
    int getCCForParameter(const juce::String& paramId) const;

    // Crop region
    void setCropRegion(float start, float end);
    float getCropStart() const { return cropStart.load(); }
    float getCropEnd() const { return cropEnd.load(); }

    // Tape disintegration
    TapeDisintegrationEngine& getDisintegrationEngine() { return disintegrationEngine; }
    const TapeDisintegrationEngine& getDisintegrationEngine() const { return disintegrationEngine; }

    // Grain visualization - get all active grains from all voices
    std::vector<GrainEngine::GrainInfo> getAllActiveGrains() const {
        std::vector<GrainEngine::GrainInfo> allGrains;
        for (const auto& voice : voices) {
            if (voice.isActive()) {
                auto voiceGrains = voice.getActiveGrainInfo();
                allGrains.insert(allGrains.end(), voiceGrains.begin(), voiceGrains.end());
            }
        }
        return allGrains;
    }

    // MIDI learn
    void setMidiLearnParameter(const juce::String& paramId) { midiLearnParamId = paramId; }
    void clearMidiLearnParameter() { midiLearnParamId.clear(); }
    juce::String getMidiLearnParameter() const { return midiLearnParamId; }

    // Last received CC (for MIDI learn feedback)
    int getLastReceivedCC() const { return lastReceivedCC.load(); }

    // MIDI debug info
    int getMidiMessageCount() const { return midiMessageCount.load(); }
    int getLastMidiChannel() const { return lastMidiChannel.load(); }
    int getLastMidiType() const { return lastMidiType.load(); }  // 0=none, 1=noteOn, 2=noteOff, 3=CC, 4=other
    int getLastMidiData1() const { return lastMidiData1.load(); }
    int getLastMidiData2() const { return lastMidiData2.load(); }

private:
    // Audio processing constants
    static constexpr float MAX_FEEDBACK_SCALING = 0.85f;   // Prevent runaway feedback
    static constexpr float REVERB_DRY_RETAIN = 0.5f;       // Preserve dry signal in mix
    static constexpr double TAIL_LENGTH_SECONDS = 10.0;    // Max release envelope time

    void handleMidiEvent(const juce::MidiMessage& message);
    void handleMidiLearn(int ccNumber);
    void applyMidiMapping(int ccNumber, int ccValue);
    void updateLFO(int numSamples);
    void processMidiMessages(juce::MidiBuffer& midiMessages);
    void processVoices(juce::AudioBuffer<float>& buffer);
    void applyEffects(juce::AudioBuffer<float>& buffer);
    void applyOutputGain(juce::AudioBuffer<float>& buffer);
    void updateVoiceParameters();
    Voice* findFreeVoice();
    Voice* findVoiceForNote(int midiNote);
    Voice* stealVoice();

    // === Core DSP Components ===
    juce::AudioProcessorValueTreeState apvts;
    Parameters parameters;
    SampleBuffer sampleBuffer;
    std::array<Voice, NUM_VOICES> voices;

    // === Audio Configuration ===
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // === Effects ===
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    TapeDelay tapeDelay;
    TapeDisintegrationEngine disintegrationEngine;

    // === Modulation ===
    LFO lfo;
    std::set<juce::String> lfoModulatedParams;
    std::atomic<float> currentLfoValue{0.0f};
    std::atomic<float> currentLfoPhase{0.0f};

    // === MIDI Processing ===
    juce::MidiBuffer keyboardMidiBuffer;
    juce::CriticalSection keyboardMidiLock;

    std::map<int, juce::String> midiMappings;  // CC number -> param ID
    juce::String midiLearnParamId;              // Parameter awaiting CC assignment
    std::atomic<int> lastReceivedCC{-1};
    juce::CriticalSection midiMappingLock;

    // === Sample Editing ===
    std::atomic<float> cropStart{0.0f};
    std::atomic<float> cropEnd{1.0f};

    // === MIDI Debug Telemetry ===
    std::atomic<int> midiMessageCount{0};
    std::atomic<int> lastMidiChannel{0};
    std::atomic<int> lastMidiType{0};
    std::atomic<int> lastMidiData1{0};
    std::atomic<int> lastMidiData2{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PalaceAudioProcessor)
};

} // namespace palace
