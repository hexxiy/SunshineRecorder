// AUTO-GENERATED from SR.svg - Review before integrating
// Constructor initialization

// Setup knobs with parameter attachments
setupKnob(sampleGainKnob, ParamIDs::sampleGain, "GAIN");
setupKnob(positionKnob, ParamIDs::position, "POSITION");
setupKnob(grainSizeKnob, ParamIDs::grainSize, "SIZE");
setupKnob(densityKnob, ParamIDs::density, "DENSITY");
setupKnob(pitchKnob, ParamIDs::pitch, "PITCH");
setupKnob(sprayKnob, ParamIDs::spray, "SPRAY");
setupKnob(panSpreadKnob, ParamIDs::panSpread, "PAN");
setupKnob(grainAttackKnob, ParamIDs::grainAttack, "G.ATK");
setupKnob(grainReleaseKnob, ParamIDs::grainRelease, "G.REL");
setupKnob(voiceAttackKnob, ParamIDs::voiceAttack, "ATTACK");
setupKnob(voiceDecayKnob, ParamIDs::voiceDecay, "DECAY");
setupKnob(voiceSustainKnob, ParamIDs::voiceSustain, "SUSTAIN");
setupKnob(voiceReleaseKnob, ParamIDs::voiceRelease, "RELEASE");
setupKnob(lfoRateKnob, ParamIDs::lfoRate, "RATE");
setupKnob(lfoAmountKnob, ParamIDs::lfoAmount, "AMOUNT");
setupKnob(delayTimeKnob, ParamIDs::delayTime, "DELAY");
setupKnob(flutterKnob, ParamIDs::flutter, "FLUTTER");
setupKnob(hissKnob, ParamIDs::tapeHiss, "HISS");
setupKnob(damageKnob, ParamIDs::damage, "DAMAGE");
setupKnob(lifeKnob, ParamIDs::life, "LIFE");
setupKnob(reverbKnob, ParamIDs::reverb, "REVERB");
setupKnob(feedbackKnob, ParamIDs::feedback, "FEEDBACK");
setupKnob(mixKnob, ParamIDs::mix, "MIX");
setupKnob(outputKnob, ParamIDs::output, "OUTPUT");

// Add components to visible
addAndMakeVisible(sampleGainKnob);
addAndMakeVisible(positionKnob);
addAndMakeVisible(grainSizeKnob);
addAndMakeVisible(densityKnob);
addAndMakeVisible(pitchKnob);
addAndMakeVisible(sprayKnob);
addAndMakeVisible(panSpreadKnob);
addAndMakeVisible(grainAttackKnob);
addAndMakeVisible(grainReleaseKnob);
addAndMakeVisible(voiceAttackKnob);
addAndMakeVisible(voiceDecayKnob);
addAndMakeVisible(voiceSustainKnob);
addAndMakeVisible(voiceReleaseKnob);
addAndMakeVisible(lfoRateKnob);
addAndMakeVisible(lfoAmountKnob);
addAndMakeVisible(delayTimeKnob);
addAndMakeVisible(flutterKnob);
addAndMakeVisible(hissKnob);
addAndMakeVisible(damageKnob);
addAndMakeVisible(lifeKnob);
addAndMakeVisible(reverbKnob);
addAndMakeVisible(feedbackKnob);
addAndMakeVisible(mixKnob);
addAndMakeVisible(outputKnob);
addAndMakeVisible(zoomInButton);
addAndMakeVisible(zoomOutButton);
addAndMakeVisible(waveformDisplay);
addAndMakeVisible(loadButton);
addAndMakeVisible(grainVisualizer);
addAndMakeVisible(lfoVisualizer);
addAndMakeVisible(lfoWaveformBox);
addAndMakeVisible(resetDamageButton);
addAndMakeVisible(octaveDownButton);
addAndMakeVisible(octaveUpButton);