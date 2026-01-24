#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace palace {

class SampleBuffer;

struct GrainParameters {
    int startPosition = 0;      // Sample offset in source buffer
    int sizeInSamples = 4410;   // Grain duration (100ms at 44.1kHz)
    float pitchRatio = 1.0f;    // Playback speed (1.0 = original pitch)
    float pan = 0.0f;           // -1 (left) to 1 (right)
    float amplitude = 1.0f;     // Grain volume
    float attackRatio = 0.25f;  // Attack portion of envelope (0-1)
    float releaseRatio = 0.25f; // Release portion of envelope (0-1)
};

class Grain {
public:
    Grain() = default;

    void start(const GrainParameters& params);
    void stop();

    // Process and add grain output to buffer, returns true if grain is still active
    bool process(const SampleBuffer& source, float* leftOutput, float* rightOutput, int numSamples);

    bool isActive() const { return active; }

private:
    float getEnvelopeValue() const;
    float interpolateSample(const SampleBuffer& source, double position) const;

    GrainParameters params;
    double currentPosition = 0.0;  // Current position in grain (in samples)
    int samplesProcessed = 0;      // Samples processed so far
    bool active = false;
};

} // namespace palace
