#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace palace {

class SampleBuffer;
class TapeDamageProcessor;
class TapeDisintegrationEngine;

struct GrainParameters {
    int startPosition = 0;      // Sample offset in source buffer
    int sizeInSamples = 4410;   // Grain duration (100ms at 44.1kHz)
    float pitchRatio = 1.0f;    // Playback speed (1.0 = original pitch)
    float pan = 0.0f;           // -1 (left) to 1 (right)
    float amplitude = 1.0f;     // Grain volume
    float attackRatio = 0.25f;  // Attack portion of envelope (0-1)
    float releaseRatio = 0.25f; // Release portion of envelope (0-1)
    float sampleGainDb = 0.0f;  // Sample gain in dB
};

class Grain {
public:
    Grain() = default;

    void start(const GrainParameters& params);
    void stop();

    // Process and add grain output to buffer, returns true if grain is still active
    bool process(const SampleBuffer& source, float* leftOutput, float* rightOutput, int numSamples);

    bool isActive() const { return active; }

    // Set damage processors for tape disintegration effect
    void setDamageProcessors(TapeDamageProcessor* dp, TapeDisintegrationEngine* de);
    void setDisintegrationAmount(float amount) { disintegrationAmount = amount; }

    // Get last playback region (for damage tracking)
    int getLastPlaybackStart() const { return lastPlaybackStart; }
    int getLastPlaybackEnd() const { return lastPlaybackEnd; }

    // Get current grain state for visualization
    const GrainParameters& getParameters() const { return params; }
    float getProgress() const {
        return params.sizeInSamples > 0 ? static_cast<float>(samplesProcessed) / params.sizeInSamples : 0.0f;
    }

private:
    float getEnvelopeValue() const;
    float interpolateSample(const SampleBuffer& source, double position) const;

    GrainParameters params;
    double currentPosition = 0.0;  // Current position in grain (in samples)
    int samplesProcessed = 0;      // Samples processed so far
    bool active = false;

    // Tape disintegration
    TapeDamageProcessor* damageProcessor = nullptr;
    TapeDisintegrationEngine* disintegrationEngine = nullptr;
    float disintegrationAmount = 0.0f;

    // Track playback region for damage accumulation
    int lastPlaybackStart = 0;
    int lastPlaybackEnd = 0;
};

} // namespace palace
