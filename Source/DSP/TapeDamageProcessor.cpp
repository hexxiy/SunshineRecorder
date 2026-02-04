#include "TapeDamageProcessor.h"

namespace palace {

TapeDamageProcessor::TapeDamageProcessor()
{
    // Seed with a different value for each instance
    noiseGenerator.seed(static_cast<unsigned int>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
}

void TapeDamageProcessor::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void TapeDamageProcessor::reset()
{
    filterState = 0.0f;
    flutterPhase = 0.0;
}

float TapeDamageProcessor::processSample(float input, float damageAmount)
{
    if (damageAmount < 0.001f)
        return input;

    // 1. Apply high-frequency loss (low-pass filter)
    const float cutoffFreq = MAX_CUTOFF - (damageAmount * (MAX_CUTOFF - MIN_CUTOFF));
    const float coefficient = calculateLowPassCoefficient(cutoffFreq);
    filterState = coefficient * filterState + (1.0f - coefficient) * input;
    float filtered = filterState;

    // 2. Add tape noise (reduced volume: 1/10th of original)
    const float noiseAmount = damageAmount * 0.0005f;
    const float noise = generateNoise() * noiseAmount;
    float withNoise = filtered * (1.0f - noiseAmount) + noise;

    // 3. Apply tape saturation (soft clip distortion)
    // Drive increases with damage: 0% damage = 1x (clean), 100% damage = 5x (saturated)
    const float drive = 1.0f + (damageAmount * 4.0f);
    float saturated = softClip(withNoise * drive, drive) / drive;

    // 4. Flutter is applied in grain position modulation, not here
    // (This keeps the sample processing simpler)

    return saturated;
}

float TapeDamageProcessor::calculateLowPassCoefficient(float cutoffFreq) const
{
    // Single-pole low-pass filter coefficient
    // coefficient = e^(-2π * cutoff / sampleRate)
    constexpr float twoPi = 6.28318530717958647692f;
    const float omega = twoPi * cutoffFreq / static_cast<float>(sampleRate);
    return std::exp(-omega);
}

float TapeDamageProcessor::generateNoise()
{
    return noiseDistribution(noiseGenerator);
}

float TapeDamageProcessor::softClip(float sample, float drive) const
{
    // Soft clipping using tanh function for smooth tape saturation
    // tanh provides natural compression and harmonic distortion
    return std::tanh(sample);
}

} // namespace palace
