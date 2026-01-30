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

    // Initialize reverb with spring-like settings
    reverb.setSampleRate(sampleRate);
    reverbParams.roomSize = 0.3f;      // Smaller room for spring character
    reverbParams.damping = 0.3f;       // Less damping for brighter sound
    reverbParams.wetLevel = 0.0f;      // Will be set per block
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 0.8f;         // Good stereo spread
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    // Initialize LFO
    lfo.prepare(sampleRate);

    // Initialize feedback buffer
    feedbackBuffer.setSize(2, samplesPerBlock);
    feedbackBuffer.clear();
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

    // Update LFO
    lfo.setFrequency(parameters.lfoRate->load());
    lfo.setWaveform(static_cast<LFOWaveform>(static_cast<int>(parameters.lfoWaveform->load())));

    // Process LFO once per block (use middle of block value)
    for (int i = 0; i < numSamples / 2; ++i) {
        lfo.process();
    }
    float lfoValue = lfo.process();
    currentLfoValue.store(lfoValue);
    currentLfoPhase.store(lfo.getPhase());
    for (int i = numSamples / 2 + 1; i < numSamples; ++i) {
        lfo.process();
    }

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

    // Apply feedback (mix previous output back in)
    feedbackGain = parameters.feedback->load() / 100.0f * 0.85f;  // Cap at 85% to prevent runaway
    if (feedbackGain > 0.001f) {
        const float* fbLeft = feedbackBuffer.getReadPointer(0);
        const float* fbRight = feedbackBuffer.getReadPointer(1);

        for (int i = 0; i < numSamples; ++i) {
            leftChannel[i] += fbLeft[i] * feedbackGain;
            rightChannel[i] += fbRight[i] * feedbackGain;
        }
    }

    // Store current output for next feedback cycle
    if (feedbackGain > 0.001f) {
        feedbackBuffer.copyFrom(0, 0, buffer, 0, 0, numSamples);
        feedbackBuffer.copyFrom(1, 0, buffer, 1, 0, numSamples);
    }

    // Apply spring reverb
    const float reverbMix = parameters.reverb->load() / 100.0f;
    if (reverbMix > 0.001f) {
        // Update reverb parameters
        reverbParams.wetLevel = reverbMix;
        reverbParams.dryLevel = 1.0f - reverbMix * 0.5f;  // Keep some dry signal
        reverb.setParameters(reverbParams);

        // Process reverb in stereo
        reverb.processStereo(leftChannel, rightChannel, numSamples);
    }

    // Apply output gain
    const float outputDb = parameters.output->load();
    const float outputGain = juce::Decibels::decibelsToGain(outputDb);

    buffer.applyGain(outputGain);
}

void PalaceAudioProcessor::handleMidiEvent(const juce::MidiMessage& message) {
    // Update debug info for all messages
    midiMessageCount.fetch_add(1);
    lastMidiChannel.store(message.getChannel());

    if (message.isNoteOn()) {
        lastMidiType.store(1);
        lastMidiData1.store(message.getNoteNumber());
        lastMidiData2.store(message.getVelocity());

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
        lastMidiType.store(2);
        lastMidiData1.store(message.getNoteNumber());
        lastMidiData2.store(0);

        Voice* voice = findVoiceForNote(message.getNoteNumber());
        if (voice != nullptr) {
            voice->noteOff();
        }
    } else if (message.isAllNotesOff() || message.isAllSoundOff()) {
        for (auto& voice : voices) {
            voice.noteOff();
        }
    } else if (message.isController()) {
        int ccNumber = message.getControllerNumber();
        int ccValue = message.getControllerValue();

        lastMidiType.store(3);
        lastMidiData1.store(ccNumber);
        lastMidiData2.store(ccValue);
        lastReceivedCC.store(ccNumber);

        // Check if we're in MIDI learn mode
        if (midiLearnParamId.isNotEmpty()) {
            const juce::ScopedLock sl(midiMappingLock);
            // Remove any existing mapping for this CC
            midiMappings.erase(ccNumber);
            // Remove any existing mapping for this parameter
            for (auto it = midiMappings.begin(); it != midiMappings.end(); ) {
                if (it->second == midiLearnParamId) {
                    it = midiMappings.erase(it);
                } else {
                    ++it;
                }
            }
            // Create new mapping
            midiMappings[ccNumber] = midiLearnParamId;
            midiLearnParamId.clear();
        }

        // Apply CC to mapped parameter
        {
            const juce::ScopedLock sl(midiMappingLock);
            auto it = midiMappings.find(ccNumber);
            if (it != midiMappings.end()) {
                if (auto* param = apvts.getParameter(it->second)) {
                    float normalizedValue = ccValue / 127.0f;
                    param->setValueNotifyingHost(normalizedValue);
                }
            }
        }
    } else {
        // Other message types
        lastMidiType.store(4);
        lastMidiData1.store(message.getRawData()[0]);
        lastMidiData2.store(message.getRawDataSize() > 1 ? message.getRawData()[1] : 0);
    }
}

