#pragma once

#include <cmath>
#include <random>
#include <juce_core/juce_core.h>

namespace palace {

enum class LFOWaveform {
    Sine = 0,
    Triangle,
    Square,
    Noise,
    SteppedNoise  // Sample & Hold
};

class LFO {
public:
    LFO() : rng(std::random_device{}()), dist(-1.0f, 1.0f) {}

    void prepare(double sr) {
        sampleRate = sr;
        phase = 0.0;
        lastPhase = 0.0;
        heldValue = dist(rng);
    }

    void setFrequency(float hz) {
        frequency = hz;
    }

    void setWaveform(LFOWaveform wf) {
        waveform = wf;
    }

    // Returns value in range [-1, 1]
    float process() {
        float value = 0.0f;

        switch (waveform) {
            case LFOWaveform::Sine:
                value = std::sin(phase * juce::MathConstants<double>::twoPi);
                break;

            case LFOWaveform::Triangle:
                // Triangle wave: goes from -1 to 1 to -1
                if (phase < 0.25)
                    value = static_cast<float>(phase * 4.0);
                else if (phase < 0.75)
                    value = static_cast<float>(2.0 - phase * 4.0);
                else
                    value = static_cast<float>(phase * 4.0 - 4.0);
                break;

            case LFOWaveform::Square:
                value = phase < 0.5 ? 1.0f : -1.0f;
                break;

            case LFOWaveform::Noise:
                // Continuous random noise
                value = dist(rng);
                break;

            case LFOWaveform::SteppedNoise:
                // Sample & Hold - update value at start of each cycle
                if (phase < lastPhase) {
                    heldValue = dist(rng);
                }
                value = heldValue;
                break;
        }

        lastPhase = phase;

        // Advance phase
        phase += frequency / sampleRate;
        if (phase >= 1.0)
            phase -= 1.0;

        return value;
    }

    // Get current value without advancing (for UI display)
    float getCurrentValue() const {
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

    void reset() {
        phase = 0.0;
        lastPhase = 0.0;
        heldValue = dist(rng);
    }

    float getPhase() const {
        return static_cast<float>(phase);
    }

    int getWaveformIndex() const {
        return static_cast<int>(waveform);
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double lastPhase = 0.0;
    float frequency = 1.0f;
    LFOWaveform waveform = LFOWaveform::Sine;

    // Random generator for noise
    mutable std::mt19937 rng;
    mutable std::uniform_real_distribution<float> dist;
    float heldValue = 0.0f;
};

} // namespace palace
