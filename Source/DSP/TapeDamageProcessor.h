#pragma once

#include <cmath>
#include <random>
#include <chrono>

namespace palace {

class TapeDamageProcessor
{
public:
    TapeDamageProcessor();
    ~TapeDamageProcessor() = default;

    // Initialization
    void prepare(double sampleRate);
    void reset();

    // Real-time processing
    float processSample(float input, float damageAmount);

private:
    // Low-pass filter state (single-pole)
    float filterState{0.0f};

    // Flutter LFO state
    double flutterPhase{0.0};
    double sampleRate{44100.0};

    // Noise generator
    std::minstd_rand noiseGenerator;
    std::normal_distribution<float> noiseDistribution{0.0f, 1.0f};

    // Constants
    static constexpr float FLUTTER_RATE = 7.3f;  // Hz
    static constexpr float MIN_CUTOFF = 500.0f;   // Hz (maximum damage)
    static constexpr float MAX_CUTOFF = 20000.0f; // Hz (no damage)

    // Helper methods
    float calculateLowPassCoefficient(float cutoffFreq) const;
    float generateNoise();
    float softClip(float sample, float drive) const;
};

} // namespace palace
