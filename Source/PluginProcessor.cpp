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
    return TAIL_LENGTH_SECONDS;
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

    // Initialize tape delay
    tapeDelay.prepare(sampleRate, samplesPerBlock);

    // Initialize disintegration engine
    if (sampleBuffer.isLoaded()) {
        disintegrationEngine.prepare(sampleRate, sampleBuffer.getNumSamples());
    }

    // Wire disintegration engine to voices
    for (auto& voice : voices) {
        voice.setDisintegrationEngine(&disintegrationEngine);
    }
}

void PalaceAudioProcessor::releaseResources() {
    for (auto& voice : voices) {
        voice.reset();
    }
    tapeDelay.reset();
}

bool PalaceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PalaceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();

    updateLFO(numSamples);
    updateVoiceParameters();

    // Hit tracking now happens per-sample inside Grain::process()

    processMidiMessages(midiMessages);
    processVoices(buffer);
    applyEffects(buffer);
    applyOutputGain(buffer);
}

void PalaceAudioProcessor::updateLFO(int numSamples) {
    lfo.setFrequency(parameters.lfoRate->load());
    lfo.setWaveform(static_cast<LFOWaveform>(static_cast<int>(parameters.lfoWaveform->load())));

    // Process LFO once per block (use middle of block value)
    // Advance to middle, compute value, then advance to end
    lfo.advancePhase(numSamples / 2);
    float lfoValue = lfo.process();
    currentLfoValue.store(lfoValue);
    currentLfoPhase.store(lfo.getPhase());
    lfo.advancePhase(numSamples - numSamples / 2 - 1);
}

void PalaceAudioProcessor::processMidiMessages(juce::MidiBuffer& midiMessages) {
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
}

void PalaceAudioProcessor::processVoices(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);

    // Process all active voices
    for (auto& voice : voices) {
        if (voice.isActive()) {
            voice.process(sampleBuffer, leftChannel, rightChannel, numSamples);
            voice.incrementAge();
        }
    }
}

void PalaceAudioProcessor::applyEffects(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);

    // Apply tape delay
    tapeDelay.setDelayTime(parameters.delayTime->load());
    tapeDelay.setFeedback(parameters.feedback->load() / 100.0f * MAX_FEEDBACK_SCALING);
    tapeDelay.setFlutter(parameters.flutter->load() / 100.0f);
    tapeDelay.setHiss(parameters.tapeHiss->load() / 100.0f);
    tapeDelay.process(leftChannel, rightChannel, numSamples);

    // Apply spring reverb
    const float reverbMix = parameters.reverb->load() / 100.0f;
    if (reverbMix > 0.001f) {
        reverbParams.wetLevel = reverbMix;
        reverbParams.dryLevel = 1.0f - reverbMix * REVERB_DRY_RETAIN;
        reverb.setParameters(reverbParams);
        reverb.processStereo(leftChannel, rightChannel, numSamples);
    }
}

void PalaceAudioProcessor::applyOutputGain(juce::AudioBuffer<float>& buffer) {
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

        handleMidiLearn(ccNumber);
        applyMidiMapping(ccNumber, ccValue);
    } else {
        // Other message types
        lastMidiType.store(4);
        lastMidiData1.store(message.getRawData()[0]);
        lastMidiData2.store(message.getRawDataSize() > 1 ? message.getRawData()[1] : 0);
    }
}

