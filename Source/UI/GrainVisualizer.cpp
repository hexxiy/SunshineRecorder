#include "GrainVisualizer.h"
#include "OccultLookAndFeel.h"

namespace palace {

GrainVisualizer::GrainVisualizer() {
    startTimerHz(30);  // 30 FPS animation
}

GrainVisualizer::~GrainVisualizer() {
    stopTimer();
}

void GrainVisualizer::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();

    // Draw background
    g.setColour(OccultLookAndFeel::backgroundDark);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Draw border
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Draw center position line
    const float posX = bounds.getX() + position * bounds.getWidth();
    g.setColour(OccultLookAndFeel::amberDim.withAlpha(0.3f));
    g.drawLine(posX, bounds.getY(), posX, bounds.getBottom(), 1.0f);

    // Draw spray range
    if (spray > 0.0f) {
        const float sprayWidth = spray * bounds.getWidth() * 0.5f;
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.1f));
        g.fillRect(posX - sprayWidth, bounds.getY(), sprayWidth * 2.0f, bounds.getHeight());
    }

    // Draw grains as glowing particles
    for (const auto& grain : visualGrains) {
        const float x = bounds.getX() + grain.x * bounds.getWidth();
        const float y = bounds.getY() + grain.y * bounds.getHeight();

        // Draw glow
        g.setColour(OccultLookAndFeel::amber.withAlpha(grain.alpha * 0.3f));
        g.fillEllipse(x - grain.size * 1.5f, y - grain.size * 1.5f,
                      grain.size * 3.0f, grain.size * 3.0f);

        // Draw core
        g.setColour(OccultLookAndFeel::amber.withAlpha(grain.alpha));
        g.fillEllipse(x - grain.size * 0.5f, y - grain.size * 0.5f,
                      grain.size, grain.size);
    }

    // Draw grain count
    g.setColour(OccultLookAndFeel::textDim);
    g.setFont(10.0f);
    g.drawText(juce::String(activeGrainCount) + " grains",
               bounds.reduced(4.0f), juce::Justification::bottomRight);
}

void GrainVisualizer::timerCallback() {
    // Update existing grains
    for (auto it = visualGrains.begin(); it != visualGrains.end();) {
        it->y -= it->velocity * 0.02f;
        it->alpha -= 0.03f;

        if (it->alpha <= 0.0f || it->y < 0.0f) {
            it = visualGrains.erase(it);
        } else {
            ++it;
        }
    }

    // Spawn new grains based on count
    const int targetGrains = std::min(activeGrainCount, 50);  // Cap visual grains
    while (static_cast<int>(visualGrains.size()) < targetGrains) {
        VisualGrain grain;
        grain.x = position + (random.nextFloat() * 2.0f - 1.0f) * spray * 0.5f;
        grain.x = std::clamp(grain.x, 0.0f, 1.0f);
        grain.y = 0.8f + random.nextFloat() * 0.2f;
        grain.size = 3.0f + random.nextFloat() * 4.0f;
        grain.alpha = 0.5f + random.nextFloat() * 0.5f;
        grain.velocity = 0.3f + random.nextFloat() * 0.4f;
        visualGrains.push_back(grain);
    }

    repaint();
}

void GrainVisualizer::setActiveGrainCount(int count) {
    activeGrainCount = count;
}

void GrainVisualizer::setPosition(float normalizedPosition) {
    position = normalizedPosition;
}

void GrainVisualizer::setSpray(float sprayAmount) {
    spray = sprayAmount;
}

} // namespace palace
