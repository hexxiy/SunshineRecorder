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

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse interactions for zoom/pan
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // Drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

    // Callback when file is dropped
    std::function<void(const juce::File&)> onFileDropped;

private:
    void generateWaveformPath();
    float screenToNormalized(float screenX) const;
    float normalizedToScreen(float normalized) const;

    const SampleBuffer* sampleBuffer = nullptr;
    juce::Path waveformPath;
    float currentPosition = 0.0f;
    float currentGrainSize = 0.05f;  // Normalized grain size (0-1)
    bool isDraggingFile = false;

    // Zoom and pan state
    float zoomLevel = 1.0f;      // 1.0 = full view, higher = zoomed in
    float viewStart = 0.0f;      // Start of visible region (0-1)
    float lastDragX = 0.0f;      // For panning

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

} // namespace palace
