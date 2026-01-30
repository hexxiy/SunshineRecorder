#include "WaveformDisplay.h"
#include "OccultLookAndFeel.h"
#include <vector>

namespace palace {

WaveformDisplay::WaveformDisplay() {
}

WaveformDisplay::~WaveformDisplay() = default;

void WaveformDisplay::setSampleBuffer(const SampleBuffer* buffer) {
    sampleBuffer = buffer;
    // Reset zoom when loading new sample
    zoomLevel = 1.0f;
    viewStart = 0.0f;
    invalidateWaveformCache();
    repaint();
}

void WaveformDisplay::setPosition(float normalizedPosition) {
    if (std::abs(currentPosition - normalizedPosition) < 0.0001f)
        return;

    currentPosition = normalizedPosition;

    // When zoomed in, center the view on the current position
    if (zoomLevel > 1.0f) {
        float viewWidth = 1.0f / zoomLevel;
        float newViewStart = currentPosition - viewWidth * 0.5f;

        // Clamp view start to valid range
        newViewStart = juce::jlimit(0.0f, 1.0f - viewWidth, newViewStart);

        if (std::abs(newViewStart - viewStart) > 0.001f) {
            viewStart = newViewStart;
            waveformNeedsUpdate = true;
        }
    }

    repaint();
}

void WaveformDisplay::setGrainSize(float normalizedSize) {
    currentGrainSize = normalizedSize;
    repaint();
}

void WaveformDisplay::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();

    // Draw background with subtle gradient
    juce::ColourGradient bgGradient(OccultLookAndFeel::panelDark, 0, 0,
                                    OccultLookAndFeel::backgroundDark, 0, bounds.getHeight(), false);
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Draw border
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Draw drag highlight
    if (isDraggingFile) {
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 3.0f);
        g.setColour(OccultLookAndFeel::amber);
        g.drawRoundedRectangle(bounds.reduced(2.0f), 3.0f, 2.0f);
    }

    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded()) {
        // Draw placeholder text
        g.setColour(OccultLookAndFeel::textDim);
        g.setFont(14.0f);
        g.drawText("Drop audio file here", bounds, juce::Justification::centred);

        // Draw mystical symbol as placeholder
        const float symbolSize = 40.0f;
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY() - 30.0f;

        g.setColour(OccultLookAndFeel::metalLight.withAlpha(0.3f));
        // Draw circle
        g.drawEllipse(cx - symbolSize / 2, cy - symbolSize / 2, symbolSize, symbolSize, 1.0f);
        // Draw triangle inside
        juce::Path triangle;
        for (int i = 0; i < 3; ++i) {
            const float angle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 3.0f - juce::MathConstants<float>::halfPi;
            const float px = cx + std::cos(angle) * symbolSize * 0.35f;
            const float py = cy + std::sin(angle) * symbolSize * 0.35f;
            if (i == 0)
                triangle.startNewSubPath(px, py);
            else
                triangle.lineTo(px, py);
        }
        triangle.closeSubPath();
        g.strokePath(triangle, juce::PathStrokeType(1.0f));

        return;
    }

    // Regenerate waveform if needed
    if (waveformNeedsUpdate)
        generateWaveformPath();

    // Draw waveform
    const float waveformPadding = 8.0f;
    const auto waveformBounds = bounds.reduced(waveformPadding);

    // Use cached image if available and valid
    if (cachedWaveform.isValid()) {
        g.drawImageAt(cachedWaveform, static_cast<int>(waveformBounds.getX()),
                      static_cast<int>(waveformBounds.getY()));
    }

    // Draw grain size region and position indicator
    float viewWidth = 1.0f / zoomLevel;
    float viewEnd = viewStart + viewWidth;

    // Calculate grain region in normalized coordinates
    float halfGrainSize = currentGrainSize * 0.5f;
    float grainStart = currentPosition - halfGrainSize;
    float grainEnd = currentPosition + halfGrainSize;

    // Check if grain region is at least partially visible
    if (grainEnd >= viewStart && grainStart <= viewEnd) {
        // Clamp to visible region for drawing
        float visibleStart = std::max(grainStart, viewStart);
        float visibleEnd = std::min(grainEnd, viewEnd);

        float screenStart = normalizedToScreen(visibleStart);
        float screenEnd = normalizedToScreen(visibleEnd);
        float regionWidth = screenEnd - screenStart;

        // Draw grain region glow
        g.setColour(OccultLookAndFeel::amberGlow);
        g.fillRect(screenStart, waveformBounds.getY(), regionWidth, waveformBounds.getHeight());

        // Draw grain region border
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.5f));
        g.drawRect(screenStart, waveformBounds.getY(), regionWidth, waveformBounds.getHeight(), 1.0f);

        // Draw edge markers if visible
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.7f));
        if (grainStart >= viewStart && grainStart <= viewEnd) {
            float edgeX = normalizedToScreen(grainStart);
            g.drawLine(edgeX, waveformBounds.getY(), edgeX, waveformBounds.getBottom(), 1.0f);
        }
        if (grainEnd >= viewStart && grainEnd <= viewEnd) {
            float edgeX = normalizedToScreen(grainEnd);
            g.drawLine(edgeX, waveformBounds.getY(), edgeX, waveformBounds.getBottom(), 1.0f);
        }
    }

    // Draw center position line if in view
    if (currentPosition >= viewStart && currentPosition <= viewEnd) {
        const float posX = normalizedToScreen(currentPosition);

        // Draw position line
        g.setColour(OccultLookAndFeel::amber);
        g.drawLine(posX, waveformBounds.getY(), posX, waveformBounds.getBottom(), 2.0f);

        // Draw position triangle marker at top
        juce::Path marker;
        marker.addTriangle(posX - 5.0f, bounds.getY() + 2.0f,
                           posX + 5.0f, bounds.getY() + 2.0f,
                           posX, bounds.getY() + 10.0f);
        g.fillPath(marker);
    }

    // Draw zoom indicator if zoomed in
    if (zoomLevel > 1.0f) {
        g.setColour(OccultLookAndFeel::textDim);
        g.setFont(10.0f);
        juce::String zoomText = juce::String(zoomLevel, 1) + "x";
        g.drawText(zoomText, waveformBounds.getRight() - 35, waveformBounds.getY() + 2, 30, 12,
                   juce::Justification::centredRight);
    }
}

