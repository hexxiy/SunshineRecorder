# Code Review: Data Structure and Readability Improvements

## Summary

The codebase is well-architected with good separation of concerns. However, there are opportunities to improve data structures for efficiency and readability for maintainability. This review focuses on concrete, actionable improvements.

---

## Critical Issues

### 1. Memory Allocation in Audio Thread (Voice.cpp:69-70)

**Issue**: Temporary buffers allocated per voice, per process call
```cpp
std::vector<float> tempLeft(numSamples, 0.0f);
std::vector<float> tempRight(numSamples, 0.0f);
```

**Problem**: Dynamic memory allocation in real-time audio thread causes:
- Potential allocation failures
- CPU cache misses
- Non-deterministic timing (GC/heap fragmentation)

**Solution**: Pre-allocate as class members
```cpp
// In Voice.h, add private members:
std::vector<float> tempBufferLeft;
std::vector<float> tempBufferRight;

// In Voice::prepare()
tempBufferLeft.resize(samplesPerBlock);
tempBufferRight.resize(samplesPerBlock);

// In Voice::process()
std::fill(tempBufferLeft.begin(), tempBufferLeft.begin() + numSamples, 0.0f);
std::fill(tempBufferRight.begin(), tempBufferRight.begin() + numSamples, 0.0f);
grainEngine.process(source, tempBufferLeft.data(), tempBufferRight.data(), numSamples, noteRatio);
```

**Priority**: HIGH - Audio glitches possible

---

### 2. Inefficient LFO Processing (PluginProcessor.cpp:106-114)

**Issue**: LFO processes all samples but only uses middle value
```cpp
for (int i = 0; i < numSamples / 2; ++i) {
    lfo.process();
}
float lfoValue = lfo.process();
currentLfoValue.store(lfoValue);
currentLfoPhase.store(lfo.getPhase());
for (int i = numSamples / 2 + 1; i < numSamples; ++i) {
    lfo.process();
}
```

**Problem**: Wastes CPU cycles (100+ process calls per block discarded)

**Solution**: Use phase advance directly
```cpp
// Option 1: Just advance to middle of block
lfo.advancePhase(numSamples / 2);
float lfoValue = lfo.process();
currentLfoValue.store(lfoValue);
currentLfoPhase.store(lfo.getPhase());
lfo.advancePhase(numSamples - numSamples / 2 - 1);

// Option 2: Only process at middle sample
lfo.advancePhase(numSamples / 2);
float lfoValue = lfo.getCurrentValue();
currentLfoValue.store(lfoValue);
currentLfoPhase.store(lfo.getPhase());
lfo.advancePhase(numSamples - numSamples / 2);
```

Add to LFO.h:
```cpp
void advancePhase(int samples) {
    phase += (frequency / sampleRate) * samples;
    while (phase >= 1.0)
        phase -= 1.0;
}
```

**Priority**: MEDIUM - CPU optimization

---

### 3. Unused Struct Definition (PluginProcessor.h:17-20)

**Issue**: `MidiMapping` struct is defined but never used
```cpp
struct MidiMapping {
    int ccNumber = -1;
    juce::String paramId;
};
```

**Problem**: Dead code, confusing since `std::map<int, juce::String>` is used instead

**Solution**: Remove the struct entirely

**Priority**: LOW - Cleanup

---

## Readability Issues

### 4. Long processBlock Method (PluginProcessor.cpp:92-167)

**Issue**: 75-line method doing multiple distinct tasks

**Solution**: Extract logical sections into private methods
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    updateLFO(buffer.getNumSamples());
    updateVoiceParameters();
    processMidiMessages(midiMessages);
    processVoices(buffer);
    applyEffects(buffer);
    applyOutputGain(buffer);
}

private:
    void updateLFO(int numSamples);
    void processMidiMessages(juce::MidiBuffer& midiMessages);
    void processVoices(juce::AudioBuffer<float>& buffer);
    void applyEffects(juce::AudioBuffer<float>& buffer);
    void applyOutputGain(juce::AudioBuffer<float>& buffer);
