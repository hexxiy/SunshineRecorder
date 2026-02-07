# SVG-to-JUCE Code Converter

A Python tool that parses `SR.svg` (Inkscape/SVG UI design) and generates JUCE C++ code snippets for the PluginEditor layout.

## Purpose

This tool bridges the gap between visual UI design in Inkscape and JUCE code implementation:
- Designers can iterate on the UI layout in `SR.svg`
- Developers run this tool to regenerate component positioning code
- Ensures pixel-perfect consistency between design and implementation

## Files

- **svg_to_juce.py** - Main converter script
- **component_mapping.json** - Configuration mapping SVG IDs to JUCE components
- **SR.svg** - Source SVG layout (225mm × 190mm → 850px × 720px JUCE window)
- **generated/** - Output directory for code snippets (auto-generated)

## Usage

### 1. Initialize Mapping (First Time)

Generate the initial `component_mapping.json` from `SR.svg`:

```bash
python3 svg_to_juce.py --init
```

This scans the SVG and creates a skeleton mapping with suggested component names, types, and parameter IDs. Review and adjust the generated `component_mapping.json` to match your actual PluginEditor code.

### 2. Generate Code Snippets

After mapping is configured, generate the JUCE C++ code snippets:

```bash
python3 svg_to_juce.py
```

This creates three files in `generated/`:
- **header_snippet.hpp** - Member variable declarations
- **constructor_snippet.cpp** - Component initialization and `setupKnob()` calls
- **resized_snippet.cpp** - Layout code with `setBounds()` calls

### 3. Validate Configuration

Check for issues before generating code:

```bash
python3 svg_to_juce.py --validate
```

This reports:
- Unmapped SVG components
- Components with `Unknown` type
- Position overlaps (< 2mm apart)
- Components outside SVG bounds

### 4. Dry Run (Preview)

Preview generated code without writing files:

```bash
python3 svg_to_juce.py --dry-run
```

## Integration Workflow

1. **Edit UI in Inkscape**: Modify `SR.svg` with new component positions/layouts
2. **Regenerate code**: Run `python3 svg_to_juce.py`
3. **Review snippets**: Check `generated/` directory for output
4. **Manually integrate**: Copy relevant sections into `PluginEditor.h` and `PluginEditor.cpp`
5. **Build and test**: Compile plugin and verify layout

**Important**: Generated snippets are meant for **manual integration**, not direct file replacement. This preserves existing business logic (MIDI learn, callbacks, timers) while updating layout coordinates.

## Component Mapping

`component_mapping.json` structure:

```json
{
  "config": {
    "svg_width_mm": 225,
    "svg_height_mm": 190,
    "juce_width_px": 850,
    "juce_height_px": 720
  },
  "components": {
    "knob_position": {
      "type": "OccultKnob",
      "member": "positionKnob",
      "paramId": "ParamIDs::position",
      "label": "POSITION",
      "modulation": true
    },
    "load_btn": {
      "type": "TextButton",
      "member": "loadButton",
      "label": "LOAD"
    }
  }
}
```

### Supported Component Types

- **OccultKnob** - Custom rotary knob with LFO modulation
- **TextButton** - JUCE TextButton
- **ComboBox** - JUCE ComboBox (e.g., LFO waveform selector)
- **WaveformDisplay** - Custom waveform display component
- **GrainVisualizer** - Custom grain particle visualizer
- **LFOVisualizer** - Custom LFO waveform display
- **ModButton** - Modulation enable buttons (skipped, handled by existing `modButtons` map)
- **Indicator** - UI indicators (skipped, decorative only)
- **Unknown** - Unmapped or decorative elements (skipped)

## SVG Component Naming Conventions

The tool auto-detects component types based on SVG element IDs:

- `knob_*` (circle) → OccultKnob
- `*_btn` (rect) → TextButton
- `mod_*` (rect) → ModButton
- `*_visualizer` (rect) → Custom visualizer component
- `*_display` (rect) → Custom display component

## Coordinate Conversion

- **SVG**: 225mm × 190mm viewBox
- **JUCE**: 850px × 720px window
- **Scale factors**: scaleX ≈ 3.78, scaleY ≈ 3.79

Generated code uses dynamic scaling:
```cpp
const float scaleX = getWidth() / 850.0f;
const float scaleY = getHeight() / 720.0f;
```

Knobs (circles) are positioned from center, adjusted to top-left corner in generated code.

## Advantages

- **Fast iteration**: Designer updates SVG, developer regenerates code
- **Consistency**: UI layout matches visual design exactly
- **Documentation**: SVG serves as visual spec
- **Maintainability**: JSON config is human-readable
- **Non-destructive**: Generated snippets don't overwrite logic

## Troubleshooting

### "ERROR: No mapping configuration found"

Run `python3 svg_to_juce.py --init` first to generate `component_mapping.json`.

### Components Missing from Generated Code

Check if component type is `Unknown`, `ModButton`, or `Indicator` in `component_mapping.json` - these are intentionally skipped.

### Coordinates Don't Match SVG

Verify `config` section in `component_mapping.json` matches your SVG dimensions and JUCE window size.

### New Components Not Appearing

1. Run `--init` to regenerate mapping (preserves existing entries)
2. Or manually add new components to `component_mapping.json`
3. Run `--validate` to check for unmapped components

## Future Enhancements

- Auto-detect new components and suggest ParamIDs
- Generate section border drawing code
- Support for component variants (big knob, small knob)
- Inkscape plugin for live preview
- CI integration: detect SVG/code drift
- Coordinate accuracy comparison with existing PluginEditor code

## Requirements

- Python 3.7+
- No external dependencies (uses standard library only)