void WaveformDisplay::resized() {
    // Only invalidate if we have valid bounds and size changed
    if (getWidth() > 0 && getHeight() > 0) {
        invalidateWaveformCache();
    }
}

float WaveformDisplay::screenToNormalized(float screenX) const {
    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    float viewWidth = 1.0f / zoomLevel;
    float normalizedX = (screenX - bounds.getX()) / bounds.getWidth();
    return viewStart + normalizedX * viewWidth;
}

float WaveformDisplay::normalizedToScreen(float normalized) const {
    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    float viewWidth = 1.0f / zoomLevel;
    float screenNormalized = (normalized - viewStart) / viewWidth;
    return bounds.getX() + screenNormalized * bounds.getWidth();
}

void WaveformDisplay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded())
        return;

    // Get the normalized position under the mouse before zooming
    float mouseNormalized = screenToNormalized(static_cast<float>(event.x));

    // Adjust zoom level
    float zoomFactor = 1.0f + wheel.deltaY * 0.5f;
    float newZoom = juce::jlimit(1.0f, 50.0f, zoomLevel * zoomFactor);

    if (newZoom != zoomLevel) {
        // Calculate new view start to keep mouse position stationary
        float viewWidth = 1.0f / newZoom;
        float mouseRatio = (static_cast<float>(event.x) - getLocalBounds().toFloat().reduced(8.0f).getX())
                          / getLocalBounds().toFloat().reduced(8.0f).getWidth();
        viewStart = mouseNormalized - mouseRatio * viewWidth;

        // Clamp view start
        viewStart = juce::jlimit(0.0f, 1.0f - viewWidth, viewStart);

        zoomLevel = newZoom;
        waveformNeedsUpdate = true;
        repaint();
    }
}

