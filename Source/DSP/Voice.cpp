#include "Voice.h"
#include "TapeDisintegrationEngine.h"
#include <cmath>

namespace palace {

Voice::Voice() = default;

void Voice::prepare(double sr, int samplesPerBlock) {
    sampleRate = sr;
    grainEngine.prepare(sr, samplesPerBlock);

    // Pre-allocate temporary buffers to avoid allocation in audio thread
    tempBufferLeft.resize(samplesPerBlock);
    tempBufferRight.resize(samplesPerBlock);

    reset();
}

void Voice::reset() {
    active = false;
    releasing = false;
    currentNote = -1;
    envelopeStage = EnvelopeStage::Idle;
    envelopeValue = 0.0f;
    age = 0;
    grainEngine.reset();
}

void Voice::noteOn(int midiNote, float vel) {
    currentNote = midiNote;
    velocity = vel;
    active = true;
    releasing = false;
    age = 0;

    // Calculate pitch ratio based on MIDI note
    // Each semitone is a factor of 2^(1/12)
    const int semitoneOffset = midiNote - BASE_NOTE;
    noteRatio = std::pow(2.0f, semitoneOffset / 12.0f);

    // Start envelope
    envelopeStage = EnvelopeStage::Attack;
    envelopeTarget = 1.0f;

    grainEngine.reset();
}

void Voice::noteOff() {
    if (!active)
        return;

    releasing = true;
    envelopeStage = EnvelopeStage::Release;
    envelopeTarget = 0.0f;
}

void Voice::process(const SampleBuffer& source,
                    float* leftOutput,
                    float* rightOutput,
                    int numSamples) {
    if (!active)
        return;

    // Update envelope
    updateEnvelope(numSamples);

    // Check if voice has finished
    if (envelopeStage == EnvelopeStage::Idle) {
        reset();
        return;
    }

    // Process grain engine using pre-allocated buffers
    std::fill(tempBufferLeft.begin(), tempBufferLeft.begin() + numSamples, 0.0f);
    std::fill(tempBufferRight.begin(), tempBufferRight.begin() + numSamples, 0.0f);

    grainEngine.process(source, tempBufferLeft.data(), tempBufferRight.data(), numSamples, noteRatio);

    // Apply envelope and velocity, add to output
    const float gain = envelopeValue * velocity;
    for (int i = 0; i < numSamples; ++i) {
        leftOutput[i] += tempBufferLeft[i] * gain;
        rightOutput[i] += tempBufferRight[i] * gain;
    }
}

void Voice::setGrainParameters(const GrainEngineParameters& params) {
    grainEngine.setParameters(params);
}

void Voice::setADSR(float attackMs, float decayMs, float sustain, float releaseMs) {
    // Convert ms to rate per sample
    // Rate = 1.0 / (time_in_seconds * sample_rate)
    attackRate = attackMs > 0.0f ? 1.0f / (attackMs * 0.001f * static_cast<float>(sampleRate)) : 1.0f;
    decayRate = decayMs > 0.0f ? 1.0f / (decayMs * 0.001f * static_cast<float>(sampleRate)) : 1.0f;
    sustainLevel = sustain / 100.0f;  // Convert from percentage
    releaseRate = releaseMs > 0.0f ? 1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)) : 1.0f;
}

void Voice::updateEnvelope(int numSamples) {
    // Per-sample ADSR envelope generation with linear ramps
    // Attack/Decay/Release use constant rate per sample
    for (int i = 0; i < numSamples; ++i) {
        switch (envelopeStage) {
            case EnvelopeStage::Attack:
                envelopeValue += attackRate;
                if (envelopeValue >= 1.0f) {
                    envelopeValue = 1.0f;
                    envelopeStage = EnvelopeStage::Decay;
                    envelopeTarget = sustainLevel;
                }
                break;

            case EnvelopeStage::Decay:
                envelopeValue -= decayRate;
                if (envelopeValue <= sustainLevel) {
                    envelopeValue = sustainLevel;
                    envelopeStage = EnvelopeStage::Sustain;
                }
                break;

            case EnvelopeStage::Sustain:
                // Hold at sustain level
                envelopeValue = sustainLevel;
                break;

            case EnvelopeStage::Release:
                envelopeValue -= releaseRate;
                if (envelopeValue <= 0.0f) {
                    envelopeValue = 0.0f;
                    envelopeStage = EnvelopeStage::Idle;
                    return;  // Voice finished
                }
                break;

            case EnvelopeStage::Idle:
                return;
        }
    }
}

void Voice::setDisintegrationEngine(TapeDisintegrationEngine* de) {
    grainEngine.setDisintegrationEngine(de);
}

void Voice::setDisintegrationAmount(float amount) {
    grainEngine.setDisintegrationAmount(amount);
}

} // namespace palace