```

**Priority**: MEDIUM - Maintainability

---

### 5. Magic Numbers Without Named Constants

**Issue**: Scattered literal values with unclear meaning

**Examples**:
- `PluginProcessor.cpp:145`: `0.85f` - Why 85% max feedback?
- `PluginProcessor.cpp:155`: `0.5f` - Why keep 50% dry signal?
- `PluginProcessor.cpp:32`: `10.0` - Why 10 second tail?

**Solution**: Named constants at class scope
```cpp
// In PluginProcessor.h private section:
static constexpr float MAX_FEEDBACK_SCALING = 0.85f;  // Prevent runaway feedback
static constexpr float REVERB_DRY_RETAIN = 0.5f;      // Preserve dry signal in mix
static constexpr double TAIL_LENGTH_SECONDS = 10.0;   // Max release envelope time
```

**Priority**: MEDIUM - Code clarity

---

### 6. Inconsistent Naming Conventions

**Issue**: Mixed naming styles across files

**Examples**:
- TapeDelay uses trailing underscores: `bufferL_`, `sampleRate_`, `lfoPhase1_`
- Other classes don't: `sampleRate`, `currentNote`, `envelopeValue`

**Solution**: Choose one convention and apply consistently
- **Recommendation**: No trailing underscores (matches JUCE style and rest of codebase)
- Refactor TapeDelay member names in next iteration

**Priority**: LOW - Consistency

---

### 7. Complex Lambda in updateVoiceParameters (PluginProcessor.cpp:254-259)

**Issue**: Inline lambda makes logic harder to trace
```cpp
auto applyMod = [&](const juce::String& paramId, float value, float range) -> float {
    if (lfoModulatedParams.count(paramId) > 0) {
        return value + lfoMod * range;
    }
    return value;
};
```

**Solution**: Make it a named private method with clear documentation
```cpp
private:
    /// Applies LFO modulation to a parameter if enabled in routing
    /// @param paramId Parameter ID to check
    /// @param baseValue Current parameter value
    /// @param modulationRange Bipolar modulation range (±range)
    /// @return Modulated value if enabled, otherwise baseValue
    float applyLfoModulation(const juce::String& paramId, float baseValue, float modulationRange) const {
        if (lfoModulatedParams.count(paramId) > 0) {
            return baseValue + currentLfoValue.load() * (parameters.lfoAmount->load() / 100.0f) * modulationRange;
        }
        return baseValue;
    }
```

**Priority**: MEDIUM - Code clarity

---

### 8. Duplicated Waveform Logic (LFO.h)

**Issue**: `getCurrentValue()` duplicates entire waveform generation from `process()`

**Solution**: Extract waveform generation to helper
```cpp
private:
    float generateWaveform() const {
        switch (waveform) {
            case LFOWaveform::Sine:
                return static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));
            case LFOWaveform::Triangle:
                if (phase < 0.25)
                    return static_cast<float>(phase * 4.0);
                else if (phase < 0.75)
                    return static_cast<float>(2.0 - phase * 4.0);
                else
                    return static_cast<float>(phase * 4.0 - 4.0);
            case LFOWaveform::Square:
                return phase < 0.5 ? 1.0f : -1.0f;
            case LFOWaveform::Noise:
            case LFOWaveform::SteppedNoise:
                return heldValue;
        }
        return 0.0f;
    }

public:
    float process() {
        float value = generateWaveform();

        // Handle sample & hold phase detection
        if (waveform == LFOWaveform::SteppedNoise && phase < lastPhase) {
            heldValue = dist(rng);
        }

        lastPhase = phase;

        // Advance phase
        phase += frequency / sampleRate;
        if (phase >= 1.0)
            phase -= 1.0;

        return value;
    }

    float getCurrentValue() const {
        return generateWaveform();
    }
```

**Priority**: LOW - DRY principle

---

### 9. Inefficient Reverse Lookup (PluginProcessor.cpp:375-382)

**Issue**: Linear search to find CC for parameter
```cpp
int PalaceAudioProcessor::getCCForParameter(const juce::String& paramId) const {
    for (const auto& mapping : midiMappings) {
        if (mapping.second == paramId) {
            return mapping.first;
        }
    }
    return -1;
}
```

**Problem**: O(n) lookup called from UI thread (acceptable) but could be O(1)

**Solution**: Maintain bidirectional map if this becomes a performance issue
```cpp
// In PluginProcessor.h:
std::map<int, juce::String> midiMappings;         // CC -> paramId
std::map<juce::String, int> reverseMappings;       // paramId -> CC

