#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/SampleBuffer.h"

namespace palace {

class WaveformDisplay : public juce::Component,
                        public juce::FileDragAndDropTarget {
public:
    WaveformDisplay();
    ~WaveformDisplay() override;

    void setSampleBuffer(const SampleBuffer* buffer);
    void setPosition(float normalizedPosition);  // 0-1
    void setGrainSize(float normalizedSize);     // 0-1 relative to sample length

    // Zoom controls
    void zoomIn();
    void zoomOut();
    void resetZoom();

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse interactions for zoom/pan/crop
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // Drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

    // Callback when file is dropped
    std::function<void(const juce::File&)> onFileDropped;

    // Crop region
    float getCropStart() const { return cropStart; }
    float getCropEnd() const { return cropEnd; }
    void resetCrop();

    // Callback when crop handles are changed
    std::function<void(float, float)> onCropChanged;

private:
    void generateWaveformPath();
    void invalidateWaveformCache();
    float screenToNormalized(float screenX) const;
    float normalizedToScreen(float normalized) const;
    bool isNearCropHandle(float screenX, float handleNormalized, float tolerance = 6.0f) const;

    const SampleBuffer* sampleBuffer = nullptr;
    juce::Path waveformPath;
    juce::Image cachedWaveform;
    float currentPosition = 0.0f;
    float currentGrainSize = 0.05f;  // Normalized grain size (0-1)
    bool isDraggingFile = false;
    bool waveformNeedsUpdate = true;

    // Zoom and pan state
    float zoomLevel = 1.0f;      // 1.0 = full view, higher = zoomed in
    float viewStart = 0.0f;      // Start of visible region (0-1)
    float lastDragX = 0.0f;      // For panning

    // Crop state
    float cropStart = 0.0f;
    float cropEnd = 1.0f;
    enum class DragTarget { None, CropStart, CropEnd };
    DragTarget currentDrag = DragTarget::None;

    // Cache state to avoid unnecessary regeneration
    float lastViewStart = -1.0f;
    float lastZoomLevel = -1.0f;
    int lastWidth = -1;
    int lastHeight = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

} // namespace palace
