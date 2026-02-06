// AUTO-GENERATED from SR.svg - Review before integrating
// Member variable declarations

// Envelope
OccultKnob voiceAttackKnob{"ATTACK"};
OccultKnob voiceDecayKnob{"DECAY"};
OccultKnob voiceSustainKnob{"SUSTAIN"};
OccultKnob voiceReleaseKnob{"RELEASE"};

// Grain Controls
OccultKnob positionKnob{"POSITION"};
OccultKnob grainSizeKnob{"SIZE"};
OccultKnob densityKnob{"DENSITY"};
OccultKnob pitchKnob{"PITCH"};
OccultKnob sprayKnob{"SPRAY"};
OccultKnob panSpreadKnob{"PAN"};
OccultKnob grainAttackKnob{"G.ATK"};
OccultKnob grainReleaseKnob{"G.REL"};

// LFO
OccultKnob lfoRateKnob{"RATE"};
OccultKnob lfoAmountKnob{"AMOUNT"};
LFOVisualizer lfoVisualizer;
juce::ComboBox lfoWaveformBox;

// Output
OccultKnob reverbKnob{"REVERB"};
OccultKnob feedbackKnob{"FEEDBACK"};
OccultKnob mixKnob{"MIX"};
OccultKnob outputKnob{"OUTPUT"};
juce::TextButton octaveDownButton{"-"};
juce::TextButton octaveUpButton{"+"};

// Tape Delay
OccultKnob delayTimeKnob{"DELAY"};
OccultKnob flutterKnob{"FLUTTER"};
OccultKnob hissKnob{"HISS"};
OccultKnob damageKnob{"DAMAGE"};
OccultKnob lifeKnob{"LIFE"};
juce::TextButton resetDamageButton{"RESET"};

// Waveform
OccultKnob sampleGainKnob{"GAIN"};
juce::TextButton zoomInButton{"+"};
juce::TextButton zoomOutButton{"-"};
WaveformDisplay waveformDisplay;
juce::TextButton loadButton{"LOAD"};
GrainVisualizer grainVisualizer;