// Update both maps in setMidiMapping(), removeMidiMapping(), etc.
```

**Priority**: LOW - Not currently a bottleneck

---

### 10. Complex Nested MIDI Learn Logic (PluginProcessor.cpp:211-227)

**Issue**: Deeply nested conditionals in MIDI handler

**Solution**: Extract to dedicated method
```cpp
void PalaceAudioProcessor::handleMidiLearn(int ccNumber) {
    if (midiLearnParamId.isEmpty())
        return;

    const juce::ScopedLock sl(midiMappingLock);

    // Clear existing mapping for this CC
    midiMappings.erase(ccNumber);

    // Clear existing mapping for this parameter (one param = one CC)
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

// In handleMidiEvent() where CC is detected:
if (message.isController()) {
    int ccNumber = message.getControllerNumber();
    int ccValue = message.getControllerValue();

    lastMidiType.store(3);
    lastMidiData1.store(ccNumber);
    lastMidiData2.store(ccValue);
    lastReceivedCC.store(ccNumber);

    handleMidiLearn(ccNumber);
    applyMidiMapping(ccNumber, ccValue);
}
```

**Priority**: MEDIUM - Readability

---

### 11. Sparse Comments on Complex Logic

**Issue**: Missing documentation on non-obvious algorithms

**Examples needing comments**:
- Voice envelope update per-sample loop (Voice.cpp:95-133)
- Grain envelope calculation (Grain.cpp - not shown but likely)
- Hermite interpolation (TapeDelay.cpp - not shown)
- Crop region remapping (PluginProcessor.cpp:263-265)

**Solution**: Add brief algorithmic explanations
```cpp
// Voice.cpp:95
void Voice::updateEnvelope(int numSamples) {
    // Per-sample ADSR envelope generation
    // Linear ramps for attack/decay/release stages
    for (int i = 0; i < numSamples; ++i) {
        // ... existing code with stage comments
    }
}

// PluginProcessor.cpp:263
// Map user position (0-1) to crop region bounds
float rawPosition = juce::jlimit(0.0f, 1.0f, applyMod(ParamIDs::position, parameters.position->load(), 0.5f));
float cs = cropStart.load();
float ce = cropEnd.load();
grainParams.position = cs + rawPosition * (ce - cs);  // Linear interpolation: start + t*(end-start)
```

**Priority**: LOW - Documentation

---

## Data Structure Improvements

### 12. Separate MIDI and Debug State (PluginProcessor.h)

**Issue**: MIDI mapping logic mixed with debug counters in header

**Solution**: Group related state with comments
```cpp
private:
    // === Core DSP Components ===
    juce::AudioProcessorValueTreeState apvts;
    Parameters parameters;
    SampleBuffer sampleBuffer;
    std::array<Voice, NUM_VOICES> voices;

    // === Effects ===
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    TapeDelay tapeDelay;

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

    // === Audio Config ===
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
```

**Priority**: LOW - Organization

---

### 13. Extract State Serialization (PluginProcessor.cpp:415-499)

**Issue**: State save/load methods are 40+ lines each

**Solution**: Helper methods for each subsystem
```cpp
void PalaceAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();

    saveSampleState(state);
    saveCropState(state);
    saveMidiMappings(state);
    saveLfoModulation(state);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

private:
    void saveSampleState(juce::ValueTree& state);
    void saveCropState(juce::ValueTree& state);
    void saveMidiMappings(juce::ValueTree& state);
    void saveLfoModulation(juce::ValueTree& state);

    void loadSampleState(const juce::ValueTree& state);
    void loadCropState(const juce::ValueTree& state);
    void loadMidiMappings(const juce::ValueTree& state);
    void loadLfoModulation(const juce::ValueTree& state);
```

**Priority**: MEDIUM - Maintainability

---

## Suggested Refactoring Priority

1. **HIGH**: Fix Voice temporary buffer allocation (audio safety)
2. **MEDIUM**: Extract processBlock sections (maintainability)
3. **MEDIUM**: Optimize LFO processing (CPU efficiency)
4. **MEDIUM**: Extract MIDI learn method (readability)
5. **MEDIUM**: Add named constants for magic numbers
6. **LOW**: Remove unused MidiMapping struct
7. **LOW**: Extract state serialization helpers
8. **LOW**: Unify naming conventions
9. **LOW**: Add algorithmic comments

---

## Positive Observations

The following are done well:
- Thread-safe parameter access via atomics
- Clear voice management with voice stealing
- Good separation of DSP from UI
- Proper use of JUCE idioms
- No allocations in grain processing (good!)
- Static arrays for voices and grains (excellent!)

---

## Testing Recommendations

After refactoring:
1. Profile with Valgrind or ASan to verify no allocations in audio thread
2. Benchmark LFO changes to confirm CPU savings
3. Test MIDI learn edge cases (rapid CC changes, concurrent learn requests)
4. Verify state persistence across DAW save/load cycles

---

## Conclusion

The codebase demonstrates solid audio programming fundamentals. The suggested improvements focus on:
- **Eliminating audio thread allocations** (critical)
- **Improving code organization** for future maintenance
- **Removing unnecessary CPU work** in hot paths
- **Making complex logic more readable** through extraction and naming

Prioritize the HIGH items first, then address MEDIUM items during natural code evolution.
