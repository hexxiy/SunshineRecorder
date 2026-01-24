#include "PluginEditor.h"

namespace palace {

PalaceAudioProcessorEditor::PalaceAudioProcessorEditor(PalaceAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    setLookAndFeel(&lookAndFeel);

    // Setup waveform display
    waveformDisplay.setSampleBuffer(&audioProcessor.getSampleBuffer());
    waveformDisplay.onFileDropped = [this](const juce::File& file) {
        audioProcessor.loadSample(file);
        waveformDisplay.setSampleBuffer(&audioProcessor.getSampleBuffer());
    };
    addAndMakeVisible(waveformDisplay);

    // Setup grain visualizer
    addAndMakeVisible(grainVisualizer);

    // Setup load button
    loadButton.onClick = [this]() {
        auto chooser = std::make_unique<juce::FileChooser>(
            "Select audio file",
            juce::File{},
            "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg"
        );

        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync(flags, [this, chooserPtr = chooser.get()](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file.existsAsFile()) {
                audioProcessor.loadSample(file);
                waveformDisplay.setSampleBuffer(&audioProcessor.getSampleBuffer());
            }
        });

        chooser.release();
    };
    addAndMakeVisible(loadButton);

    // Setup grain knobs
    auto& apvts = audioProcessor.getAPVTS();
    setupKnob(positionKnob, ParamIDs::position, "POSITION");
    setupKnob(grainSizeKnob, ParamIDs::grainSize, "SIZE");
    setupKnob(densityKnob, ParamIDs::density, "DENSITY");
    setupKnob(pitchKnob, ParamIDs::pitch, "PITCH");
    setupKnob(sprayKnob, ParamIDs::spray, "SPRAY");
    setupKnob(panSpreadKnob, ParamIDs::panSpread, "PAN");
    setupKnob(grainAttackKnob, ParamIDs::grainAttack, "G.ATK");
    setupKnob(grainReleaseKnob, ParamIDs::grainRelease, "G.REL");

    // Setup voice ADSR knobs
    setupKnob(voiceAttackKnob, ParamIDs::voiceAttack, "ATTACK");
    setupKnob(voiceDecayKnob, ParamIDs::voiceDecay, "DECAY");
    setupKnob(voiceSustainKnob, ParamIDs::voiceSustain, "SUSTAIN");
    setupKnob(voiceReleaseKnob, ParamIDs::voiceRelease, "RELEASE");

    // Setup output knobs
    setupKnob(mixKnob, ParamIDs::mix, "MIX");
    setupKnob(outputKnob, ParamIDs::output, "OUTPUT");

    // Setup keyboard octave controls
    octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
    octaveLabel.setColour(juce::Label::textColourId, OccultLookAndFeel::textLight);
    octaveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(octaveLabel);

    octaveDownButton.onClick = [this]() {
        if (keyboardOctave > 0) {
            keyboardOctave--;
            octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
        }
    };
    addAndMakeVisible(octaveDownButton);

    octaveUpButton.onClick = [this]() {
        if (keyboardOctave < 8) {
            keyboardOctave++;
            octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
        }
    };
    addAndMakeVisible(octaveUpButton);

    // Add keyboard listener
    addKeyListener(this);
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();

    // Start timer for UI updates
    startTimerHz(30);

    setSize(800, 600);
}

