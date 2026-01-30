#pragma once

#include <cmath>
#include <random>
#include <vector>

namespace palace {

class TapeDelay {
public:
    TapeDelay() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setDelayTime(float delayMs);
    void setFeedback(float fb);
    void setFlutter(float amount);
    void setHiss(float amount);

    void process(float* leftChannel, float* rightChannel, int numSamples);

private:
    float hermiteInterpolate(const std::vector<float>& buffer, float position) const;

    double sampleRate_ = 44100.0;
    int maxBufferSize_ = 0;

    // Circular delay buffer (stereo)
    std::vector<float> bufferL_;
    std::vector<float> bufferR_;
    int writePos_ = 0;

    // Parameters
    float delayTimeMs_ = 300.0f;
    float feedback_ = 0.0f;
    float flutterAmount_ = 0.0f;
    float hissAmount_ = 0.0f;

    // Delay time smoothing
    float smoothedDelaySamples_ = 0.0f;
    float targetDelaySamples_ = 0.0f;
    static constexpr float smoothingCoeff_ = 0.001f;

    // Flutter LFOs
    float lfoPhase1_ = 0.0f;
    float lfoPhase2_ = 0.0f;
    static constexpr float lfoFreq1_ = 3.8f;
    static constexpr float lfoFreq2_ = 5.7f;

    // Tape hiss RNG
    std::mt19937 rng_{42};
    std::uniform_real_distribution<float> noiseDist_{-1.0f, 1.0f};

    // DC blocking filter state (stereo)
    float dcPrevInL_ = 0.0f;
    float dcPrevOutL_ = 0.0f;
    float dcPrevInR_ = 0.0f;
    float dcPrevOutR_ = 0.0f;
    static constexpr float dcCoeff_ = 0.995f;
};

} // namespace palace
