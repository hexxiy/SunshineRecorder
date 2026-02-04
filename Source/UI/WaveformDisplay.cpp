#include "WaveformDisplay.h"
#include "OccultLookAndFeel.h"
#include "../DSP/TapeDisintegrationEngine.h"
#include <vector>

namespace palace {

WaveformDisplay::WaveformDisplay() {
}

WaveformDisplay::~WaveformDisplay() = default;

void WaveformDisplay::setSampleBuffer(const SampleBuffer* buffer) {
    sampleBuffer = buffer;
    // Reset zoom and crop when loading new sample
    zoomLevel = 1.0f;
    viewStart = 0.0f;
    cropStart = 0.0f;
    cropEnd = 1.0f;
    currentDrag = DragTarget::None;
    invalidateWaveformCache();
    repaint();
}

void WaveformDisplay::setCropRegion(float start, float end) {
    cropStart = juce::jlimit(0.0f, 1.0f, start);
    cropEnd = juce::jlimit(0.0f, 1.0f, end);
    repaint();
}

void WaveformDisplay::resetCrop() {
    cropStart = 0.0f;
    cropEnd = 1.0f;
    if (onCropChanged)
        onCropChanged(cropStart, cropEnd);
    repaint();
}

bool WaveformDisplay::isNearCropHandle(float screenX, float handleNormalized, float tolerance) const {
    float handleScreen = normalizedToScreen(handleNormalized);
    return std::abs(screenX - handleScreen) <= tolerance;
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

void WaveformDisplay::setDisintegrationEngine(const TapeDisintegrationEngine* de) {
    disintegrationEngine = de;
    repaint();
}

void WaveformDisplay::setSampleGain(float gainDb) {
    if (sampleGainDb != gainDb) {
        sampleGainDb = gainDb;
        invalidateWaveformCache();
        repaint();
    }
}

void WaveformDisplay::setActiveGrains(const std::vector<GrainEngine::GrainInfo>& grains, int totalSamples) {
    activeGrains = grains;
    totalSampleCount = totalSamples;
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

    // Draw dimmed overlay outside crop region
    if (cropStart > 0.0f || cropEnd < 1.0f) {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        if (cropStart > viewStart) {
            float cropStartScreen = normalizedToScreen(cropStart);
            g.fillRect(waveformBounds.getX(), waveformBounds.getY(),
                       cropStartScreen - waveformBounds.getX(), waveformBounds.getHeight());
        }
        float viewEnd = viewStart + 1.0f / zoomLevel;
        if (cropEnd < viewEnd) {
            float cropEndScreen = normalizedToScreen(cropEnd);
            g.fillRect(cropEndScreen, waveformBounds.getY(),
                       waveformBounds.getRight() - cropEndScreen, waveformBounds.getHeight());
        }
    }

    // Draw active grains visualization
    if (!activeGrains.empty() && totalSampleCount > 0) {
        const float viewEnd = viewStart + 1.0f / zoomLevel;

        for (const auto& grain : activeGrains) {
            // Convert grain position to normalized (0-1)
            float grainPosNorm = grain.position / totalSampleCount;
            float grainSizeNorm = static_cast<float>(grain.sizeInSamples) / totalSampleCount;

            // Calculate grain region bounds
            float grainStart = grainPosNorm;
            float grainEnd = grainStart + grainSizeNorm;

            // Check if grain is visible in current view
            if (grainEnd < viewStart || grainStart > viewEnd) continue;

            // Clamp to visible portion
            float visibleStart = std::max(grainStart, viewStart);
            float visibleEnd = std::min(grainEnd, viewEnd);

            // Convert to screen coordinates
            float screenX = normalizedToScreen(visibleStart);
            float screenWidth = normalizedToScreen(visibleEnd) - screenX;

            // Color based on progress (fade out as grain completes)
            float alpha = (1.0f - grain.progress) * 0.35f;  // Fade from 35% to 0%

            // Different colors for left/right panned grains
            juce::Colour grainColor;
            if (grain.pan < -0.1f) {
                // Left-panned grains are blue-ish
                grainColor = juce::Colour(0x40, 0x80, 0xFF).withAlpha(alpha);
            } else if (grain.pan > 0.1f) {
                // Right-panned grains are orange-ish
                grainColor = juce::Colour(0xFF, 0x80, 0x40).withAlpha(alpha);
            } else {
                // Center grains are amber
                grainColor = OccultLookAndFeel::amber.withAlpha(alpha);
            }

            // Draw grain region
            g.setColour(grainColor);
            g.fillRect(screenX, waveformBounds.getY(), screenWidth, waveformBounds.getHeight());

            // Draw grain edge markers (stronger at attack, fade during release)
            if (grain.progress < 0.2f) {
                // Attack phase - bright edge
                g.setColour(grainColor.brighter(0.5f).withAlpha(alpha * 2.0f));
                g.drawLine(screenX, waveformBounds.getY(), screenX, waveformBounds.getBottom(), 1.5f);
            }
        }
    }

    // Draw tape disintegration life depletion overlay (always show if damage exists)
    if (disintegrationEngine && sampleBuffer) {
        auto lifeMap = disintegrationEngine->getLifeMap();
        const int numRegions = static_cast<int>(lifeMap.size());
        const float viewEnd = viewStart + 1.0f / zoomLevel;

        for (int i = 0; i < numRegions; ++i) {
            float life = lifeMap[i];
            float damage = 1.0f - life;  // Convert life to damage
            if (damage < 0.001f) continue;  // Only skip if essentially no damage

            // Calculate normalized position in ABSOLUTE sample space (0-1 over full sample)
            float regionStart = static_cast<float>(i) / numRegions;
            float regionEnd = static_cast<float>(i + 1) / numRegions;

            // Check if region is visible in current view
            if (regionEnd < viewStart || regionStart > viewEnd) continue;

            // Clamp to visible portion
            float visibleStart = std::max(regionStart, viewStart);
            float visibleEnd = std::min(regionEnd, viewEnd);

            // Convert to screen coordinates
            float screenX = normalizedToScreen(visibleStart);
            float screenWidth = normalizedToScreen(visibleEnd) - screenX;

            // Draw life depletion overlay (red with opacity based on damage)
            // 0% life = full red (100% opacity), 50% life = 50% opacity, 100% life = no overlay
            juce::Colour damageColor = juce::Colour(0xFF, 0x00, 0x00).withAlpha(damage * 0.8f);
            g.setColour(damageColor);
            g.fillRect(screenX, waveformBounds.getY(), screenWidth, waveformBounds.getHeight());
        }
    }

    // Draw crop handles
    {
        const float handleWidth = 3.0f;
        auto drawHandle = [&](float normalized, bool isActive) {
            float viewEnd = viewStart + 1.0f / zoomLevel;
            if (normalized < viewStart || normalized > viewEnd)
                return;
            float screenX = normalizedToScreen(normalized);
            // Handle bar
            g.setColour(isActive ? OccultLookAndFeel::amber : OccultLookAndFeel::textLight);
            g.fillRect(screenX - handleWidth * 0.5f, waveformBounds.getY(),
                       handleWidth, waveformBounds.getHeight());
            // Grab tabs at top and bottom
            float tabWidth = 8.0f;
            float tabHeight = 12.0f;
            g.fillRoundedRectangle(screenX - tabWidth * 0.5f, waveformBounds.getY(),
                                   tabWidth, tabHeight, 2.0f);
            g.fillRoundedRectangle(screenX - tabWidth * 0.5f, waveformBounds.getBottom() - tabHeight,
                                   tabWidth, tabHeight, 2.0f);
        };

        bool startActive = (currentDrag == DragTarget::CropStart);
        bool endActive = (currentDrag == DragTarget::CropEnd);
        drawHandle(cropStart, startActive);
        drawHandle(cropEnd, endActive);
    }

    // Draw grain size region and position indicator
    float viewWidth = 1.0f / zoomLevel;
    float viewEnd = viewStart + viewWidth;

    // Calculate grain region in normalized coordinates, clamped to crop bounds
    float halfGrainSize = currentGrainSize * 0.5f;
    float grainStart = std::max(currentPosition - halfGrainSize, cropStart);
    float grainEnd = std::min(currentPosition + halfGrainSize, cropEnd);

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
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded())
        return;

    float mx = static_cast<float>(event.x);

    // Check crop handles first (prioritize the closer one if overlapping)
    bool nearStart = isNearCropHandle(mx, cropStart);
    bool nearEnd = isNearCropHandle(mx, cropEnd);

    if (nearStart && nearEnd) {
        // Both close - pick whichever is closer
        float distStart = std::abs(mx - normalizedToScreen(cropStart));
        float distEnd = std::abs(mx - normalizedToScreen(cropEnd));
        currentDrag = (distStart <= distEnd) ? DragTarget::CropStart : DragTarget::CropEnd;
    } else if (nearStart) {
        currentDrag = DragTarget::CropStart;
    } else if (nearEnd) {
        currentDrag = DragTarget::CropEnd;
    } else {
        currentDrag = DragTarget::None;
        lastDragX = mx;
    }
}

void WaveformDisplay::mouseDrag(const juce::MouseEvent& event) {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded())
        return;