PalaceAudioProcessorEditor::~PalaceAudioProcessorEditor() {
    removeKeyListener(this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void PalaceAudioProcessorEditor::setupKnob(OccultKnob& knob, const juce::String& paramId, const juce::String& /*label*/) {
    knob.setAttachment(audioProcessor.getAPVTS(), paramId);
    addAndMakeVisible(knob);
}

void PalaceAudioProcessorEditor::paint(juce::Graphics& g) {
    // Fill background
    g.fillAll(OccultLookAndFeel::backgroundDark);

    // Draw header
    drawHeader(g, {0, 0, getWidth(), 50});

    // Draw section borders
    drawSectionBorder(g, {10, 190, 480, 200}, "GRAIN");
    drawSectionBorder(g, {500, 190, 290, 200}, "ENVELOPE");
    drawSectionBorder(g, {10, 400, 780, 190}, "OUTPUT");

    // Draw decorative elements
    g.setColour(OccultLookAndFeel::metalDark);

    // Corner sigils
    const float sigilSize = 20.0f;

    // Top-left sigil
    g.drawLine(15.0f, 55.0f, 15.0f + sigilSize, 55.0f, 1.0f);
    g.drawLine(15.0f, 55.0f, 15.0f, 55.0f + sigilSize, 1.0f);

    // Top-right sigil
    g.drawLine(getWidth() - 15.0f, 55.0f, getWidth() - 15.0f - sigilSize, 55.0f, 1.0f);
    g.drawLine(getWidth() - 15.0f, 55.0f, getWidth() - 15.0f, 55.0f + sigilSize, 1.0f);
}

void PalaceAudioProcessorEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> bounds) {
    // Draw header background gradient
    juce::ColourGradient headerGradient(OccultLookAndFeel::panelDark, 0, static_cast<float>(bounds.getY()),
                                        OccultLookAndFeel::backgroundDark, 0, static_cast<float>(bounds.getBottom()), false);
    g.setGradientFill(headerGradient);
    g.fillRect(bounds);

    // Draw keyboard indicator (top right)
    {
        auto indicatorBounds = juce::Rectangle<float>(getWidth() - 42.0f, 8.0f, 32.0f, 18.0f);
        float alpha = keyboardActive ? 1.0f : 0.3f;

        // Draw background
        g.setColour(OccultLookAndFeel::amber.withAlpha(0.2f * alpha));
        g.fillRoundedRectangle(indicatorBounds, 3.0f);
        g.setColour(OccultLookAndFeel::amber.withAlpha(alpha));
        g.drawRoundedRectangle(indicatorBounds, 3.0f, 1.0f);

        // Draw mini keyboard icon (simplified)
        g.setColour(OccultLookAndFeel::amber.withAlpha(alpha));
        float kx = indicatorBounds.getX() + 4.0f;
        float ky = indicatorBounds.getY() + 6.0f;
        // Draw white keys
        for (int i = 0; i < 5; i++) {
            g.fillRect(kx + i * 5.0f, ky, 4.0f, 8.0f);
        }
        // Draw black keys
        g.setColour(OccultLookAndFeel::backgroundDark);
        g.fillRect(kx + 3.5f, ky, 3.0f, 5.0f);
        g.fillRect(kx + 8.5f, ky, 3.0f, 5.0f);
        g.fillRect(kx + 18.5f, ky, 3.0f, 5.0f);
    }

    // Draw title
    g.setColour(OccultLookAndFeel::amber);
    g.setFont(juce::Font(juce::FontOptions(28.0f, juce::Font::bold)));
    g.drawText("PALACE", bounds.reduced(20, 0), juce::Justification::centredLeft);

    // Draw subtitle
    g.setColour(OccultLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("GRANULAR SYNTHESIS", bounds.reduced(20, 0).translated(100, 8), juce::Justification::centredLeft);

    // Draw separator line
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawLine(0.0f, static_cast<float>(bounds.getBottom()), static_cast<float>(getWidth()), static_cast<float>(bounds.getBottom()), 1.0f);

    // Draw amber accent line
    g.setColour(OccultLookAndFeel::amberDim);
    g.drawLine(20.0f, static_cast<float>(bounds.getBottom()), 120.0f, static_cast<float>(bounds.getBottom()), 2.0f);
}

void PalaceAudioProcessorEditor::drawSectionBorder(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title) {
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    // Draw section title
    g.setColour(OccultLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));

    const int titleWidth = g.getCurrentFont().getStringWidth(title) + 10;
    juce::Rectangle<int> titleBounds(bounds.getX() + 15, bounds.getY() - 6, titleWidth, 12);

    g.setColour(OccultLookAndFeel::backgroundDark);
    g.fillRect(titleBounds);

    g.setColour(OccultLookAndFeel::textDim);
    g.drawText(title, titleBounds, juce::Justification::centred);
}

void PalaceAudioProcessorEditor::resized() {
    const int margin = 10;
    const int headerHeight = 50;
    const int waveformHeight = 120;
    const int knobWidth = 85;
    const int knobHeight = 100;

    // Waveform display
    auto waveformBounds = juce::Rectangle<int>(margin, headerHeight + margin, getWidth() - margin * 2 - 70, waveformHeight);
    waveformDisplay.setBounds(waveformBounds);

    // Load button
    loadButton.setBounds(getWidth() - margin - 60, headerHeight + margin, 60, 30);

    // Grain visualizer (below load button)
    grainVisualizer.setBounds(getWidth() - margin - 60, headerHeight + margin + 35, 60, waveformHeight - 35);

    // Grain controls section
    const int grainSectionY = headerHeight + waveformHeight + margin * 3;

    positionKnob.setBounds(margin + 20, grainSectionY, knobWidth, knobHeight);
    grainSizeKnob.setBounds(margin + 20 + knobWidth, grainSectionY, knobWidth, knobHeight);
    densityKnob.setBounds(margin + 20 + knobWidth * 2, grainSectionY, knobWidth, knobHeight);
    pitchKnob.setBounds(margin + 20 + knobWidth * 3, grainSectionY, knobWidth, knobHeight);
    sprayKnob.setBounds(margin + 20 + knobWidth * 4, grainSectionY, knobWidth, knobHeight);

    // Grain envelope row
    const int grainEnvY = grainSectionY + knobHeight + 10;
    panSpreadKnob.setBounds(margin + 20, grainEnvY, knobWidth, knobHeight);
    grainAttackKnob.setBounds(margin + 20 + knobWidth, grainEnvY, knobWidth, knobHeight);
    grainReleaseKnob.setBounds(margin + 20 + knobWidth * 2, grainEnvY, knobWidth, knobHeight);

    // Voice ADSR section
    const int adsrSectionX = 510;
    voiceAttackKnob.setBounds(adsrSectionX, grainSectionY, knobWidth, knobHeight);
    voiceDecayKnob.setBounds(adsrSectionX + knobWidth, grainSectionY, knobWidth, knobHeight);
    voiceSustainKnob.setBounds(adsrSectionX, grainEnvY, knobWidth, knobHeight);
    voiceReleaseKnob.setBounds(adsrSectionX + knobWidth, grainEnvY, knobWidth, knobHeight);

    // Output section
    const int outputSectionY = grainEnvY + knobHeight + 30;
    const int outputSectionX = getWidth() / 2 - knobWidth;
    mixKnob.setBounds(outputSectionX, outputSectionY, knobWidth, knobHeight);
    outputKnob.setBounds(outputSectionX + knobWidth, outputSectionY, knobWidth, knobHeight);

    // Keyboard octave controls (bottom right)
    const int octaveControlY = outputSectionY + 30;
    const int octaveControlX = getWidth() - margin - 140;
    octaveDownButton.setBounds(octaveControlX, octaveControlY, 30, 25);
    octaveLabel.setBounds(octaveControlX + 35, octaveControlY, 60, 25);
    octaveUpButton.setBounds(octaveControlX + 100, octaveControlY, 30, 25);
}

void PalaceAudioProcessorEditor::mouseDown(const juce::MouseEvent& event) {
    juce::AudioProcessorEditor::mouseDown(event);
    grabKeyboardFocus();
}

void PalaceAudioProcessorEditor::timerCallback() {
    // Update waveform position indicator
    const float position = audioProcessor.getParameters().position->load();
    waveformDisplay.setPosition(position);

    // Update grain size indicator on waveform
    const float grainSizeMs = audioProcessor.getParameters().grainSize->load();
    const auto& sampleBuffer = audioProcessor.getSampleBuffer();
    if (sampleBuffer.isLoaded()) {
        const double sampleDurationMs = (sampleBuffer.getNumSamples() / sampleBuffer.getSampleRate()) * 1000.0;
        const float normalizedGrainSize = static_cast<float>(grainSizeMs / sampleDurationMs);
        waveformDisplay.setGrainSize(normalizedGrainSize);
    }

    // Update grain visualizer
    grainVisualizer.setPosition(position);
    grainVisualizer.setSpray(audioProcessor.getParameters().spray->load() / 100.0f);

    // Estimate active grain count from density
    const float density = audioProcessor.getParameters().density->load();
    const float grainSize = audioProcessor.getParameters().grainSize->load();
    const int estimatedGrains = static_cast<int>(density * grainSize / 1000.0f);
    grainVisualizer.setActiveGrainCount(estimatedGrains);

    // Update keyboard state for note-offs
    updateKeyboardState();
}

int PalaceAudioProcessorEditor::keyToMidiNote(int keyCode) const {
    // Map QWERTY keys to piano layout
    // White keys: A S D F G H J K = C D E F G A B C
    // Black keys: W E   T Y U     = C# D#  F# G# A#
    int baseNote = keyboardOctave * 12;  // C of current octave

    switch (keyCode) {
        case 'A': return baseNote + 0;   // C
        case 'W': return baseNote + 1;   // C#
        case 'S': return baseNote + 2;   // D
        case 'E': return baseNote + 3;   // D#
        case 'D': return baseNote + 4;   // E
        case 'F': return baseNote + 5;   // F
        case 'T': return baseNote + 6;   // F#
        case 'G': return baseNote + 7;   // G
        case 'Y': return baseNote + 8;   // G#
        case 'H': return baseNote + 9;   // A
        case 'U': return baseNote + 10;  // A#
        case 'J': return baseNote + 11;  // B
        case 'K': return baseNote + 12;  // C (next octave)
        default: return -1;
    }
}

bool PalaceAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component*) {
    int keyCode = key.getKeyCode();

    // Detect caps lock state - if we receive uppercase, caps is on
    bool isCapsOn = (keyCode >= 'A' && keyCode <= 'Z');
    if (keyboardActive != isCapsOn) {
        keyboardActive = isCapsOn;
        repaint();
    }

    // Octave switching with Z and X
    if (keyCode == 'Z') {
        if (keyboardOctave > 0) {
            keyboardOctave--;
            octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
        }
        return true;
    }
    if (keyCode == 'X') {
        if (keyboardOctave < 8) {
            keyboardOctave++;
            octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
        }
        return true;
    }

    // Check if it's a note key and not already pressed
    int midiNote = keyToMidiNote(keyCode);
    if (midiNote >= 0 && midiNote <= 127) {
        if (activeKeys.find(keyCode) == activeKeys.end()) {
            activeKeys.insert(keyCode);
            audioProcessor.addKeyboardNoteOn(midiNote, 0.8f);
        }
        return true;
    }

    return false;
}

bool PalaceAudioProcessorEditor::keyStateChanged(bool /*isKeyDown*/, juce::Component*) {
    updateKeyboardState();
    return false;
}

void PalaceAudioProcessorEditor::updateKeyboardState() {
    // Check for released keys
    std::vector<int> keysToRemove;

    for (int keyCode : activeKeys) {
        if (!juce::KeyPress::isKeyCurrentlyDown(keyCode)) {
            int midiNote = keyToMidiNote(keyCode);
            if (midiNote >= 0 && midiNote <= 127) {
                audioProcessor.addKeyboardNoteOff(midiNote);
            }
            keysToRemove.push_back(keyCode);
        }
    }

    for (int keyCode : keysToRemove) {
        activeKeys.erase(keyCode);
    }
}

} // namespace palace
