#include "TapeDisintegrationEngine.h"
#include <cmath>
#include <algorithm>

namespace palace {

TapeDisintegrationEngine::TapeDisintegrationEngine()
{
    reset();
}

void TapeDisintegrationEngine::prepare(double sr, int samples)
{
    sampleRate.store(sr);
    totalSamples.store(samples);
}

void TapeDisintegrationEngine::reset()
{
    for (auto& region : regions)
    {
        region.lifeRemaining.store(1.0f);
        region.totalHits.store(0);
    }
}

void TapeDisintegrationEngine::decrementLife(int sampleIndex)
{
    if (!enabled.load() || totalSamples.load() == 0)
        return;

    int regionIndex = positionToRegion(sampleIndex);
    if (regionIndex < 0 || regionIndex >= NUM_REGIONS)
        return;

    auto& region = regions[regionIndex];

    // Calculate decrement amount (1 hit = 1/maxLife)
    float maxLife = maxLifeHits.load();
    float decrement = 1.0f / maxLife;

    // Atomic decrement with clamping
    float currentLife = region.lifeRemaining.load();
    float newLife = std::max(0.0f, currentLife - decrement);
    region.lifeRemaining.store(newLife);

    // Increment hit counter
    region.totalHits.fetch_add(1);
}

void TapeDisintegrationEngine::resetAllLife()
{
    for (auto& region : regions)
    {
        region.lifeRemaining.store(1.0f);
        region.totalHits.store(0);
    }
}

float TapeDisintegrationEngine::getDamageAtPosition(int sampleIndex) const
{
    if (!enabled.load() || totalSamples.load() == 0)
        return 0.0f;

    const int regionIndex = positionToRegion(sampleIndex);
    if (regionIndex < 0 || regionIndex >= NUM_REGIONS)
        return 0.0f;

    float life = regions[regionIndex].lifeRemaining.load();

    // Damage = inverse of remaining life
    return 1.0f - life;  // 1.0 = full damage, 0.0 = no damage
}

std::vector<float> TapeDisintegrationEngine::getLifeMap() const
{
    std::vector<float> lifeMap;
    lifeMap.reserve(NUM_REGIONS);

    for (const auto& region : regions)
    {
        lifeMap.push_back(region.lifeRemaining.load());
    }

    return lifeMap;
}

void TapeDisintegrationEngine::setMaxLife(float hits)
{
    maxLifeHits.store(std::clamp(hits, 25.0f, 1000000.0f));
}

void TapeDisintegrationEngine::setEnabled(bool shouldBeEnabled)
{
    enabled.store(shouldBeEnabled);
}

bool TapeDisintegrationEngine::isEnabled() const
{
    return enabled.load();
}

void TapeDisintegrationEngine::setRegionLife(int regionIndex, float life)
{
    if (regionIndex >= 0 && regionIndex < NUM_REGIONS)
    {
        regions[regionIndex].lifeRemaining.store(std::clamp(life, 0.0f, 1.0f));
    }
}

int TapeDisintegrationEngine::positionToRegion(int sampleIndex) const
{
    const int samples = totalSamples.load();
    if (samples == 0)
        return 0;

    return std::clamp((sampleIndex * NUM_REGIONS) / samples, 0, NUM_REGIONS - 1);
}

} // namespace palace
