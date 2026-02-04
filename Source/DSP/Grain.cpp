#include "Grain.h"
#include "SampleBuffer.h"
#include "TapeDamageProcessor.h"
#include "TapeDisintegrationEngine.h"

namespace palace {

void Grain::start(const GrainParameters& p) {
    params = p;
    currentPosition = 0.0;
    samplesProcessed = 0;
    active = true;
}

void Grain::stop() {
    active = false;
}

bool Grain::process(const SampleBuffer& source, float* leftOutput, float* rightOutput, int numSamples) {
    if (!active || !source.isLoaded())
        return false;

    // Calculate pan gains (constant power)
    const float panAngle = (params.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    const float leftGain = std::cos(panAngle);
    const float rightGain = std::sin(panAngle);

    // Track playback region for damage accumulation
    double minPos = currentPosition + params.startPosition;
    double maxPos = minPos;

    for (int i = 0; i < numSamples && active; ++i) {
        // Check if grain has finished
        if (samplesProcessed >= params.sizeInSamples) {
            active = false;
            break;
        }

        // Calculate source position
        const double sourcePos = params.startPosition + currentPosition;

        // Track region bounds
        minPos = std::min(minPos, sourcePos);
        maxPos = std::max(maxPos, sourcePos);

        // Get interpolated sample from source
        float sample = interpolateSample(source, sourcePos);

        // Apply sample gain (convert dB to linear)
        if (params.sampleGainDb != 0.0f) {
            float gainLinear = std::pow(10.0f, params.sampleGainDb / 20.0f);
            sample *= gainLinear;
        }

        // Apply tape damage if enabled
        if (damageProcessor && disintegrationEngine && disintegrationAmount > 0.001f) {
            int sampleIndex = static_cast<int>(sourcePos);

            // Decrement life on each sample read
            disintegrationEngine->decrementLife(sampleIndex);

            // Query damage and apply effect
            float damage = disintegrationEngine->getDamageAtPosition(sampleIndex);
            if (damage > 0.001f) {
                // Scale damage by the disintegration amount parameter
                float scaledDamage = damage * (disintegrationAmount / 100.0f);
                sample = damageProcessor->processSample(sample, scaledDamage);
            }
        }

        // Apply envelope
        sample *= getEnvelopeValue();

        // Apply amplitude
        sample *= params.amplitude;

        // Apply panning and add to output
        leftOutput[i] += sample * leftGain;
        rightOutput[i] += sample * rightGain;

        // Advance position based on pitch ratio
        currentPosition += params.pitchRatio;
        samplesProcessed++;
    }

    // Store playback region for damage accumulation
    if (active || samplesProcessed >= params.sizeInSamples) {
        lastPlaybackStart = static_cast<int>(minPos);
        lastPlaybackEnd = static_cast<int>(maxPos);
    }

    return active;
}

float Grain::getEnvelopeValue() const {
    const float progress = static_cast<float>(samplesProcessed) / static_cast<float>(params.sizeInSamples);

    // Attack phase
    if (progress < params.attackRatio) {
        if (params.attackRatio <= 0.0f)
            return 1.0f;
        // Smooth attack using sine curve
        const float attackProgress = progress / params.attackRatio;
        return std::sin(attackProgress * juce::MathConstants<float>::halfPi);
    }

    // Release phase
    const float releaseStart = 1.0f - params.releaseRatio;
    if (progress > releaseStart) {
        if (params.releaseRatio <= 0.0f)
            return 1.0f;
        // Smooth release using cosine curve
        const float releaseProgress = (progress - releaseStart) / params.releaseRatio;
        return std::cos(releaseProgress * juce::MathConstants<float>::halfPi);
    }

    // Sustain phase
    return 1.0f;
}

float Grain::interpolateSample(const SampleBuffer& source, double position) const {
    // For stereo sources, mix to mono or use channel based on pan
    const int channels = source.getNumChannels();

    if (channels == 1) {
        return source.getSampleInterpolated(0, position);
    } else {
        // Mix stereo to mono
        const float left = source.getSampleInterpolated(0, position);
        const float right = source.getSampleInterpolated(1, position);
        return (left + right) * 0.5f;
    }
}

void Grain::setDamageProcessors(TapeDamageProcessor* dp, TapeDisintegrationEngine* de) {
    damageProcessor = dp;
    disintegrationEngine = de;
}

} // namespace palace
