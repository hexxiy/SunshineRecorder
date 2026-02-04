#include "GrainEngine.h"
#include "TapeDisintegrationEngine.h"
#include <cmath>

namespace palace {

GrainEngine::GrainEngine() {
    std::random_device rd;
    rng.seed(rd());
}

void GrainEngine::prepare(double sr, int blockSize) {
    sampleRate = sr;
    samplesPerBlock = blockSize;

    // Initialize damage processors
    for (auto& processor : damageProcessors) {
        processor.prepare(sr);
    }

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
    info.reserve(MAX_GRAINS);

    for (const auto& grain : grains) {
        if (grain.isActive()) {
            const auto& gParams = grain.getParameters();

            GrainInfo gInfo;
            gInfo.position = static_cast<float>(gParams.startPosition);  // Absolute sample position
            gInfo.progress = grain.getProgress();
            gInfo.pan = gParams.pan;
            gInfo.sizeInSamples = gParams.sizeInSamples;

            info.push_back(gInfo);
        }
    }

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

    // Calculate position with spray, clamped to crop region
    float positionNorm = params.position;
    if (params.spray > 0.0f) {
        const float sprayAmount = dist(rng) * params.spray;
        positionNorm += sprayAmount;
    }
    positionNorm = std::clamp(positionNorm, params.cropStart, params.cropEnd);
    grainParams.startPosition = static_cast<int>(positionNorm * (sourceSamples - 1));

    // Calculate grain size in samples
    grainParams.sizeInSamples = static_cast<int>(params.grainSizeMs * 0.001 * sampleRate);
    grainParams.sizeInSamples = std::max(grainParams.sizeInSamples, 64);  // Minimum size

    // Clamp grain size so reading window stays within crop region
    const int cropEndSample = static_cast<int>(params.cropEnd * (sourceSamples - 1));
    const float pitchRatio = std::pow(2.0f, params.pitchSemitones / 12.0f) * noteRatio;
    if (pitchRatio > 0.0f) {
        const int maxReadSamples = static_cast<int>((cropEndSample - grainParams.startPosition) / pitchRatio);
        grainParams.sizeInSamples = std::min(grainParams.sizeInSamples, std::max(maxReadSamples, 64));
    }

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
    grainParams.sampleGainDb = params.sampleGainDb;

    // Set up damage processors for this grain
    grains[freeIndex].setDamageProcessors(&damageProcessors[freeIndex], disintegrationEngine);
    grains[freeIndex].setDisintegrationAmount(disintegrationAmount);

    grains[freeIndex].start(grainParams);
}

int GrainEngine::findFreeGrain() const {
    for (int i = 0; i < MAX_GRAINS; ++i) {
        if (!grains[i].isActive())
            return i;
    }
    return -1;  // All grains in use
}

void GrainEngine::setDisintegrationEngine(TapeDisintegrationEngine* de) {
    disintegrationEngine = de;
}

void GrainEngine::setDisintegrationAmount(float amount) {
    disintegrationAmount = amount;
}

std::vector<GrainEngine::PlaybackRegion> GrainEngine::getActivePlaybackRegions() const {
    std::vector<PlaybackRegion> regions;
    regions.reserve(MAX_GRAINS);

    for (const auto& grain : grains) {
        if (grain.isActive()) {
            PlaybackRegion region;
            region.startSample = grain.getLastPlaybackStart();
            region.endSample = grain.getLastPlaybackEnd();
            if (region.startSample >= 0 && region.endSample >= region.startSample) {
                regions.push_back(region);
            }
        }
    }

    return regions;
}

} // namespace palace
