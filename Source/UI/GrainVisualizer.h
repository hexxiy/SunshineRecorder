#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/GrainEngine.h"
#include <vector>

namespace palace {

class GrainVisualizer : public juce::Component,
                        public juce::Timer {
public:
    GrainVisualizer();
    ~GrainVisualizer() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

    void setActiveGrainCount(int count);
    void setPosition(float normalizedPosition);
    void setSpray(float spray);

private:
    struct VisualGrain {
        float x;
        float y;
        float size;
        float alpha;
        float velocity;
    };

    std::vector<VisualGrain> visualGrains;
    int activeGrainCount = 0;
    float position = 0.0f;
    float spray = 0.0f;
    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainVisualizer)
};

} // namespace palace