void WaveformDisplay::mouseDown(const juce::MouseEvent& event) {
    if (sampleBuffer != nullptr && sampleBuffer->isLoaded()) {
        lastDragX = static_cast<float>(event.x);
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event) {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded() || zoomLevel <= 1.0f)
        return;

    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    float deltaX = lastDragX - static_cast<float>(event.x);
    float viewWidth = 1.0f / zoomLevel;
    float deltaNormalized = deltaX / bounds.getWidth() * viewWidth;

    viewStart = juce::jlimit(0.0f, 1.0f - viewWidth, viewStart + deltaNormalized);
    lastDragX = static_cast<float>(event.x);

    waveformNeedsUpdate = true;
    repaint();
}

void WaveformDisplay::mouseDoubleClick(const juce::MouseEvent&) {
    resetZoom();
}

void WaveformDisplay::zoomIn() {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded())
        return;

    float newZoom = juce::jlimit(1.0f, 50.0f, zoomLevel * 1.5f);

    if (newZoom != zoomLevel) {
        // Keep view centered on current position
        float viewWidth = 1.0f / newZoom;
        viewStart = currentPosition - viewWidth * 0.5f;
        viewStart = juce::jlimit(0.0f, 1.0f - viewWidth, viewStart);

        zoomLevel = newZoom;
        waveformNeedsUpdate = true;
        repaint();
    }
}

void WaveformDisplay::zoomOut() {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded())
        return;

    float newZoom = juce::jlimit(1.0f, 50.0f, zoomLevel / 1.5f);

    if (newZoom != zoomLevel) {
        float viewWidth = 1.0f / newZoom;
        viewStart = currentPosition - viewWidth * 0.5f;
        viewStart = juce::jlimit(0.0f, 1.0f - viewWidth, viewStart);

        zoomLevel = newZoom;
        waveformNeedsUpdate = true;
        repaint();
    }
}

void WaveformDisplay::resetZoom() {
    zoomLevel = 1.0f;
    viewStart = 0.0f;
    invalidateWaveformCache();
    repaint();
}

