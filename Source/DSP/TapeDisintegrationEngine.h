#pragma once

#include <array>
#include <vector>
#include <atomic>
#include <algorithm>

namespace palace {

class TapeDisintegrationEngine
{
public:
    static constexpr int NUM_REGIONS = 512;

    TapeDisintegrationEngine();
    ~TapeDisintegrationEngine() = default;

    // Initialization
    void prepare(double sampleRate, int totalSamples);
    void reset();

    // Real-time life tracking (called from audio thread per-sample)
    void decrementLife(int sampleIndex);
    void resetAllLife();

    // Thread-safe damage queries
    float getDamageAtPosition(int sampleIndex) const;
    std::vector<float> getLifeMap() const;

    // Parameter control
    void setMaxLife(float hits);
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // State management
    void setRegionLife(int regionIndex, float life);

private:
    struct DamageRegion
    {
        std::atomic<float> lifeRemaining{1.0f};    // 0-1 normalized (1.0 = full life)
        std::atomic<int> totalHits{0};             // Diagnostic counter
    };

    std::array<DamageRegion, NUM_REGIONS> regions;

    std::atomic<double> sampleRate{44100.0};
    std::atomic<int> totalSamples{0};
    std::atomic<float> maxLifeHits{1000.0f};
    std::atomic<bool> enabled{false};

    // Helper methods
    int positionToRegion(int sampleIndex) const;
};

} // namespace palace