void PalaceAudioProcessor::updateVoiceParameters() {
    // Get LFO modulation amount
    float lfoAmount = parameters.lfoAmount->load() / 100.0f;
    float lfoMod = currentLfoValue.load() * lfoAmount;

    // Helper to apply LFO modulation
    auto applyMod = [&](const juce::String& paramId, float value, float range) -> float {
        if (lfoModulatedParams.count(paramId) > 0) {
            return value + lfoMod * range;
        }
        return value;
    };

    GrainEngineParameters grainParams;
    grainParams.position = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::position, parameters.position->load(), 0.5f));
    grainParams.grainSizeMs = juce::jlimit(10.0f, 2000.0f, applyMod(ParamIDs::grainSize, parameters.grainSize->load(), 500.0f));
    grainParams.density = juce::jlimit(1.0f, 200.0f, applyMod(ParamIDs::density, parameters.density->load(), 50.0f));
    grainParams.pitchSemitones = juce::jlimit(-48.0f, 48.0f, applyMod(ParamIDs::pitch, parameters.pitch->load(), 12.0f));
    grainParams.spray = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::spray, parameters.spray->load() / 100.0f, 0.5f));
    grainParams.panSpread = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::panSpread, parameters.panSpread->load() / 100.0f, 0.5f));
    grainParams.attackRatio = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::grainAttack, parameters.grainAttack->load() / 100.0f, 0.25f));
    grainParams.releaseRatio = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::grainRelease, parameters.grainRelease->load() / 100.0f, 0.25f));

    const float attackMs = juce::jlimit(0.0f, 5000.0f, applyMod(ParamIDs::voiceAttack, parameters.voiceAttack->load(), 500.0f));
    const float decayMs = juce::jlimit(0.0f, 5000.0f, applyMod(ParamIDs::voiceDecay, parameters.voiceDecay->load(), 500.0f));
    const float sustain = juce::jlimit(0.0f, 100.0f, applyMod(ParamIDs::voiceSustain, parameters.voiceSustain->load(), 25.0f));
    const float releaseMs = juce::jlimit(0.0f, 10000.0f, applyMod(ParamIDs::voiceRelease, parameters.voiceRelease->load(), 1000.0f));

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

void PalaceAudioProcessor::setMidiMapping(int ccNumber, const juce::String& paramId) {
    const juce::ScopedLock sl(midiMappingLock);
    midiMappings[ccNumber] = paramId;
}

void PalaceAudioProcessor::removeMidiMapping(int ccNumber) {
    const juce::ScopedLock sl(midiMappingLock);
    midiMappings.erase(ccNumber);
}

void PalaceAudioProcessor::clearAllMidiMappings() {
    const juce::ScopedLock sl(midiMappingLock);
    midiMappings.clear();
}

juce::String PalaceAudioProcessor::getParameterForCC(int ccNumber) const {
    auto it = midiMappings.find(ccNumber);
    if (it != midiMappings.end()) {
        return it->second;
    }
    return {};
}

int PalaceAudioProcessor::getCCForParameter(const juce::String& paramId) const {
    for (const auto& mapping : midiMappings) {
        if (mapping.second == paramId) {
            return mapping.first;
        }
    }
    return -1;
}

void PalaceAudioProcessor::loadSample(const juce::File& file) {
    sampleBuffer.loadFromFile(file);
}

void PalaceAudioProcessor::setLfoModulation(const juce::String& paramId, bool enabled) {
    if (enabled) {
        lfoModulatedParams.insert(paramId);
    } else {
        lfoModulatedParams.erase(paramId);
    }
}

bool PalaceAudioProcessor::isLfoModulated(const juce::String& paramId) const {
    return lfoModulatedParams.count(paramId) > 0;
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

    // Store MIDI mappings
    juce::ValueTree midiMappingsTree("MidiMappings");
    {
        const juce::ScopedLock sl(midiMappingLock);
        for (const auto& mapping : midiMappings) {
            juce::ValueTree mappingTree("Mapping");
            mappingTree.setProperty("cc", mapping.first, nullptr);
            mappingTree.setProperty("param", mapping.second, nullptr);
            midiMappingsTree.appendChild(mappingTree, nullptr);
        }
    }
    state.appendChild(midiMappingsTree, nullptr);

    // Store LFO modulation routing
    juce::ValueTree lfoModTree("LfoModulation");
    for (const auto& paramId : lfoModulatedParams) {
        juce::ValueTree modTree("Mod");
        modTree.setProperty("param", paramId, nullptr);
        lfoModTree.appendChild(modTree, nullptr);
    }
    state.appendChild(lfoModTree, nullptr);

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

        // Load MIDI mappings
        auto midiMappingsTree = state.getChildWithName("MidiMappings");
        if (midiMappingsTree.isValid()) {
            const juce::ScopedLock sl(midiMappingLock);
            midiMappings.clear();
            for (int i = 0; i < midiMappingsTree.getNumChildren(); ++i) {
                auto mappingTree = midiMappingsTree.getChild(i);
                int cc = mappingTree.getProperty("cc", -1);
                juce::String param = mappingTree.getProperty("param", "").toString();
                if (cc >= 0 && param.isNotEmpty()) {
                    midiMappings[cc] = param;
                }
            }
        }

        // Load LFO modulation routing
        auto lfoModTree = state.getChildWithName("LfoModulation");
        if (lfoModTree.isValid()) {
            lfoModulatedParams.clear();
            for (int i = 0; i < lfoModTree.getNumChildren(); ++i) {
                auto modTree = lfoModTree.getChild(i);
                juce::String param = modTree.getProperty("param", "").toString();
                if (param.isNotEmpty()) {
                    lfoModulatedParams.insert(param);
                }
            }
        }
    }
}

} // namespace palace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new palace::PalaceAudioProcessor();
}
