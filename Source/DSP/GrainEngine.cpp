#include "GrainEngine.h"
#include <cmath>

namespace palace {

GrainEngine::GrainEngine() {
    std::random_device rd;
    rng.seed(rd());
}

void GrainEngine::prepare(double sr, int blockSize) {
    sampleRate = sr;
    samplesPerBlock = blockSize;
    reset();
}

void GrainEngine::reset() {
    for (auto& grain : grains) {
        grain.stop();
    }
    samplesSinceLastGrain = 0.0;
}

void GrainEngine::process(const SampleBuffer& source,
                          float* leftOutput,
                          float* rightOutput,
                          int numSamples,
                          float noteRatio) {
    if (!source.isLoaded())
        return;

    // Clear output buffers
    std::fill(leftOutput, leftOutput + numSamples, 0.0f);
    std::fill(rightOutput, rightOutput + numSamples, 0.0f);

    // Calculate samples between grain triggers
    samplesPerGrain = sampleRate / std::max(params.density, 0.1f);

    // Process sample by sample for grain scheduling
    for (int i = 0; i < numSamples; ++i) {
        // Check if we should trigger a new grain
        samplesSinceLastGrain++;
        if (samplesSinceLastGrain >= samplesPerGrain) {
            triggerGrain(source, noteRatio);
            samplesSinceLastGrain = 0.0;
        }
    }

    // Process all active grains
    for (auto& grain : grains) {
        if (grain.isActive()) {
            grain.process(source, leftOutput, rightOutput, numSamples);
        }
    }
}

void GrainEngine::setParameters(const GrainEngineParameters& p) {
    params = p;
}

int GrainEngine::getActiveGrainCount() const {
    int count = 0;
    for (const auto& grain : grains) {
        if (grain.isActive())
            count++;
    }
    return count;
}

std::vector<GrainEngine::GrainInfo> GrainEngine::getActiveGrainInfo() const {
    std::vector<GrainInfo> info;
    // Simplified - would need to expose grain state for full implementation
    return info;
}

void GrainEngine::triggerGrain(const SampleBuffer& source, float noteRatio) {
    const int freeIndex = findFreeGrain();
    if (freeIndex < 0)
        return;

    const int sourceSamples = source.getNumSamples();
    if (sourceSamples == 0)
        return;

    GrainParameters grainParams;

    // Calculate position with spray
    float positionNorm = params.position;
    if (params.spray > 0.0f) {
        const float sprayAmount = dist(rng) * params.spray;
        positionNorm = std::clamp(positionNorm + sprayAmount, 0.0f, 1.0f);
    }
    grainParams.startPosition = static_cast<int>(positionNorm * (sourceSamples - 1));

    // Calculate grain size in samples
    grainParams.sizeInSamples = static_cast<int>(params.grainSizeMs * 0.001 * sampleRate);
    grainParams.sizeInSamples = std::max(grainParams.sizeInSamples, 64);  // Minimum size

    // Calculate pitch ratio from semitones + MIDI note
    const float semitonePitch = std::pow(2.0f, params.pitchSemitones / 12.0f);
    grainParams.pitchRatio = semitonePitch * noteRatio;

    // Calculate pan with spread
    if (params.panSpread > 0.0f) {
        grainParams.pan = dist(rng) * params.panSpread;
    } else {
        grainParams.pan = 0.0f;
    }

    grainParams.amplitude = 1.0f;
    grainParams.attackRatio = params.attackRatio;
    grainParams.releaseRatio = params.releaseRatio;

    grains[freeIndex].start(grainParams);
}

int GrainEngine::findFreeGrain() const {
    for (int i = 0; i < MAX_GRAINS; ++i) {
        if (!grains[i].isActive())
            return i;
    }
    return -1;  // All grains in use
}

} // namespace palace
