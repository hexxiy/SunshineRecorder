#pragma once

#include "Grain.h"
#include "SampleBuffer.h"
#include <array>
#include <random>

namespace palace {

struct GrainEngineParameters {
    float position = 0.0f;       // 0-1 normalized position in sample
    float grainSizeMs = 100.0f;  // Grain size in milliseconds
    float density = 10.0f;       // Grains per second
    float pitchSemitones = 0.0f; // Pitch offset in semitones
    float spray = 0.0f;          // Position randomization (0-1)
    float panSpread = 0.5f;      // Pan randomization (0-1)
    float attackRatio = 0.25f;   // Grain envelope attack (0-1)
    float releaseRatio = 0.25f;  // Grain envelope release (0-1)
};

class GrainEngine {
public:
    static constexpr int MAX_GRAINS = 128;

    GrainEngine();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // Process audio - output to stereo buffers
    void process(const SampleBuffer& source,
                 float* leftOutput,
                 float* rightOutput,
                 int numSamples,
                 float noteRatio = 1.0f);  // MIDI note pitch ratio

    // Update parameters
    void setParameters(const GrainEngineParameters& params);

    // Get active grain count for visualization
    int getActiveGrainCount() const;

    // Get active grain info for visualization
    struct GrainInfo {
        float position;  // 0-1 in sample
        float progress;  // 0-1 through grain
        float pan;
    };
    std::vector<GrainInfo> getActiveGrainInfo() const;

private:
    void triggerGrain(const SampleBuffer& source, float noteRatio);
    int findFreeGrain() const;

    std::array<Grain, MAX_GRAINS> grains;
    GrainEngineParameters params;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Scheduling
    double samplesSinceLastGrain = 0.0;
    double samplesPerGrain = 4410.0;  // Based on density

    // Random number generation
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{-1.0f, 1.0f};
};

} // namespace palace
