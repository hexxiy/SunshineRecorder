# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SunshineRecorder is a JUCE-based VST3/Standalone granular synthesizer plugin with tape delay effects. The plugin features 8-voice polyphony, per-voice granular synthesis engines, LFO modulation, MIDI learn, and a custom "occult" themed UI.

## Build Commands

```bash
# Configure and build (from project root)
cmake -B build -S .
cmake --build build

# Install plugin (copies to system VST3 directory)
cmake --build build --target install

# Clean build
rm -rf build
```

The built plugin will be copied to `~/.vst3/` on Linux automatically via `COPY_PLUGIN_AFTER_BUILD`.

## Architecture Overview

### Plugin Structure (JUCE AudioProcessor Pattern)

- **PluginProcessor** (`Source/PluginProcessor.h/cpp`): Main audio processing engine
  - Manages 8 polyphonic `Voice` instances (static array, no dynamic allocation in audio thread)
  - Holds shared `SampleBuffer` for loaded audio samples
  - Contains `TapeDelay` and JUCE `Reverb` for post-processing effects
  - Manages `AudioProcessorValueTreeState` (apvts) for all parameters
  - Implements MIDI learn system with persistent storage

- **PluginEditor** (`Source/PluginEditor.h/cpp`): UI rendering and user interaction
  - 30 Hz timer-driven UI updates (reads processor state, updates visualizers)
  - No direct audio thread access - all communication via thread-safe atomics or locks

### DSP Architecture (Source/DSP/)

The audio processing hierarchy is:

```
PluginProcessor
├─ Voice (8 instances) - Polyphonic voice management with ADSR envelope
│  └─ GrainEngine - Granular synthesis engine (up to 128 overlapping grains per voice)
│     └─ Grain - Individual grain playback with interpolation and envelope
│        └─ SampleBuffer (shared, read-only) - Thread-safe sample storage
├─ TapeDelay - Analog-style tape delay with flutter, hiss, feedback saturation
├─ Reverb - JUCE spring reverb
└─ LFO - Modulation source (5 waveforms: sine, triangle, square, noise, S&H)
```

**Key Design Patterns:**

1. **Voice Management**: Voice stealing uses LRU-style aging. Voices are never dynamically allocated.
   - Order: Find free voice → Find voice for same MIDI note → Steal oldest/releasing voice

2. **Grain Scheduling**: Each `GrainEngine` maintains sample-accurate grain trigger timing
   - New grains spawn based on `density` parameter (grains per second)
   - Grains read from shared `SampleBuffer` with linear interpolation
   - Grain envelope uses sine/cosine shaping for smooth attack/release

3. **Thread Safety**:
   - Parameters use `std::atomic<float>*` pointers from apvts for lock-free reads in audio thread
   - `SampleBuffer` uses mutex only for file loading; audio thread reads lock-free
   - MIDI mappings use `juce::CriticalSection` when accessed from UI thread

4. **LFO Modulation System**: Post-parameter modulation applied in `updateVoiceParameters()`
   - Modulation targets stored in `std::set<juce::String> lfoModulationTargets`
   - LFO updates once per audio block at middle sample for CPU efficiency
   - Modulation formula: `baseValue + lfoValue * (lfoAmount/100) * range`

### UI Architecture (Source/UI/)