void WaveformDisplay::generateWaveformPath() {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded()) {
        waveformNeedsUpdate = false;
        return;
    }

    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    const int width = static_cast<int>(bounds.getWidth());
    const int height = static_cast<int>(bounds.getHeight());

    // Check if we actually need to regenerate
    if (!waveformNeedsUpdate &&
        lastViewStart == viewStart &&
        lastZoomLevel == zoomLevel &&
        lastWidth == width &&
        lastHeight == height &&
        cachedWaveform.isValid()) {
        return;
    }

    const int totalSamples = sampleBuffer->getNumSamples();
    if (width <= 0 || height <= 0 || totalSamples <= 0) {
        waveformNeedsUpdate = false;
        return;
    }

    // Calculate visible sample range based on zoom/pan
    float viewWidth = 1.0f / zoomLevel;
    int startSampleIdx = static_cast<int>(viewStart * totalSamples);
    int endSampleIdx = static_cast<int>((viewStart + viewWidth) * totalSamples);
    endSampleIdx = std::min(endSampleIdx, totalSamples);

    int visibleSamples = endSampleIdx - startSampleIdx;
    if (visibleSamples <= 0) {
        waveformNeedsUpdate = false;
        return;
    }

    // Create or resize cached image
    if (!cachedWaveform.isValid() ||
        cachedWaveform.getWidth() != width ||
        cachedWaveform.getHeight() != height) {
        cachedWaveform = juce::Image(juce::Image::ARGB, width, height, true);
    }

    // Clear the image
    cachedWaveform.clear(cachedWaveform.getBounds());

    // Calculate samples per pixel for visible region
    const double samplesPerPixel = static_cast<double>(visibleSamples) / width;
    const auto& buffer = sampleBuffer->getBuffer();
    const int numChannels = buffer.getNumChannels();
    const float centerY = height * 0.5f;
    const float amplitude = height * 0.45f;

    // Build optimized path with single pass
    waveformPath.clear();
    waveformPath.preallocateSpace(width * 3);

    std::vector<float> topPoints(width);
    std::vector<float> bottomPoints(width);

    // Single pass to calculate both top and bottom
    for (int x = 0; x < width; ++x) {
        const int localStart = static_cast<int>(x * samplesPerPixel);
        const int localEnd = static_cast<int>((x + 1) * samplesPerPixel);

        float maxVal = 0.0f;

        // Unroll channel loop for better performance when we have 1 or 2 channels
        if (numChannels == 1) {
            for (int s = localStart; s < localEnd; ++s) {
                int sampleIdx = startSampleIdx + s;
                if (sampleIdx >= 0 && sampleIdx < totalSamples) {
                    maxVal = std::max(maxVal, std::abs(buffer.getSample(0, sampleIdx)));
                }
            }
        } else {
            for (int s = localStart; s < localEnd; ++s) {
                int sampleIdx = startSampleIdx + s;
                if (sampleIdx >= 0 && sampleIdx < totalSamples) {
                    for (int ch = 0; ch < numChannels; ++ch) {
                        maxVal = std::max(maxVal, std::abs(buffer.getSample(ch, sampleIdx)));
                    }
                }
            }
        }

        topPoints[x] = centerY - maxVal * amplitude;
        bottomPoints[x] = centerY + maxVal * amplitude;
    }

    // Build path from pre-calculated points
    waveformPath.startNewSubPath(0.0f, topPoints[0]);
    for (int x = 1; x < width; ++x) {
        waveformPath.lineTo(static_cast<float>(x), topPoints[x]);
    }
    for (int x = width - 1; x >= 0; --x) {
        waveformPath.lineTo(static_cast<float>(x), bottomPoints[x]);
    }
    waveformPath.closeSubPath();

    // Render to cached image
    juce::Graphics g(cachedWaveform);

    // Draw waveform shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.strokePath(waveformPath, juce::PathStrokeType(2.0f),
                 juce::AffineTransform::translation(1.0f, 1.0f));

    // Draw main waveform
    g.setColour(OccultLookAndFeel::textLight.withAlpha(0.7f));
    g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

    // Update cache state
    lastViewStart = viewStart;
    lastZoomLevel = zoomLevel;
    lastWidth = width;
    lastHeight = height;
    waveformNeedsUpdate = false;
}

void WaveformDisplay::invalidateWaveformCache() {
    waveformNeedsUpdate = true;
    cachedWaveform = juce::Image();
    lastViewStart = -1.0f;
    lastZoomLevel = -1.0f;
    lastWidth = -1;
    lastHeight = -1;
}

bool WaveformDisplay::isInterestedInFileDrag(const juce::StringArray& files) {
    for (const auto& file : files) {
        if (file.endsWithIgnoreCase(".wav") ||
            file.endsWithIgnoreCase(".aiff") ||
            file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".mp3") ||
            file.endsWithIgnoreCase(".flac") ||
            file.endsWithIgnoreCase(".ogg")) {
            return true;
        }
    }
    return false;
}

void WaveformDisplay::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/) {
    isDraggingFile = false;
    repaint();

    for (const auto& filePath : files) {
        juce::File file(filePath);
        if (file.existsAsFile() && onFileDropped) {
            onFileDropped(file);
            break;
        }
    }
}

void WaveformDisplay::fileDragEnter(const juce::StringArray& /*files*/, int /*x*/, int /*y*/) {
    isDraggingFile = true;
    repaint();
}

void WaveformDisplay::fileDragExit(const juce::StringArray& /*files*/) {
    isDraggingFile = false;
    repaint();
}

} // namespace palace
