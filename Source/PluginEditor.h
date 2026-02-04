#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/OccultLookAndFeel.h"
#include "UI/OccultKnob.h"
#include "UI/WaveformDisplay.h"
#include "UI/GrainVisualizer.h"
#include "UI/LFOVisualizer.h"
#include <set>

namespace palace {

class PalaceAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    public juce::Timer,
                                    public juce::KeyListener {
public:
    explicit PalaceAudioProcessorEditor(PalaceAudioProcessor&);
    ~PalaceAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;

private:
    void setupKnob(OccultKnob& knob, const juce::String& paramId, const juce::String& label);
    void drawHeader(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawSectionBorder(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title);

    // Keyboard MIDI
    int keyToMidiNote(int keyCode) const;
    void updateKeyboardState();

    PalaceAudioProcessor& audioProcessor;
    OccultLookAndFeel lookAndFeel;

    // Waveform display
    WaveformDisplay waveformDisplay;
    GrainVisualizer grainVisualizer;

    // Load button
    juce::TextButton loadButton{"LOAD"};

    // Zoom buttons
    juce::TextButton zoomInButton{"+"};
    juce::TextButton zoomOutButton{"-"};

    // Grain controls
    OccultKnob positionKnob{"POSITION"};
    OccultKnob grainSizeKnob{"SIZE"};
    OccultKnob densityKnob{"DENSITY"};
    OccultKnob pitchKnob{"PITCH"};
    OccultKnob sprayKnob{"SPRAY"};
    OccultKnob panSpreadKnob{"PAN"};
    OccultKnob grainAttackKnob{"ATTACK"};
    OccultKnob grainReleaseKnob{"RELEASE"};

    // Voice ADSR
    OccultKnob voiceAttackKnob{"ATTACK"};
    OccultKnob voiceDecayKnob{"DECAY"};
    OccultKnob voiceSustainKnob{"SUSTAIN"};
    OccultKnob voiceReleaseKnob{"RELEASE"};

    // LFO
    LFOVisualizer lfoVisualizer;
    OccultKnob lfoRateKnob{"RATE"};
    OccultKnob lfoAmountKnob{"AMOUNT"};
    juce::ComboBox lfoWaveformBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoWaveformAttachment;

    // Modulation buttons (one per modulatable parameter)
    std::map<juce::String, std::unique_ptr<juce::TextButton>> modButtons;
    void createModButton(const juce::String& paramId);
    void updateModButtonStates();

    // Tape Delay
    OccultKnob delayTimeKnob{"DELAY"};
    OccultKnob flutterKnob{"FLUTTER"};
    OccultKnob hissKnob{"HISS"};

    // Tape Disintegration
    OccultKnob damageKnob{"DAMAGE"};
    OccultKnob lifeKnob{"LIFE"};
    juce::TextButton resetDamageButton{"Reset Damage"};

    // Effects
    OccultKnob reverbKnob{"REVERB"};
    OccultKnob feedbackKnob{"FEEDBACK"};

    // Output
    OccultKnob mixKnob{"MIX"};
    OccultKnob outputKnob{"OUTPUT"};

    // Sample
    OccultKnob sampleGainKnob{"GAIN"};

    // Keyboard MIDI state
    int keyboardOctave = 4;  // Middle C octave
    std::set<int> activeKeys;  // Currently held key codes
    juce::Label octaveLabel;
    juce::TextButton octaveDownButton{"-"};
    juce::TextButton octaveUpButton{"+"};
    bool keyboardActive = true;  // Keyboard input enabled (default: on)

    // MIDI learn mode
    bool midiLearnMode = false;
    bool midiAutoSave = true;
    juce::String selectedParamForLearn;
    int lastDisplayedCC = -1;
    int lastMessageCount = 0;
    juce::String lastMappingMessage;
    int mappingMessageTimeout = 0;
    void showMidiDeviceMenu();
    void selectParameterForLearn(const juce::String& paramId);
    juce::String getParamIdForKnob(OccultKnob* knob);
    void drawMidiStatus(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawMidiDebug(juce::Graphics& g, juce::Rectangle<int> bounds);

    // MIDI mapping file operations
    void saveMidiMappings();
    void loadMidiMappings();
    void saveMidiMappingsToFile(const juce::File& file);
    void loadMidiMappingsFromFile(const juce::File& file);
    juce::File getDefaultMidiMappingsFile();
    void loadAutoSaveSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PalaceAudioProcessorEditor)
};

} // namespace palace