void PalaceAudioProcessor::handleMidiLearn(int ccNumber) {
    if (midiLearnParamId.isEmpty())
        return;

    const juce::ScopedLock sl(midiMappingLock);

    // Remove any existing mapping for this CC
    midiMappings.erase(ccNumber);

    // Remove any existing mapping for this parameter (one param = one CC)
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

void PalaceAudioProcessor::applyMidiMapping(int ccNumber, int ccValue) {
    const juce::ScopedLock sl(midiMappingLock);
    auto it = midiMappings.find(ccNumber);
    if (it != midiMappings.end()) {
        if (auto* param = apvts.getParameter(it->second)) {
            float normalizedValue = ccValue / 127.0f;
            param->setValueNotifyingHost(normalizedValue);
        }
    }
}

void PalaceAudioProcessor::updateVoiceParameters() {
    // Update disintegration engine parameters
    disintegrationEngine.setMaxLife(parameters.life->load());
    disintegrationEngine.setEnabled(parameters.damage->load() > 0.01f);

    // Get LFO modulation amount
    float lfoAmount = parameters.lfoAmount->load() / 100.0f;
    float lfoMod = currentLfoValue.load() * lfoAmount;

    // Helper to apply LFO modulation to parameters
    // If parameter is in lfoModulatedParams set, adds bipolar modulation (±range)
    auto applyMod = [&](const juce::String& paramId, float value, float range) -> float {
        if (lfoModulatedParams.count(paramId) > 0) {
            return value + lfoMod * range;
        }
        return value;
    };

    // Build grain engine parameters with LFO modulation
    GrainEngineParameters grainParams;

    // Map user position (0-1) to crop region bounds: start + t*(end-start)
    float rawPosition = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::position, parameters.position->load(), 0.5f));
    float cs = cropStart.load();
    float ce = cropEnd.load();
    grainParams.position = cs + rawPosition * (ce - cs);
    grainParams.grainSizeMs = juce::jlimit(10.0f, 8000.0f, applyMod(ParamIDs::grainSize, parameters.grainSize->load(), 500.0f));

    // Clamp grain size so the grain window cannot extend past crop boundaries
    if (sampleBuffer.isLoaded() && sampleBuffer.getNumSamples() > 0) {
        double sampleDurationMs = (sampleBuffer.getNumSamples() / sampleBuffer.getSampleRate()) * 1000.0;
        float cropWidthMs = static_cast<float>((ce - cs) * sampleDurationMs);
        grainParams.grainSizeMs = std::min(grainParams.grainSizeMs, cropWidthMs);
    }
    grainParams.density = juce::jlimit(1.0f, 200.0f, applyMod(ParamIDs::density, parameters.density->load(), 50.0f));
    grainParams.pitchSemitones = juce::jlimit(-48.0f, 48.0f, applyMod(ParamIDs::pitch, parameters.pitch->load(), 12.0f));
    grainParams.spray = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::spray, parameters.spray->load() / 100.0f, 0.5f));
    grainParams.panSpread = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::panSpread, parameters.panSpread->load() / 100.0f, 0.5f));
    grainParams.attackRatio = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::grainAttack, parameters.grainAttack->load() / 100.0f, 0.25f));
    grainParams.releaseRatio = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::grainRelease, parameters.grainRelease->load() / 100.0f, 0.25f));
    grainParams.cropStart = cs;
    grainParams.cropEnd = ce;
    grainParams.sampleGainDb = parameters.sampleGain->load();

    const float attackMs = juce::jlimit(0.0f, 5000.0f, applyMod(ParamIDs::voiceAttack, parameters.voiceAttack->load(), 500.0f));
    const float decayMs = juce::jlimit(0.0f, 5000.0f, applyMod(ParamIDs::voiceDecay, parameters.voiceDecay->load(), 500.0f));
    const float sustain = juce::jlimit(0.0f, 100.0f, applyMod(ParamIDs::voiceSustain, parameters.voiceSustain->load(), 25.0f));
    const float releaseMs = juce::jlimit(0.0f, 10000.0f, applyMod(ParamIDs::voiceRelease, parameters.voiceRelease->load(), 1000.0f));

    // Get disintegration amount
    const float disintegrationAmount = parameters.damage->load();

    for (auto& voice : voices) {
        voice.setGrainParameters(grainParams);
        voice.setADSR(attackMs, decayMs, sustain, releaseMs);
        voice.setDisintegrationAmount(disintegrationAmount);
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
    cropStart.store(0.0f);
    cropEnd.store(1.0f);

    // Reset damage when loading a new sample
    disintegrationEngine.reset();
    if (sampleBuffer.isLoaded()) {
        disintegrationEngine.prepare(currentSampleRate, sampleBuffer.getNumSamples());
    }
}

void PalaceAudioProcessor::setCropRegion(float start, float end) {
    cropStart.store(juce::jlimit(0.0f, 1.0f, start));
    cropEnd.store(juce::jlimit(0.0f, 1.0f, end));
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

    // Store crop region
    state.setProperty("cropStart", static_cast<double>(cropStart.load()), nullptr);
    state.setProperty("cropEnd", static_cast<double>(cropEnd.load()), nullptr);

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

    // Store life map
    auto lifeMap = disintegrationEngine.getLifeMap();
    juce::ValueTree lifeTree("Life");
    for (int i = 0; i < static_cast<int>(lifeMap.size()); ++i) {
        if (lifeMap[i] < 0.999f) {  // Only save depleted regions
            lifeTree.setProperty(juce::String(i), lifeMap[i], nullptr);
        }
    }
    if (lifeTree.getNumProperties() > 0) {
        state.appendChild(lifeTree, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PalaceAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(apvts.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);

        // Load sample if path exists (don't use loadSample() to avoid resetting crop/damage)
        juce::String samplePath = state.getProperty("samplePath", "").toString();
        if (samplePath.isNotEmpty()) {
            juce::File sampleFile(samplePath);
            if (sampleFile.existsAsFile()) {
                sampleBuffer.loadFromFile(sampleFile);
                if (sampleBuffer.isLoaded()) {
                    disintegrationEngine.prepare(currentSampleRate, sampleBuffer.getNumSamples());
                }
            }
        }

        // Load crop region (must be loaded before damage map)
        cropStart.store(static_cast<float>(static_cast<double>(state.getProperty("cropStart", 0.0))));
        cropEnd.store(static_cast<float>(static_cast<double>(state.getProperty("cropEnd", 1.0))));

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

        // Load life map (or legacy damage map for backwards compatibility)
        auto lifeTree = state.getChildWithName("Life");
        if (lifeTree.isValid()) {
            disintegrationEngine.reset();  // Reset to full life
            for (int i = 0; i < lifeTree.getNumProperties(); ++i) {
                auto prop = lifeTree.getPropertyName(i);
                int regionIndex = prop.toString().getIntValue();
                float life = lifeTree.getProperty(prop);
                disintegrationEngine.setRegionLife(regionIndex, life);
            }
        } else {
            // Legacy support: convert old "Damage" tree to life values
            auto damageTree = state.getChildWithName("Damage");
            if (damageTree.isValid()) {
                disintegrationEngine.reset();  // Reset to full life
                for (int i = 0; i < damageTree.getNumProperties(); ++i) {
                    auto prop = damageTree.getPropertyName(i);
                    int regionIndex = prop.toString().getIntValue();
                    float damage = damageTree.getProperty(prop);
                    float life = 1.0f - damage;  // Convert damage to life
                    disintegrationEngine.setRegionLife(regionIndex, life);
                }
            }
        }
    }
}

} // namespace palace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new palace::PalaceAudioProcessor();
}