    const float minCropGap = 0.01f;

    if (currentDrag == DragTarget::CropStart) {
        float normalized = screenToNormalized(static_cast<float>(event.x));
        cropStart = juce::jlimit(0.0f, cropEnd - minCropGap, normalized);
        if (onCropChanged)
            onCropChanged(cropStart, cropEnd);
        repaint();
        return;
    }

    if (currentDrag == DragTarget::CropEnd) {
        float normalized = screenToNormalized(static_cast<float>(event.x));
        cropEnd = juce::jlimit(cropStart + minCropGap, 1.0f, normalized);
        if (onCropChanged)
            onCropChanged(cropStart, cropEnd);
        repaint();
        return;
    }

    // Panning (only when zoomed in)
    if (zoomLevel <= 1.0f)
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

void WaveformDisplay::mouseUp(const juce::MouseEvent&) {
    currentDrag = DragTarget::None;
}

void WaveformDisplay::mouseMove(const juce::MouseEvent& event) {
    if (sampleBuffer == nullptr || !sampleBuffer->isLoaded()) {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    float mx = static_cast<float>(event.x);
    if (isNearCropHandle(mx, cropStart) || isNearCropHandle(mx, cropEnd)) {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    } else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
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
        lastSampleGain == sampleGainDb &&
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

    // Apply sample gain to amplitude (convert dB to linear)
    float gainLinear = std::pow(10.0f, sampleGainDb / 20.0f);
    const float amplitude = height * 0.45f * gainLinear;

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
    lastSampleGain = sampleGainDb;
    lastWidth = width;
    lastHeight = height;
    waveformNeedsUpdate = false;
}

void WaveformDisplay::invalidateWaveformCache() {
    waveformNeedsUpdate = true;
    cachedWaveform = juce::Image();
    lastViewStart = -1.0f;
    lastZoomLevel = -1.0f;
    lastSampleGain = -999.0f;
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
