#pragma once

#include "GrainEngine.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace palace {

class TapeDisintegrationEngine;

class Voice {
public:
    Voice();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Start voice with MIDI note
    void noteOn(int midiNote, float velocity);
    void noteOff();

    // Process audio
    void process(const SampleBuffer& source,
                 float* leftOutput,
                 float* rightOutput,
                 int numSamples);

    // Update parameters
    void setGrainParameters(const GrainEngineParameters& params);
    void setADSR(float attackMs, float decayMs, float sustain, float releaseMs);

    // Tape disintegration support
    void setDisintegrationEngine(TapeDisintegrationEngine* de);
    void setDisintegrationAmount(float amount);

    // State queries
    bool isActive() const { return active; }
    bool isReleasing() const { return releasing; }
    int getCurrentNote() const { return currentNote; }

    // For voice stealing - lower age = more recently triggered
    int getAge() const { return age; }
    void incrementAge() { age++; }

    // Get playback regions from grain engine (for damage tracking)
    std::vector<GrainEngine::PlaybackRegion> getActivePlaybackRegions() const {
        return grainEngine.getActivePlaybackRegions();
    }

    // Get active grain info for visualization
    std::vector<GrainEngine::GrainInfo> getActiveGrainInfo() const {
        return grainEngine.getActiveGrainInfo();
    }

private:
    void updateEnvelope(int numSamples);

    GrainEngine grainEngine;

    // Voice state
    bool active = false;
    bool releasing = false;
    int currentNote = -1;
    float velocity = 1.0f;
    int age = 0;

    // ADSR envelope
    enum class EnvelopeStage { Idle, Attack, Decay, Sustain, Release };
    EnvelopeStage envelopeStage = EnvelopeStage::Idle;
    float envelopeValue = 0.0f;
    float envelopeTarget = 0.0f;
    float attackRate = 0.001f;
    float decayRate = 0.001f;
    float sustainLevel = 0.8f;
    float releaseRate = 0.0001f;

    double sampleRate = 44100.0;

    // Pitch ratio for MIDI note (relative to middle C = 60)
    float noteRatio = 1.0f;
    static constexpr int BASE_NOTE = 60;  // Middle C

    // Pre-allocated temporary buffers (avoid allocation in audio thread)
    std::vector<float> tempBufferLeft;
    std::vector<float> tempBufferRight;
};

} // namespace palace
