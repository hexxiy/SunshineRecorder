#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace palace {

PalaceAudioProcessor::PalaceAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", Parameters::createParameterLayout()),
      parameters(apvts) {
}

PalaceAudioProcessor::~PalaceAudioProcessor() = default;

const juce::String PalaceAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool PalaceAudioProcessor::acceptsMidi() const {
    return true;
}

bool PalaceAudioProcessor::producesMidi() const {
    return false;
}

bool PalaceAudioProcessor::isMidiEffect() const {
    return false;
}

double PalaceAudioProcessor::getTailLengthSeconds() const {
    return 10.0;  // Maximum release time
}

int PalaceAudioProcessor::getNumPrograms() {
    return 1;
}

int PalaceAudioProcessor::getCurrentProgram() {
    return 0;
}

void PalaceAudioProcessor::setCurrentProgram(int /*index*/) {
}

const juce::String PalaceAudioProcessor::getProgramName(int /*index*/) {
    return {};
}

void PalaceAudioProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/) {
}

void PalaceAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    for (auto& voice : voices) {
        voice.prepare(sampleRate, samplesPerBlock);
    }
}

void PalaceAudioProcessor::releaseResources() {
    for (auto& voice : voices) {
        voice.reset();
    }
}

bool PalaceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PalaceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Clear output buffer
    buffer.clear();

    // Update voice parameters
    updateVoiceParameters();

    // Merge keyboard MIDI with incoming MIDI
    {
        const juce::ScopedLock sl(keyboardMidiLock);
        midiMessages.addEvents(keyboardMidiBuffer, 0, -1, 0);
        keyboardMidiBuffer.clear();
    }

    // Handle MIDI messages
    for (const auto metadata : midiMessages) {
        handleMidiEvent(metadata.getMessage());
    }

    // Get output pointers
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);

    // Process all active voices
    for (auto& voice : voices) {
        if (voice.isActive()) {
            voice.process(sampleBuffer, leftChannel, rightChannel, numSamples);
            voice.incrementAge();
        }
    }

    // Apply output gain
    const float outputDb = parameters.output->load();
    const float outputGain = juce::Decibels::decibelsToGain(outputDb);

    buffer.applyGain(outputGain);
}

void PalaceAudioProcessor::handleMidiEvent(const juce::MidiMessage& message) {
    if (message.isNoteOn()) {
        Voice* voice = findVoiceForNote(message.getNoteNumber());
        if (voice == nullptr) {
            voice = findFreeVoice();
        }
        if (voice == nullptr) {
            voice = stealVoice();
        }
        if (voice != nullptr) {
            voice->noteOn(message.getNoteNumber(), message.getFloatVelocity());
        }
    } else if (message.isNoteOff()) {
        Voice* voice = findVoiceForNote(message.getNoteNumber());
        if (voice != nullptr) {
            voice->noteOff();
        }
    } else if (message.isAllNotesOff() || message.isAllSoundOff()) {
        for (auto& voice : voices) {
            voice.noteOff();
        }
    }
}

void PalaceAudioProcessor::updateVoiceParameters() {
    GrainEngineParameters grainParams;
    grainParams.position = parameters.position->load();
    grainParams.grainSizeMs = parameters.grainSize->load();
    grainParams.density = parameters.density->load();
    grainParams.pitchSemitones = parameters.pitch->load();
    grainParams.spray = parameters.spray->load() / 100.0f;
    grainParams.panSpread = parameters.panSpread->load() / 100.0f;
    grainParams.attackRatio = parameters.grainAttack->load() / 100.0f;
    grainParams.releaseRatio = parameters.grainRelease->load() / 100.0f;

    const float attackMs = parameters.voiceAttack->load();
    const float decayMs = parameters.voiceDecay->load();
    const float sustain = parameters.voiceSustain->load();
    const float releaseMs = parameters.voiceRelease->load();

    for (auto& voice : voices) {
        voice.setGrainParameters(grainParams);
        voice.setADSR(attackMs, decayMs, sustain, releaseMs);
    }
}

Voice* PalaceAudioProcessor::findFreeVoice() {
    for (auto& voice : voices) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

Voice* PalaceAudioProcessor::findVoiceForNote(int midiNote) {
    for (auto& voice : voices) {
        if (voice.isActive() && voice.getCurrentNote() == midiNote) {
            return &voice;
        }
    }
    return nullptr;
}

Voice* PalaceAudioProcessor::stealVoice() {
    // Prefer stealing releasing voices first
    Voice* oldestReleasing = nullptr;
    int maxReleasingAge = -1;

    for (auto& voice : voices) {
        if (voice.isReleasing() && voice.getAge() > maxReleasingAge) {
            maxReleasingAge = voice.getAge();
            oldestReleasing = &voice;
        }
    }

    if (oldestReleasing != nullptr) {
        return oldestReleasing;
    }

    // Otherwise steal oldest active voice
    Voice* oldest = nullptr;
    int maxAge = -1;

    for (auto& voice : voices) {
        if (voice.getAge() > maxAge) {
            maxAge = voice.getAge();
            oldest = &voice;
        }
    }

    return oldest;
}

void PalaceAudioProcessor::addKeyboardNoteOn(int midiNote, float velocity) {
    const juce::ScopedLock sl(keyboardMidiLock);
    keyboardMidiBuffer.addEvent(juce::MidiMessage::noteOn(1, midiNote, velocity), 0);
}

void PalaceAudioProcessor::addKeyboardNoteOff(int midiNote) {
    const juce::ScopedLock sl(keyboardMidiLock);
    keyboardMidiBuffer.addEvent(juce::MidiMessage::noteOff(1, midiNote), 0);
}

void PalaceAudioProcessor::loadSample(const juce::File& file) {
    sampleBuffer.loadFromFile(file);
}

bool PalaceAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* PalaceAudioProcessor::createEditor() {
    return new PalaceAudioProcessorEditor(*this);
}

void PalaceAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();

    // Store sample path
    state.setProperty("samplePath", sampleBuffer.getFilePath(), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PalaceAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(apvts.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);

        // Load sample if path exists
        juce::String samplePath = state.getProperty("samplePath", "").toString();
        if (samplePath.isNotEmpty()) {
            juce::File sampleFile(samplePath);
            if (sampleFile.existsAsFile()) {
                loadSample(sampleFile);
            }
        }
    }
}

} // namespace palace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new palace::PalaceAudioProcessor();
}
