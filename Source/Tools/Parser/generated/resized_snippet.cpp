// AUTO-GENERATED from SR.svg - Review before integrating
// resized() method layout

const float scaleX = getWidth() / 850.0f;
const float scaleY = getHeight() / 720.0f;
const int knobWidth = static_cast<int>(85 * scaleX);
const int knobHeight = static_cast<int>(100 * scaleY);

// Envelope
voiceAttackKnob.setBounds(
    static_cast<int>(567 * scaleX) - knobWidth/2,  // SVG: 150.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
voiceDecayKnob.setBounds(
    static_cast<int>(650 * scaleX) - knobWidth/2,  // SVG: 172.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
voiceSustainKnob.setBounds(
    static_cast<int>(567 * scaleX) - knobWidth/2,  // SVG: 150.0mm
    static_cast<int>(352 * scaleY) - knobHeight/2, // SVG: 93.0mm
    knobWidth,
    knobHeight
);
voiceReleaseKnob.setBounds(
    static_cast<int>(650 * scaleX) - knobWidth/2,  // SVG: 172.0mm
    static_cast<int>(352 * scaleY) - knobHeight/2, // SVG: 93.0mm
    knobWidth,
    knobHeight
);

// Grain Controls
positionKnob.setBounds(
    static_cast<int>(64 * scaleX) - knobWidth/2,  // SVG: 17.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
grainSizeKnob.setBounds(
    static_cast<int>(147 * scaleX) - knobWidth/2,  // SVG: 39.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
densityKnob.setBounds(
    static_cast<int>(230 * scaleX) - knobWidth/2,  // SVG: 61.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
pitchKnob.setBounds(
    static_cast<int>(314 * scaleX) - knobWidth/2,  // SVG: 83.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
sprayKnob.setBounds(
    static_cast<int>(397 * scaleX) - knobWidth/2,  // SVG: 105.0mm
    static_cast<int>(254 * scaleY) - knobHeight/2, // SVG: 67.0mm
    knobWidth,
    knobHeight
);
panSpreadKnob.setBounds(
    static_cast<int>(64 * scaleX) - knobWidth/2,  // SVG: 17.0mm
    static_cast<int>(352 * scaleY) - knobHeight/2, // SVG: 93.0mm
    knobWidth,
    knobHeight
);
grainAttackKnob.setBounds(
    static_cast<int>(147 * scaleX) - knobWidth/2,  // SVG: 39.0mm
    static_cast<int>(352 * scaleY) - knobHeight/2, // SVG: 93.0mm
    knobWidth,
    knobHeight
);
grainReleaseKnob.setBounds(
    static_cast<int>(230 * scaleX) - knobWidth/2,  // SVG: 61.0mm
    static_cast<int>(352 * scaleY) - knobHeight/2, // SVG: 93.0mm
    knobWidth,
    knobHeight
);

// LFO
lfoRateKnob.setBounds(
    static_cast<int>(72 * scaleX) - knobWidth/2,  // SVG: 19.0mm
    static_cast<int>(549 * scaleY) - knobHeight/2, // SVG: 145.0mm
    knobWidth,
    knobHeight
);
lfoAmountKnob.setBounds(
    static_cast<int>(155 * scaleX) - knobWidth/2,  // SVG: 41.0mm
    static_cast<int>(549 * scaleY) - knobHeight/2, // SVG: 145.0mm
    knobWidth,
    knobHeight
);
lfoVisualizer.setBounds(
    static_cast<int>(34 * scaleX),  // SVG: 9.0mm
    static_cast<int>(447 * scaleY),  // SVG: 118.0mm
    static_cast<int>(166 * scaleX),
    static_cast<int>(49 * scaleY)
);
lfoWaveformBox.setBounds(
    static_cast<int>(42 * scaleX),  // SVG: 11.0mm
    static_cast<int>(603 * scaleY),  // SVG: 159.0mm
    static_cast<int>(151 * scaleX),
    static_cast<int>(23 * scaleY)
);

// Output
reverbKnob.setBounds(
    static_cast<int>(559 * scaleX) - knobWidth/2,  // SVG: 148.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
feedbackKnob.setBounds(
    static_cast<int>(642 * scaleX) - knobWidth/2,  // SVG: 170.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
mixKnob.setBounds(
    static_cast<int>(725 * scaleX) - knobWidth/2,  // SVG: 192.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
outputKnob.setBounds(
    static_cast<int>(808 * scaleX) - knobWidth/2,  // SVG: 214.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
octaveDownButton.setBounds(
    static_cast<int>(597 * scaleX),  // SVG: 158.0mm
    static_cast<int>(561 * scaleY),  // SVG: 148.0mm
    static_cast<int>(30 * scaleX),
    static_cast<int>(25 * scaleY)
);
octaveUpButton.setBounds(
    static_cast<int>(710 * scaleX),  // SVG: 188.0mm
    static_cast<int>(561 * scaleY),  // SVG: 148.0mm
    static_cast<int>(30 * scaleX),
    static_cast<int>(25 * scaleY)
);

// Tape Delay
delayTimeKnob.setBounds(
    static_cast<int>(268 * scaleX) - knobWidth/2,  // SVG: 71.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
flutterKnob.setBounds(
    static_cast<int>(351 * scaleX) - knobWidth/2,  // SVG: 93.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
hissKnob.setBounds(
    static_cast<int>(434 * scaleX) - knobWidth/2,  // SVG: 115.0mm
    static_cast<int>(496 * scaleY) - knobHeight/2, // SVG: 131.0mm
    knobWidth,
    knobHeight
);
damageKnob.setBounds(
    static_cast<int>(268 * scaleX) - knobWidth/2,  // SVG: 71.0mm
    static_cast<int>(595 * scaleY) - knobHeight/2, // SVG: 157.0mm
    knobWidth,
    knobHeight
);
lifeKnob.setBounds(
    static_cast<int>(351 * scaleX) - knobWidth/2,  // SVG: 93.0mm
    static_cast<int>(595 * scaleY) - knobHeight/2, // SVG: 157.0mm
    knobWidth,
    knobHeight
);
resetDamageButton.setBounds(
    static_cast<int>(389 * scaleX),  // SVG: 103.0mm
    static_cast<int>(644 * scaleY),  // SVG: 170.0mm
    static_cast<int>(76 * scaleX),
    static_cast<int>(25 * scaleY)
);

// Waveform
sampleGainKnob.setBounds(
    static_cast<int>(751 * scaleX) - knobWidth/2,  // SVG: 198.8mm
    static_cast<int>(127 * scaleY) - knobHeight/2, // SVG: 33.4mm
    knobWidth,
    knobHeight
);
zoomInButton.setBounds(
    static_cast<int>(19 * scaleX),  // SVG: 5.0mm
    static_cast<int>(87 * scaleY),  // SVG: 23.0mm
    static_cast<int>(26 * scaleX),
    static_cast<int>(27 * scaleY)
);
zoomOutButton.setBounds(
    static_cast<int>(19 * scaleX),  // SVG: 5.0mm
    static_cast<int>(117 * scaleY),  // SVG: 31.0mm
    static_cast<int>(26 * scaleX),
    static_cast<int>(27 * scaleY)
);
waveformDisplay.setBounds(
    static_cast<int>(53 * scaleX),  // SVG: 14.0mm
    static_cast<int>(68 * scaleY),  // SVG: 18.0mm
    static_cast<int>(567 * scaleX),
    static_cast<int>(121 * scaleY)
);
loadButton.setBounds(
    static_cast<int>(631 * scaleX),  // SVG: 167.0mm
    static_cast<int>(68 * scaleY),  // SVG: 18.0mm
    static_cast<int>(57 * scaleX),
    static_cast<int>(30 * scaleY)
);
grainVisualizer.setBounds(
    static_cast<int>(631 * scaleX),  // SVG: 167.0mm
    static_cast<int>(167 * scaleY),  // SVG: 44.0mm
    static_cast<int>(57 * scaleX),
    static_cast<int>(19 * scaleY)
);
