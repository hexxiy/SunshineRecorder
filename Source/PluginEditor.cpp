#include "PluginEditor.h"

namespace palace {

PalaceAudioProcessorEditor::PalaceAudioProcessorEditor(PalaceAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    setLookAndFeel(&lookAndFeel);

    // Setup waveform display
    waveformDisplay.setSampleBuffer(&audioProcessor.getSampleBuffer());
    waveformDisplay.setDisintegrationEngine(&audioProcessor.getDisintegrationEngine());
    waveformDisplay.onFileDropped = [this](const juce::File& file) {
        audioProcessor.loadSample(file);
        waveformDisplay.setSampleBuffer(&audioProcessor.getSampleBuffer());
    };
    waveformDisplay.onCropChanged = [this](float start, float end) {
        audioProcessor.setCropRegion(start, end);
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

    // Setup zoom buttons
    zoomInButton.onClick = [this]() {
        waveformDisplay.zoomIn();
    };
    addAndMakeVisible(zoomInButton);

    zoomOutButton.onClick = [this]() {
        waveformDisplay.zoomOut();
    };
    addAndMakeVisible(zoomOutButton);

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

    // Setup LFO controls
    addAndMakeVisible(lfoVisualizer);
    setupKnob(lfoRateKnob, ParamIDs::lfoRate, "RATE");
    setupKnob(lfoAmountKnob, ParamIDs::lfoAmount, "AMOUNT");

    lfoWaveformBox.addItem("Sine", 1);
    lfoWaveformBox.addItem("Triangle", 2);
    lfoWaveformBox.addItem("Square", 3);
    lfoWaveformBox.addItem("Noise", 4);
    lfoWaveformBox.addItem("S&H", 5);
    lfoWaveformBox.setSelectedId(1);
    lfoWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParamIDs::lfoWaveform, lfoWaveformBox);
    addAndMakeVisible(lfoWaveformBox);

    // Create modulation buttons for each modulatable parameter
    createModButton(ParamIDs::position);
    createModButton(ParamIDs::grainSize);
    createModButton(ParamIDs::density);
    createModButton(ParamIDs::pitch);
    createModButton(ParamIDs::spray);
    createModButton(ParamIDs::panSpread);
    createModButton(ParamIDs::grainAttack);
    createModButton(ParamIDs::grainRelease);
    createModButton(ParamIDs::voiceAttack);
    createModButton(ParamIDs::voiceDecay);
    createModButton(ParamIDs::voiceSustain);
    createModButton(ParamIDs::voiceRelease);

    // Setup tape delay knobs
    setupKnob(delayTimeKnob, ParamIDs::delayTime, "DELAY");
    setupKnob(flutterKnob, ParamIDs::flutter, "FLUTTER");
    setupKnob(hissKnob, ParamIDs::tapeHiss, "HISS");

    // Setup tape disintegration knobs
    setupKnob(damageKnob, ParamIDs::damage, "DAMAGE");
    setupKnob(lifeKnob, ParamIDs::life, "LIFE");

    // Setup reset damage button
    resetDamageButton.onClick = [this]() {
        audioProcessor.getDisintegrationEngine().resetAllLife();
    };
    addAndMakeVisible(resetDamageButton);

    // Setup effects knobs
    setupKnob(reverbKnob, ParamIDs::reverb, "REVERB");
    setupKnob(feedbackKnob, ParamIDs::feedback, "FEEDBACK");

    // Setup output knobs
    setupKnob(mixKnob, ParamIDs::mix, "MIX");
    setupKnob(outputKnob, ParamIDs::output, "OUTPUT");

    // Setup sample gain knob
    setupKnob(sampleGainKnob, ParamIDs::sampleGain, "GAIN");

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

    // Add mouse listeners to knobs for MIDI learn (with child listening)
    positionKnob.addMouseListener(this, true);
    grainSizeKnob.addMouseListener(this, true);
    densityKnob.addMouseListener(this, true);
    pitchKnob.addMouseListener(this, true);
    sprayKnob.addMouseListener(this, true);
    panSpreadKnob.addMouseListener(this, true);
    grainAttackKnob.addMouseListener(this, true);
    grainReleaseKnob.addMouseListener(this, true);
    voiceAttackKnob.addMouseListener(this, true);
    voiceDecayKnob.addMouseListener(this, true);
    voiceSustainKnob.addMouseListener(this, true);
    voiceReleaseKnob.addMouseListener(this, true);
    lfoRateKnob.addMouseListener(this, true);
    lfoAmountKnob.addMouseListener(this, true);
    delayTimeKnob.addMouseListener(this, true);
    flutterKnob.addMouseListener(this, true);
    hissKnob.addMouseListener(this, true);
    damageKnob.addMouseListener(this, true);
    lifeKnob.addMouseListener(this, true);
    reverbKnob.addMouseListener(this, true);
    feedbackKnob.addMouseListener(this, true);
    mixKnob.addMouseListener(this, true);
    outputKnob.addMouseListener(this, true);
    sampleGainKnob.addMouseListener(this, true);

    // Start timer for UI updates
    startTimerHz(30);

    // Load autosave settings and MIDI mappings
    loadAutoSaveSettings();
    if (midiAutoSave) {
        loadMidiMappings();
    }

    setSize(850, 720);
}

PalaceAudioProcessorEditor::~PalaceAudioProcessorEditor() {
    // Autosave MIDI mappings if enabled
    if (midiAutoSave) {
        saveMidiMappings();
    }

    // Remove mouse listeners from knobs
    positionKnob.removeMouseListener(this);
    grainSizeKnob.removeMouseListener(this);
    densityKnob.removeMouseListener(this);
    pitchKnob.removeMouseListener(this);
    sprayKnob.removeMouseListener(this);
    panSpreadKnob.removeMouseListener(this);
    grainAttackKnob.removeMouseListener(this);
    grainReleaseKnob.removeMouseListener(this);
    voiceAttackKnob.removeMouseListener(this);
    voiceDecayKnob.removeMouseListener(this);
    voiceSustainKnob.removeMouseListener(this);
    voiceReleaseKnob.removeMouseListener(this);
    lfoRateKnob.removeMouseListener(this);
    lfoAmountKnob.removeMouseListener(this);
    delayTimeKnob.removeMouseListener(this);
    flutterKnob.removeMouseListener(this);
    hissKnob.removeMouseListener(this);
    damageKnob.removeMouseListener(this);
    lifeKnob.removeMouseListener(this);
    reverbKnob.removeMouseListener(this);
    feedbackKnob.removeMouseListener(this);
    mixKnob.removeMouseListener(this);
    outputKnob.removeMouseListener(this);
    sampleGainKnob.removeMouseListener(this);

    removeKeyListener(this);
    stopTimer();
    setLookAndFeel(nullptr);
}

void PalaceAudioProcessorEditor::setupKnob(OccultKnob& knob, const juce::String& paramId, const juce::String& /*label*/) {
    knob.setAttachment(audioProcessor.getAPVTS(), paramId);
    addAndMakeVisible(knob);
}

void PalaceAudioProcessorEditor::paint(juce::Graphics& g) {
    const float scaleX = getWidth() / 850.0f;
    const float scaleY = getHeight() / 720.0f;
    const float scale = std::min(scaleX, scaleY);

    // Fill background
    g.fillAll(OccultLookAndFeel::backgroundDark);

    // Draw header
    drawHeader(g, {0, 0, getWidth(), static_cast<int>(50 * scaleY)});

    // Draw section borders (scaled) with proper margin padding
    const int margin = static_cast<int>(20 * scale);
    const int grainY = static_cast<int>(190 * scaleY);
    const int grainH = static_cast<int>(200 * scaleY);
    const int bottomY = static_cast<int>(400 * scaleY);
    const int bottomH = getHeight() - bottomY - margin;

    drawSectionBorder(g, {margin, grainY, static_cast<int>(470 * scaleX), grainH}, "GRAIN");
    drawSectionBorder(g, {static_cast<int>(500 * scaleX), grainY, getWidth() - static_cast<int>(500 * scaleX) - margin, grainH}, "ENVELOPE");
    // LFO section starts a bit lower to position title below visualizer
    const int lfoSectionY = bottomY + static_cast<int>(55 * scaleY);
    drawSectionBorder(g, {margin, lfoSectionY, static_cast<int>(185 * scaleX), bottomH - static_cast<int>(55 * scaleY)}, "LFO");
    drawSectionBorder(g, {static_cast<int>(210 * scaleX), bottomY, static_cast<int>(275 * scaleX), bottomH}, "TAPE DELAY");
    drawSectionBorder(g, {static_cast<int>(495 * scaleX), bottomY, getWidth() - static_cast<int>(495 * scaleX) - margin, bottomH}, "OUTPUT");

    // Draw decorative elements
    g.setColour(OccultLookAndFeel::metalDark);

    // Corner sigils
    const float sigilSize = 20.0f * scale;
    const float sigilY = 55.0f * scaleY;
    const float sigilMargin = static_cast<float>(margin);

    // Top-left sigil
    g.drawLine(sigilMargin, sigilY, sigilMargin + sigilSize, sigilY, 1.0f);
    g.drawLine(sigilMargin, sigilY, sigilMargin, sigilY + sigilSize, 1.0f);

    // Top-right sigil
    g.drawLine(getWidth() - sigilMargin, sigilY, getWidth() - sigilMargin - sigilSize, sigilY, 1.0f);
    g.drawLine(getWidth() - sigilMargin, sigilY, getWidth() - sigilMargin, sigilY + sigilSize, 1.0f);

    // Draw MIDI status when in learn mode
    if (midiLearnMode || mappingMessageTimeout > 0) {
        drawMidiStatus(g, {margin, getHeight() - static_cast<int>(55 * scaleY), getWidth() - margin * 2, static_cast<int>(22 * scaleY)});
        drawMidiDebug(g, {margin, getHeight() - static_cast<int>(30 * scaleY), getWidth() - margin * 2, static_cast<int>(25 * scaleY)});
    }

    // Draw version info (bottom right corner)
    g.setColour(OccultLookAndFeel::textDim.withAlpha(0.5f));
    g.setFont(9.0f);
    juce::String versionText = "v" PROJECT_VERSION " (" BUILD_TIMESTAMP ")";
    g.drawText(versionText, getWidth() - 210, getHeight() - 18, 200, 15, juce::Justification::centredRight);
}

void PalaceAudioProcessorEditor::drawHeader(juce::Graphics& g, juce::Rectangle<int> bounds) {
    // Draw header background gradient
    juce::ColourGradient headerGradient(OccultLookAndFeel::panelDark, 0, static_cast<float>(bounds.getY()),
                                        OccultLookAndFeel::backgroundDark, 0, static_cast<float>(bounds.getBottom()), false);
    g.setGradientFill(headerGradient);
    g.fillRect(bounds);

    // Draw MIDI indicator (top right, before keyboard)
    {
        auto indicatorBounds = juce::Rectangle<float>(getWidth() - 80.0f, 8.0f, 32.0f, 18.0f);
        float alpha = midiLearnMode ? 1.0f : 0.3f;

        // Draw background - pulsing when in learn mode
        if (midiLearnMode) {
            float pulse = 0.5f + 0.5f * std::sin(juce::Time::getMillisecondCounterHiRes() / 200.0f);
            g.setColour(OccultLookAndFeel::amber.withAlpha(0.3f * pulse));
        } else {
            g.setColour(OccultLookAndFeel::amber.withAlpha(0.2f * alpha));
        }
        g.fillRoundedRectangle(indicatorBounds, 3.0f);
        g.setColour(OccultLookAndFeel::amber.withAlpha(alpha));
        g.drawRoundedRectangle(indicatorBounds, 3.0f, 1.0f);

        // Draw MIDI icon (5-pin DIN connector style)
        g.setColour(OccultLookAndFeel::amber.withAlpha(alpha));
        float mx = indicatorBounds.getCentreX();
        float my = indicatorBounds.getCentreY();
        // Draw circle
        g.drawEllipse(mx - 6.0f, my - 5.0f, 12.0f, 10.0f, 1.0f);
        // Draw pins (simplified)
        g.fillEllipse(mx - 4.0f, my - 2.0f, 2.0f, 2.0f);
        g.fillEllipse(mx + 2.0f, my - 2.0f, 2.0f, 2.0f);
        g.fillEllipse(mx - 1.0f, my + 1.0f, 2.0f, 2.0f);
    }

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

    // Draw title - centered with serif font
    g.setColour(OccultLookAndFeel::amber);
    // Use serif font - try to get a nice looking one
    juce::Font titleFont = juce::Font(juce::Font::getDefaultSerifFontName(), 24.0f, juce::Font::bold);
    g.setFont(titleFont);

    auto titleBounds = bounds.reduced(100, 0).withTrimmedBottom(14);
    g.drawText("SUNSHINE RECORDER", titleBounds, juce::Justification::centred);

    // Draw subtitle below title
    g.setColour(OccultLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    auto subtitleBounds = bounds.reduced(100, 0).withTrimmedTop(28);
    g.drawText("GRANULAR SYNTHESIZER", subtitleBounds, juce::Justification::centred);

    // Draw separator line
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawLine(0.0f, static_cast<float>(bounds.getBottom()), static_cast<float>(getWidth()), static_cast<float>(bounds.getBottom()), 1.0f);

    // Draw amber accent lines (centered)
    float centerX = getWidth() / 2.0f;
    float lineWidth = 80.0f;
    g.setColour(OccultLookAndFeel::amberDim);
    g.drawLine(centerX - lineWidth, static_cast<float>(bounds.getBottom()),
               centerX + lineWidth, static_cast<float>(bounds.getBottom()), 2.0f);
}

void PalaceAudioProcessorEditor::drawSectionBorder(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title) {
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    // Draw section title
    g.setColour(OccultLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));

    const int titleWidth = g.getCurrentFont().getStringWidth(title) + 10;
    juce::Rectangle<int> titleBounds(bounds.getX() + 15, bounds.getY() - 11, titleWidth, 12);

    g.setColour(OccultLookAndFeel::backgroundDark);
    g.fillRect(titleBounds);

    g.setColour(OccultLookAndFeel::textDim);
    g.drawText(title, titleBounds, juce::Justification::centred);
}

void PalaceAudioProcessorEditor::resized() {
    // SVG-based layout (generated from SR.svg via svg_to_juce.py)
    const float scaleX = getWidth() / 850.0f;
    const float scaleY = getHeight() / 720.0f;
    const float scale = std::min(scaleX, scaleY);
    const int knobWidth = static_cast<int>(85 * scaleX);
    const int knobHeight = static_cast<int>(100 * scaleY);

    // Waveform section
    zoomInButton.setBounds(
        static_cast<int>(19 * scaleX),
        static_cast<int>(87 * scaleY),
        static_cast<int>(26 * scaleX),
        static_cast<int>(27 * scaleY)
    );
    zoomOutButton.setBounds(
        static_cast<int>(19 * scaleX),
        static_cast<int>(117 * scaleY),
        static_cast<int>(26 * scaleX),
        static_cast<int>(27 * scaleY)
    );
    waveformDisplay.setBounds(
        static_cast<int>(53 * scaleX),
        static_cast<int>(68 * scaleY),
        static_cast<int>(567 * scaleX),
        static_cast<int>(121 * scaleY)
    );
    loadButton.setBounds(
        static_cast<int>(631 * scaleX),
        static_cast<int>(68 * scaleY),
        static_cast<int>(57 * scaleX),
        static_cast<int>(30 * scaleY)
    );
    sampleGainKnob.setBounds(
        static_cast<int>(751 * scaleX) - knobWidth/2,
        static_cast<int>(127 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    grainVisualizer.setBounds(
        static_cast<int>(631 * scaleX),
        static_cast<int>(167 * scaleY),
        static_cast<int>(57 * scaleX),
        static_cast<int>(19 * scaleY)
    );

    // Grain controls
    positionKnob.setBounds(
        static_cast<int>(64 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    grainSizeKnob.setBounds(
        static_cast<int>(147 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    densityKnob.setBounds(
        static_cast<int>(230 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    pitchKnob.setBounds(
        static_cast<int>(314 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    sprayKnob.setBounds(
        static_cast<int>(397 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    panSpreadKnob.setBounds(
        static_cast<int>(64 * scaleX) - knobWidth/2,
        static_cast<int>(352 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    grainAttackKnob.setBounds(
        static_cast<int>(147 * scaleX) - knobWidth/2,
        static_cast<int>(352 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    grainReleaseKnob.setBounds(
        static_cast<int>(230 * scaleX) - knobWidth/2,
        static_cast<int>(352 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );

    // Voice ADSR
    voiceAttackKnob.setBounds(
        static_cast<int>(567 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    voiceDecayKnob.setBounds(
        static_cast<int>(650 * scaleX) - knobWidth/2,
        static_cast<int>(254 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    voiceSustainKnob.setBounds(
        static_cast<int>(567 * scaleX) - knobWidth/2,
        static_cast<int>(352 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    voiceReleaseKnob.setBounds(
        static_cast<int>(650 * scaleX) - knobWidth/2,
        static_cast<int>(352 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );

    // LFO section
    lfoVisualizer.setBounds(
        static_cast<int>(34 * scaleX),
        static_cast<int>(447 * scaleY),
        static_cast<int>(166 * scaleX),
        static_cast<int>(49 * scaleY)
    );
    lfoRateKnob.setBounds(
        static_cast<int>(72 * scaleX) - knobWidth/2,
        static_cast<int>(549 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    lfoAmountKnob.setBounds(
        static_cast<int>(155 * scaleX) - knobWidth/2,
        static_cast<int>(549 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    lfoWaveformBox.setBounds(
        static_cast<int>(42 * scaleX),
        static_cast<int>(603 * scaleY),
        static_cast<int>(151 * scaleX),
        static_cast<int>(23 * scaleY)
    );

    // Tape delay
    delayTimeKnob.setBounds(
        static_cast<int>(268 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    flutterKnob.setBounds(
        static_cast<int>(351 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    hissKnob.setBounds(
        static_cast<int>(434 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );

    // Tape disintegration
    damageKnob.setBounds(
        static_cast<int>(268 * scaleX) - knobWidth/2,
        static_cast<int>(595 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    lifeKnob.setBounds(
        static_cast<int>(351 * scaleX) - knobWidth/2,
        static_cast<int>(595 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    resetDamageButton.setBounds(
        static_cast<int>(389 * scaleX),
        static_cast<int>(644 * scaleY),
        static_cast<int>(76 * scaleX),
        static_cast<int>(25 * scaleY)
    );

    // Output section
    reverbKnob.setBounds(
        static_cast<int>(559 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    feedbackKnob.setBounds(
        static_cast<int>(642 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    mixKnob.setBounds(
        static_cast<int>(725 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );
    outputKnob.setBounds(
        static_cast<int>(808 * scaleX) - knobWidth/2,
        static_cast<int>(496 * scaleY) - knobHeight/2,
        knobWidth,
        knobHeight
    );

    // Keyboard octave controls
    octaveDownButton.setBounds(
        static_cast<int>(597 * scaleX),
        static_cast<int>(561 * scaleY),
        static_cast<int>(30 * scaleX),
        static_cast<int>(25 * scaleY)
    );
    octaveLabel.setBounds(
        static_cast<int>(632 * scaleX),
        static_cast<int>(561 * scaleY),
        static_cast<int>(60 * scaleX),
        static_cast<int>(25 * scaleY)
    );
    octaveUpButton.setBounds(
        static_cast<int>(710 * scaleX),
        static_cast<int>(561 * scaleY),
        static_cast<int>(30 * scaleX),
        static_cast<int>(25 * scaleY)
    );

    // Position modulation buttons next to their associated knobs
    const int modButtonSize = static_cast<int>(16 * scale);
    auto positionModButton = [&](const juce::String& paramId, OccultKnob& knob) {
        if (modButtons.count(paramId) > 0) {
            auto knobBounds = knob.getBounds();
            modButtons[paramId]->setBounds(knobBounds.getRight() - modButtonSize - 2,
                                            knobBounds.getY() + static_cast<int>(18 * scale),
                                            modButtonSize, modButtonSize);
        }
    };

    positionModButton(ParamIDs::position, positionKnob);
    positionModButton(ParamIDs::grainSize, grainSizeKnob);
    positionModButton(ParamIDs::density, densityKnob);
    positionModButton(ParamIDs::pitch, pitchKnob);
    positionModButton(ParamIDs::spray, sprayKnob);
    positionModButton(ParamIDs::panSpread, panSpreadKnob);
    positionModButton(ParamIDs::grainAttack, grainAttackKnob);
    positionModButton(ParamIDs::grainRelease, grainReleaseKnob);
    positionModButton(ParamIDs::voiceAttack, voiceAttackKnob);
    positionModButton(ParamIDs::voiceDecay, voiceDecayKnob);
    positionModButton(ParamIDs::voiceSustain, voiceSustainKnob);
    positionModButton(ParamIDs::voiceRelease, voiceReleaseKnob);
}

void PalaceAudioProcessorEditor::mouseDown(const juce::MouseEvent& event) {
    // Check if clicking on MIDI indicator
    auto midiIndicatorBounds = juce::Rectangle<float>(getWidth() - 80.0f, 8.0f, 32.0f, 18.0f);
    if (midiIndicatorBounds.contains(event.position)) {
        if (event.mods.isRightButtonDown()) {
            showMidiDeviceMenu();
        } else {
            // Toggle MIDI learn mode
            midiLearnMode = !midiLearnMode;
            if (!midiLearnMode) {
                selectedParamForLearn.clear();
                audioProcessor.clearMidiLearnParameter();
            }
            repaint();
        }
        return;
    }

    // Check if clicking on keyboard indicator
    auto indicatorBounds = juce::Rectangle<float>(getWidth() - 42.0f, 8.0f, 32.0f, 18.0f);
    if (indicatorBounds.contains(event.position)) {
        keyboardActive = !keyboardActive;
        repaint();
        return;
    }

    // If in MIDI learn mode, check if clicking on a knob
    if (midiLearnMode) {
        // Get the original component that was clicked
        juce::Component* clickedComponent = event.originalComponent;

        // Check each knob - see if the clicked component is the knob or a child of it
        OccultKnob* knobs[] = {
            &positionKnob, &grainSizeKnob, &densityKnob, &pitchKnob,
            &sprayKnob, &panSpreadKnob, &grainAttackKnob, &grainReleaseKnob,
            &voiceAttackKnob, &voiceDecayKnob, &voiceSustainKnob, &voiceReleaseKnob,
            &lfoRateKnob, &lfoAmountKnob,
            &delayTimeKnob, &flutterKnob, &hissKnob,
            &damageKnob, &lifeKnob,
            &reverbKnob, &feedbackKnob, &mixKnob, &outputKnob
        };

        for (auto* knob : knobs) {
            if (clickedComponent == knob || knob->isParentOf(clickedComponent)) {
                juce::String paramId = getParamIdForKnob(knob);
                if (paramId.isNotEmpty()) {
                    selectParameterForLearn(paramId);
                }
                return;
            }
        }
    }

    juce::AudioProcessorEditor::mouseDown(event);
    grabKeyboardFocus();
}

void PalaceAudioProcessorEditor::timerCallback() {
    // Get LFO modulation values
    float lfoValue = audioProcessor.getCurrentLfoValue();
    float lfoAmount = audioProcessor.getParameters().lfoAmount->load() / 100.0f;
    float modAmount = lfoValue * lfoAmount;

    // Sync crop region from processor to waveform display
    float cs = audioProcessor.getCropStart();
    float ce = audioProcessor.getCropEnd();
    if (waveformDisplay.getCropStart() != cs || waveformDisplay.getCropEnd() != ce) {
        waveformDisplay.setCropRegion(cs, ce);
    }

    // Update sample gain in waveform display
    waveformDisplay.setSampleGain(audioProcessor.getParameters().sampleGain->load());

    // Update waveform position indicator (with LFO modulation if enabled)
    // Remap through crop region so the indicator shows the actual mapped position
    float rawPosition = audioProcessor.getParameters().position->load();
    if (audioProcessor.isLfoModulated(ParamIDs::position)) {
        rawPosition = juce::jlimit(0.0f, 1.0f, rawPosition + modAmount * 0.5f);
    }
    float position = cs + rawPosition * (ce - cs);
    waveformDisplay.setPosition(position);

    // Update grain size indicator on waveform (with LFO modulation if enabled)
    float grainSizeMs = audioProcessor.getParameters().grainSize->load();
    if (audioProcessor.isLfoModulated(ParamIDs::grainSize)) {
        // Modulate grain size (range is 10-8000ms, so modulate proportionally)
        grainSizeMs = juce::jlimit(10.0f, 8000.0f, grainSizeMs + modAmount * 2000.0f);
    }
    const auto& sampleBuffer = audioProcessor.getSampleBuffer();

    // Update active grain visualization
    if (sampleBuffer.isLoaded()) {
        auto activeGrains = audioProcessor.getAllActiveGrains();
        waveformDisplay.setActiveGrains(activeGrains, sampleBuffer.getNumSamples());
    }
    if (sampleBuffer.isLoaded()) {
        const double sampleDurationMs = (sampleBuffer.getNumSamples() / sampleBuffer.getSampleRate()) * 1000.0;
        // Clamp grain size so it cannot extend past crop boundaries
        float cropWidthMs = static_cast<float>((ce - cs) * sampleDurationMs);
        grainSizeMs = std::min(grainSizeMs, cropWidthMs);
        const float normalizedGrainSize = static_cast<float>(grainSizeMs / sampleDurationMs);
        waveformDisplay.setGrainSize(normalizedGrainSize);
    }

    // Update grain visualizer (with modulated position)
    grainVisualizer.setPosition(position);
    grainVisualizer.setSpray(audioProcessor.getParameters().spray->load() / 100.0f);

    // Estimate active grain count from density
    const float density = audioProcessor.getParameters().density->load();
    const float grainSize = audioProcessor.getParameters().grainSize->load();
    const int estimatedGrains = static_cast<int>(density * grainSize / 1000.0f);
    grainVisualizer.setActiveGrainCount(estimatedGrains);

    // Update LFO visualizer
    lfoVisualizer.setPhase(audioProcessor.getCurrentLfoPhase());
    lfoVisualizer.setWaveform(audioProcessor.getCurrentLfoWaveform());

    // Update modulation visualization on knobs (using lfoValue, lfoAmount, modAmount from above)

    // Helper lambda to update knob modulation
    auto updateKnobMod = [&](OccultKnob& knob, const juce::String& paramId) {
        bool isModulated = audioProcessor.isLfoModulated(paramId);
        knob.setModulationActive(isModulated);
        if (isModulated) {
            knob.setModulationAmount(modAmount);
        }
    };

    updateKnobMod(positionKnob, ParamIDs::position);
    updateKnobMod(grainSizeKnob, ParamIDs::grainSize);
    updateKnobMod(densityKnob, ParamIDs::density);
    updateKnobMod(pitchKnob, ParamIDs::pitch);
    updateKnobMod(sprayKnob, ParamIDs::spray);
    updateKnobMod(panSpreadKnob, ParamIDs::panSpread);
    updateKnobMod(grainAttackKnob, ParamIDs::grainAttack);
    updateKnobMod(grainReleaseKnob, ParamIDs::grainRelease);
    updateKnobMod(voiceAttackKnob, ParamIDs::voiceAttack);
    updateKnobMod(voiceDecayKnob, ParamIDs::voiceDecay);
    updateKnobMod(voiceSustainKnob, ParamIDs::voiceSustain);
    updateKnobMod(voiceReleaseKnob, ParamIDs::voiceRelease);

    // Update keyboard state for note-offs
    updateKeyboardState();

    // Check for incoming MIDI CC
    int currentCC = audioProcessor.getLastReceivedCC();
    if (currentCC >= 0 && currentCC != lastDisplayedCC) {
        lastDisplayedCC = currentCC;
    }

    // Check if MIDI learn was completed
    if (midiLearnMode && selectedParamForLearn.isNotEmpty()) {
        if (audioProcessor.getMidiLearnParameter().isEmpty()) {
            // Learn completed - CC was assigned
            lastMappingMessage = "Mapped CC " + juce::String(lastDisplayedCC) + " -> " + selectedParamForLearn;
            mappingMessageTimeout = 90; // ~3 seconds at 30fps
            selectedParamForLearn.clear();
        }
    }

    // Decrement mapping message timeout
    if (mappingMessageTimeout > 0) {
        mappingMessageTimeout--;
    }

    // Update knob highlights for MIDI learn
    OccultKnob* knobs[] = {
        &positionKnob, &grainSizeKnob, &densityKnob, &pitchKnob,
        &sprayKnob, &panSpreadKnob, &grainAttackKnob, &grainReleaseKnob,
        &voiceAttackKnob, &voiceDecayKnob, &voiceSustainKnob, &voiceReleaseKnob,
        &lfoRateKnob, &lfoAmountKnob,
        &delayTimeKnob, &flutterKnob, &hissKnob,
        &damageKnob, &lifeKnob,
        &reverbKnob, &feedbackKnob, &mixKnob, &outputKnob
    };

    // Update modulation button states
    updateModButtonStates();

    for (auto* knob : knobs) {
        juce::String paramId = getParamIdForKnob(knob);
        bool shouldHighlight = midiLearnMode && (selectedParamForLearn == paramId);
        knob->setHighlighted(shouldHighlight);
    }

    // Repaint for MIDI learn animation
    if (midiLearnMode) {
        repaint();
    }
}

int PalaceAudioProcessorEditor::keyToMidiNote(int keyCode) const {
    // Map QWERTY keys to piano layout
    // White keys: A S D F G H J K = C D E F G A B C
    // Black keys: W E   T Y U     = C# D#  F# G# A#
    int baseNote = keyboardOctave * 12;  // C of current octave

    // Convert to uppercase for matching
    int key = std::toupper(keyCode);

    switch (key) {
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

    // Only process keyboard MIDI when active
    if (!keyboardActive)
        return false;

    // Normalize to uppercase for consistent handling
    int normalizedKey = std::toupper(keyCode);

    // Octave switching with Z and X
    if (normalizedKey == 'Z') {
        if (keyboardOctave > 0) {
            keyboardOctave--;
            octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
        }
        return true;
    }
    if (normalizedKey == 'X') {
        if (keyboardOctave < 8) {
            keyboardOctave++;
            octaveLabel.setText("OCT: " + juce::String(keyboardOctave), juce::dontSendNotification);
        }
        return true;
    }

    // Check if it's a note key and not already pressed
    int midiNote = keyToMidiNote(normalizedKey);
    if (midiNote >= 0 && midiNote <= 127) {
        if (activeKeys.find(normalizedKey) == activeKeys.end()) {
            activeKeys.insert(normalizedKey);
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
        // Check both uppercase and lowercase versions
        bool isDown = juce::KeyPress::isKeyCurrentlyDown(keyCode) ||
                      juce::KeyPress::isKeyCurrentlyDown(std::tolower(keyCode));
        if (!isDown) {
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

void PalaceAudioProcessorEditor::showMidiDeviceMenu() {
    juce::PopupMenu menu;

    menu.addSectionHeader("MIDI Learn");
    menu.addItem(1, "Enable MIDI Learn", true, midiLearnMode);
    menu.addItem(2, "Clear All Mappings");

    menu.addSeparator();
    menu.addSectionHeader("Save/Load");
    menu.addItem(3, "Save Mappings...");
    menu.addItem(4, "Load Mappings...");
    menu.addItem(5, "Auto-save on Exit", true, midiAutoSave);

    // Show current mappings
    const auto& mappings = audioProcessor.getMidiMappings();
    if (!mappings.empty()) {
        menu.addSeparator();
        menu.addSectionHeader("Current Mappings (" + juce::String(mappings.size()) + ")");
        int itemId = 100;
        for (const auto& mapping : mappings) {
            juce::String text = "CC " + juce::String(mapping.first) + " -> " + mapping.second;
            menu.addItem(itemId++, text, false);
        }
    }

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this](int result) {
            if (result == 1) {
                midiLearnMode = !midiLearnMode;
                if (!midiLearnMode) {
                    selectedParamForLearn.clear();
                    audioProcessor.clearMidiLearnParameter();
                }
                repaint();
            } else if (result == 2) {
                audioProcessor.clearAllMidiMappings();
            } else if (result == 3) {
                // Save mappings
                auto chooser = std::make_unique<juce::FileChooser>(
                    "Save MIDI Mappings",
                    getDefaultMidiMappingsFile().getParentDirectory(),
                    "*.xml"
                );
                auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
                chooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File{}) {
                        saveMidiMappingsToFile(file.withFileExtension("xml"));
                    }
                });
                chooser.release();
            } else if (result == 4) {
                // Load mappings
                auto chooser = std::make_unique<juce::FileChooser>(
                    "Load MIDI Mappings",
                    getDefaultMidiMappingsFile().getParentDirectory(),
                    "*.xml"
                );
                auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
                chooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile()) {
                        loadMidiMappingsFromFile(file);
                    }
                });
                chooser.release();
            } else if (result == 5) {
                // Toggle autosave
                midiAutoSave = !midiAutoSave;
                // Save the autosave preference
                auto settingsFile = getDefaultMidiMappingsFile().getSiblingFile("settings.xml");
                auto xml = std::make_unique<juce::XmlElement>("Settings");
                xml->setAttribute("autoSave", midiAutoSave);
                xml->writeTo(settingsFile);
            }
        });
}

void PalaceAudioProcessorEditor::selectParameterForLearn(const juce::String& paramId) {
    selectedParamForLearn = paramId;
    audioProcessor.setMidiLearnParameter(paramId);
    repaint();
}

juce::String PalaceAudioProcessorEditor::getParamIdForKnob(OccultKnob* knob) {
    if (knob == &positionKnob) return ParamIDs::position;
    if (knob == &grainSizeKnob) return ParamIDs::grainSize;
    if (knob == &densityKnob) return ParamIDs::density;
    if (knob == &pitchKnob) return ParamIDs::pitch;
    if (knob == &sprayKnob) return ParamIDs::spray;
    if (knob == &panSpreadKnob) return ParamIDs::panSpread;
    if (knob == &grainAttackKnob) return ParamIDs::grainAttack;
    if (knob == &grainReleaseKnob) return ParamIDs::grainRelease;
    if (knob == &voiceAttackKnob) return ParamIDs::voiceAttack;
    if (knob == &voiceDecayKnob) return ParamIDs::voiceDecay;
    if (knob == &voiceSustainKnob) return ParamIDs::voiceSustain;
    if (knob == &voiceReleaseKnob) return ParamIDs::voiceRelease;
    if (knob == &lfoRateKnob) return ParamIDs::lfoRate;
    if (knob == &lfoAmountKnob) return ParamIDs::lfoAmount;
    if (knob == &delayTimeKnob) return ParamIDs::delayTime;
    if (knob == &flutterKnob) return ParamIDs::flutter;
    if (knob == &hissKnob) return ParamIDs::tapeHiss;
    if (knob == &damageKnob) return ParamIDs::damage;
    if (knob == &lifeKnob) return ParamIDs::life;
    if (knob == &reverbKnob) return ParamIDs::reverb;
    if (knob == &feedbackKnob) return ParamIDs::feedback;
    if (knob == &mixKnob) return ParamIDs::mix;
    if (knob == &outputKnob) return ParamIDs::output;
    if (knob == &sampleGainKnob) return ParamIDs::sampleGain;
    return {};
}

void PalaceAudioProcessorEditor::drawMidiStatus(juce::Graphics& g, juce::Rectangle<int> bounds) {
    // Draw status bar background
    g.setColour(OccultLookAndFeel::panelDark.withAlpha(0.9f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(OccultLookAndFeel::amber.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    g.setFont(11.0f);

    // Show mapping success message if recent
    if (mappingMessageTimeout > 0) {
        g.setColour(OccultLookAndFeel::amber);
        g.drawText(lastMappingMessage, bounds.reduced(10, 0), juce::Justification::centred);
        return;
    }

    // Build status text
    juce::String statusText = "MIDI LEARN: ";

    if (selectedParamForLearn.isNotEmpty()) {
        statusText += "Waiting for CC -> " + selectedParamForLearn;
    } else {
        statusText += "Click a knob to select";
    }

    g.setColour(OccultLookAndFeel::textLight);
    g.drawText(statusText, bounds.reduced(10, 0), juce::Justification::centredLeft);

    // Show number of mappings
    const auto& mappings = audioProcessor.getMidiMappings();
    if (!mappings.empty()) {
        g.setColour(OccultLookAndFeel::textDim);
        g.drawText(juce::String(mappings.size()) + " mapped", bounds.reduced(10, 0), juce::Justification::centredRight);
    }
}

void PalaceAudioProcessorEditor::drawMidiDebug(juce::Graphics& g, juce::Rectangle<int> bounds) {
    // Draw debug bar background
    g.setColour(OccultLookAndFeel::backgroundDark.withAlpha(0.95f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    g.setColour(OccultLookAndFeel::metalDark);
    g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);

    g.setFont(10.0f);

    int msgCount = audioProcessor.getMidiMessageCount();
    int midiType = audioProcessor.getLastMidiType();
    int midiChannel = audioProcessor.getLastMidiChannel();
    int midiData1 = audioProcessor.getLastMidiData1();
    int midiData2 = audioProcessor.getLastMidiData2();

    // Show message count (to verify MIDI is coming through at all)
    juce::String debugText = "MIDI Debug - Messages: " + juce::String(msgCount);

    if (msgCount > 0) {
        // Type name
        juce::String typeName;
        switch (midiType) {
            case 1: typeName = "NoteOn"; break;
            case 2: typeName = "NoteOff"; break;
            case 3: typeName = "CC"; break;
            case 4: typeName = "Other"; break;
            default: typeName = "None"; break;
        }

        debugText += "  |  Last: " + typeName;
        debugText += " Ch:" + juce::String(midiChannel);
        debugText += " D1:" + juce::String(midiData1);
        debugText += " D2:" + juce::String(midiData2);

        // Flash indicator for recent activity
        if (msgCount != lastMessageCount) {
            g.setColour(OccultLookAndFeel::amber);
            g.fillEllipse(bounds.getX() + 8.0f, bounds.getCentreY() - 4.0f, 8.0f, 8.0f);
        }
    } else {
        debugText += "  |  No MIDI received (check Options > Audio/MIDI Settings)";
    }

    g.setColour(OccultLookAndFeel::textDim);
    g.drawText(debugText, bounds.reduced(20, 0), juce::Justification::centredLeft);

    lastMessageCount = msgCount;
}

juce::File PalaceAudioProcessorEditor::getDefaultMidiMappingsFile() {
    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                        .getChildFile("SunshineRecorder");
    if (!appDataDir.exists()) {
        appDataDir.createDirectory();
    }
    return appDataDir.getChildFile("midi_mappings.xml");
}

void PalaceAudioProcessorEditor::saveMidiMappings() {
    saveMidiMappingsToFile(getDefaultMidiMappingsFile());
}

void PalaceAudioProcessorEditor::loadMidiMappings() {
    auto file = getDefaultMidiMappingsFile();
    if (file.existsAsFile()) {
        loadMidiMappingsFromFile(file);
    }
}

void PalaceAudioProcessorEditor::saveMidiMappingsToFile(const juce::File& file) {
    auto xml = std::make_unique<juce::XmlElement>("MidiMappings");

    const auto& mappings = audioProcessor.getMidiMappings();
    for (const auto& mapping : mappings) {
        auto* mappingElement = xml->createNewChildElement("Mapping");
        mappingElement->setAttribute("cc", mapping.first);
        mappingElement->setAttribute("param", mapping.second);
    }

    xml->writeTo(file);
}

void PalaceAudioProcessorEditor::loadMidiMappingsFromFile(const juce::File& file) {
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || !xml->hasTagName("MidiMappings")) {
        return;
    }

    audioProcessor.clearAllMidiMappings();

    for (auto* mappingElement : xml->getChildWithTagNameIterator("Mapping")) {
        int cc = mappingElement->getIntAttribute("cc", -1);
        juce::String param = mappingElement->getStringAttribute("param");
        if (cc >= 0 && param.isNotEmpty()) {
            audioProcessor.setMidiMapping(cc, param);
        }
    }
}

void PalaceAudioProcessorEditor::loadAutoSaveSettings() {
    auto settingsFile = getDefaultMidiMappingsFile().getSiblingFile("settings.xml");
    if (settingsFile.existsAsFile()) {
        auto xml = juce::XmlDocument::parse(settingsFile);
        if (xml != nullptr && xml->hasTagName("Settings")) {
            midiAutoSave = xml->getBoolAttribute("autoSave", true);
        }
    }
}

void PalaceAudioProcessorEditor::createModButton(const juce::String& paramId) {
    auto button = std::make_unique<juce::TextButton>("M");
    button->setColour(juce::TextButton::buttonColourId, OccultLookAndFeel::panelDark);
    button->setColour(juce::TextButton::buttonOnColourId, OccultLookAndFeel::amber);
    button->setColour(juce::TextButton::textColourOffId, OccultLookAndFeel::textDim);
    button->setColour(juce::TextButton::textColourOnId, OccultLookAndFeel::backgroundDark);
    button->setClickingTogglesState(true);
    button->setToggleState(audioProcessor.isLfoModulated(paramId), juce::dontSendNotification);

    button->onClick = [this, paramId]() {
        bool enabled = modButtons[paramId]->getToggleState();
        audioProcessor.setLfoModulation(paramId, enabled);
    };

    addAndMakeVisible(*button);
    modButtons[paramId] = std::move(button);
}

void PalaceAudioProcessorEditor::updateModButtonStates() {
    for (auto& [paramId, button] : modButtons) {
        bool isModulated = audioProcessor.isLfoModulated(paramId);
        if (button->getToggleState() != isModulated) {
            button->setToggleState(isModulated, juce::dontSendNotification);
        }
    }
}

} // namespace palace