- **OccultLookAndFeel**: Custom JUCE LookAndFeel with dark occult theme
  - Colors: Dark grays, amber accents (#d4a574), metal textures
  - Overrides rotary slider, button, and label rendering

- **OccultKnob**: Custom knob component with MIDI learn highlight and LFO modulation visualization

- **Visualizers**:
  - **WaveformDisplay**: Shows loaded sample with grain position/size overlay, zoom/pan, crop handles, drag-and-drop loading
  - **GrainVisualizer**: Animated particle effect based on active grain count
  - **LFOVisualizer**: Real-time LFO waveform display

**Performance Optimization**: `WaveformDisplay` caches the waveform path and only regenerates when zoom, pan, or sample changes.

### Parameter System (Source/Parameters/)

- **ParamIDs namespace**: Compile-time string constants for all 21 parameters
- **Parameters class**: Wraps apvts with member pointers to `std::atomic<float>*` for thread-safe access
- **Parameter categories**:
  - Grain parameters (8): position, grainSize, density, pitch, spray, panSpread, grainAttack, grainRelease
  - Voice ADSR (4): voiceAttack, voiceDecay, voiceSustain, voiceRelease
  - LFO (3): lfoRate, lfoWaveform, lfoAmount
  - Tape Delay (3): delayTime, flutter, tapeHiss
  - Effects (2): reverb, feedback
  - Output (2): mix, output

## Important Implementation Details

### TapeDelay Effect (Source/DSP/TapeDelay.h/cpp)

Complex analog modeling with:
- **Hermite interpolation** (4-point) for smooth delay time modulation
- **Flutter simulation**: Two LFO rates (3.8 Hz and 5.7 Hz) mixed for natural wow
- **Hiss simulation**: Gaussian noise generator at adjustable level
- **DC blocking filter**: High-pass filter prevents DC offset accumulation
- **Delay time smoothing**: Exponential smoothing (coefficient: 0.001) for gradual transitions

### MIDI Learn System

- Toggle learn mode via MIDI indicator in UI
- Click a knob to select parameter
- Move MIDI CC controller to assign mapping
- Mappings persist to file (XML) and reload on plugin initialization
- UI feedback: Selected knob highlights, MIDI indicator pulses

### Keyboard Input

QWERTY keyboard mapped to virtual MIDI:
- White keys: A S D F G H J K (C D E F G A B C)
- Black keys: W E T Y U (C# D# F# G# A#)
- Octave control: Z (down), X (up)
- Toggle via keyboard icon in UI

### State Serialization

Custom ValueTree nodes stored beyond apvts parameters:
- Sample file path (relative to project if possible)
- Crop region (start/end as 0-1 normalized floats)
- MIDI CC mappings (cc→paramID map)
- LFO modulation routing (set of enabled parameter IDs)

## Code Conventions

### DSP Code (Source/DSP/)
- All audio processing is lock-free - no mutex/critical section locks in process() methods
- Use atomic loads for reading parameters: `parameters.density->load()`
- Grain envelope values calculated per-sample for anti-aliasing
- All buffer access with bounds checking via wrapping (modulo)

### UI Code (Source/UI/ and PluginEditor)
- UI updates at 30 Hz via timer callback
- Never call processor methods that modify audio state from UI thread
- Read-only access to processor via getters and atomic parameter values
- Use `juce::ScopedLock` when accessing complex data structures (MIDI mappings, LFO routing)

### Parameter Access Pattern
```cpp
// In audio thread (PluginProcessor::processBlock)
float density = parameters.density->load();  // Lock-free atomic load

// In UI thread (PluginEditor::timerCallback)
float displayValue = processor.getParameters().lfoAmount->load();  // Safe read

// For complex data (UI thread only)
juce::ScopedLock sl(processor.getMidiMappingLock());
auto mappings = processor.getMidiMappings();
```

## Dependencies

- **JUCE Framework**: Included as git submodule in `JUCE/` directory
- **CMake**: Minimum version 3.22
- **C++17**: Required standard

The project explicitly disables JUCE web browser and CURL to avoid external dependencies:
- `JUCE_WEB_BROWSER=0`
- `JUCE_USE_CURL=0`

## Common Development Scenarios

### Adding a New Parameter

1. Add ID constant to `ParamIDs` namespace in `Source/Parameters/Parameters.h`
2. Add member pointer to `Parameters` class
3. Create parameter in `createParameterLayout()` function
4. Attach pointer in `Parameters::attachParameters()`
5. Read parameter value in `PluginProcessor::updateVoiceParameters()` or relevant processing method
6. Add UI control in `PluginEditor` constructor and attach via `SliderAttachment` or `ButtonAttachment`

### Adding LFO Modulation to a Parameter

1. Ensure parameter is in `ParamIDs` namespace
2. Add modulation button in `PluginEditor::resized()` next to knob
3. Button callback should call `processor.setLFOModulationEnabled(paramID, state)`
4. Modulation automatically applied in `PluginProcessor::updateVoiceParameters()`

### Modifying Grain Behavior

1. Update `GrainEngineParameters` struct in `Source/DSP/GrainEngine.h` if adding parameters
2. Modify `Grain::process()` in `Source/DSP/Grain.cpp` for per-sample grain processing
3. Modify `GrainEngine::process()` for grain scheduling/triggering logic
4. Update `Voice::setGrainParameters()` to pass new parameters to grain engine

### Adding a New Effect

1. Create effect class in `Source/DSP/` (see `TapeDelay.h/cpp` as example)
2. Add instance as member of `PluginProcessor`
3. Add parameters following "Adding a New Parameter" steps
4. Call effect's `process()` method in `PluginProcessor::processBlock()` after voice mixing
5. Add UI controls in `PluginEditor`

## File Organization

```
Source/
├── PluginProcessor.h/cpp     # Main audio processor, voice management, effects chain
├── PluginEditor.h/cpp         # Main UI, layout, timer updates, MIDI learn UI
├── DSP/
│   ├── Grain.h/cpp           # Individual grain playback with envelope
│   ├── GrainEngine.h/cpp     # Grain pool management and scheduling
│   ├── Voice.h/cpp           # Polyphonic voice with ADSR envelope
│   ├── SampleBuffer.h/cpp    # Thread-safe sample storage
│   ├── TapeDelay.h/cpp       # Tape delay effect with flutter/hiss
│   └── LFO.h                 # LFO modulation source (header-only)
├── Parameters/
│   └── Parameters.h/cpp      # Parameter definitions and layout
└── UI/
    ├── OccultLookAndFeel.h/cpp    # Custom theme
    ├── OccultKnob.h/cpp            # Custom knob with modulation display
    ├── WaveformDisplay.h/cpp       # Sample waveform with grain overlay
    ├── GrainVisualizer.h/cpp       # Animated grain particle display
    └── LFOVisualizer.h             # LFO waveform display (header-only)
```

## Testing the Plugin

1. Build and install using commands above
2. Open in DAW that supports VST3 plugins, or run Standalone build
3. Load a sample via drag-and-drop onto waveform display or "Load Sample" button
4. Send MIDI notes (or use QWERTY keyboard mode) to trigger voices
5. Adjust grain parameters (position, size, density, pitch) to shape sound
6. Test LFO modulation by enabling "M" buttons next to parameters
7. Test MIDI learn by clicking MIDI indicator, selecting knob, moving CC controller


## Workflow Orchestration

### 1. Plan Mode Default
- Enter plan mode for ANY non-trivial task (3+ steps or architectural decisions)
- If something goes sideways, STOP and re-plan immediately - don't keep pushing
- Use plan mode for verification steps, not just building
- Write detailed specs upfront to reduce ambiguity

### 2. Subagent Strategy to keep main context window clean
- Offload research, exploration, and parallel analysis to subagents
- For complex problems, throw more compute at it via subagents
- One task per subagent for focused execution

### 3. Self-Improvement Loop
- After ANY correction from the user: update 'tasks/lessons.md' with the pattern
- Write rules for yourself that prevent the same mistake
- Ruthlessly iterate on these lessons until mistake rate drops
- Review lessons at session start for relevant project

### 4. Verification Before Done
- Never mark a task complete without proving it works
- Diff behavior between main and your changes when relevant
- Ask yourself: "Would a staff engineer approve this?"
- Run tests, check logs, demonstrate correctness

### 5. Demand Elegance (Balanced)
- For non-trivial changes: pause and ask "is there a more elegant way?"
- If a fix feels hacky: "Knowing everything I know now, implement the elegant solution"
- Skip this for simple, obvious fixes - don't over-engineer
- Challenge your own work before presenting it

### 6. Autonomous Bug Fixing
- When given a bug report: just fix it. Don't ask for hand-holding
- Point at logs, errors, failing tests -> then resolve them
- Zero context switching required from the user
- Go fix failing CI tests without being told how

## Task Management
1. **Plan First**: Write plan to 'tasks/todo.md' with checkable items
2. **Verify Plan**: Check in before starting implementation
3. **Track Progress**: Mark items complete as you go
4. **Explain Changes**: High-level summary at each step
5. **Document Results**: Add review to 'tasks/todo.md'
6. **Capture Lessons**: Update 'tasks/lessons.md' after corrections

## Core Principles
- **Simplicity First**: Make every change as simple as possible. Impact minimal code.
- **No Laziness**: Find root causes. No temporary fixes. Senior developer standards.
- **Minimal Impact**: Changes should only touch what's necessary. Avoid introducing bugs.

