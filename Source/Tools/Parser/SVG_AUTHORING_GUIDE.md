# SVG Authoring Guide for svg_to_juce.py

This guide explains how to structure and format your SVG files in Inkscape to work seamlessly with the `svg_to_juce.py` converter tool. This is a companion to `README.md`, which covers tool usage.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [SVG Document Setup](#svg-document-setup)
3. [Component Naming Conventions](#component-naming-conventions)
4. [Organizing with Groups and Layers](#organizing-with-groups-and-layers)
5. [Component Types](#component-types)
6. [Positioning Guidelines](#positioning-guidelines)
7. [Step-by-Step Workflow](#step-by-step-workflow)
8. [Common Pitfalls](#common-pitfalls)
9. [Advanced Techniques](#advanced-techniques)

---

## Prerequisites

- **Inkscape 1.0+** (tested with 1.4.2)
- Basic Inkscape knowledge (shapes, selection, grouping)
- Understanding of your JUCE plugin's UI component structure

---

## SVG Document Setup

### 1. Document Dimensions

The converter expects specific dimensions matching your JUCE window:

**Default Configuration:**
- **SVG Canvas**: 225mm × 190mm (millimeters)
- **JUCE Window**: 850px × 720px (pixels)
- **Scale Factor**: ~3.78 (px per mm)

**Set up in Inkscape:**

1. Go to **File → Document Properties** (Shift+Ctrl+D)
2. Set **Display units**: mm
3. Set **Width**: 225 mm
4. Set **Height**: 190 mm
5. Set **Viewbox**: 0 0 225 190

### 2. Grid and Guides (Optional but Recommended)

Enable a grid for precise alignment:

1. **Document Properties → Grids → Create Rectangular Grid**
2. Set **Spacing X**: 5 mm
3. Set **Spacing Y**: 5 mm
4. Enable **View → Page Grid** (# key)

Add guides for major sections:
- Drag from rulers (Ctrl+R to show/hide rulers)
- Use guides to define section boundaries

---

## Component Naming Conventions

**The most important aspect of SVG authoring:** Component IDs must follow naming conventions for automatic type detection.

### Naming Rules

Every UI component must have a unique **Object ID** (set via Object → Object Properties, or Ctrl+Shift+O).

| Component Type | Naming Pattern | Example IDs |
|----------------|----------------|-------------|
| **Knobs (rotary controls)** | `knob_*` | `knob_position`, `knob_filter`, `knob_attack` |
| **Buttons** | `*_btn` | `load_btn`, `reset_btn`, `save_btn` |
| **Modulation Buttons** | `mod_*` | `mod_position`, `mod_filter` |
| **Visualizers** | `*_visualizer` | `grain_visualizer`, `lfo_visualizer`, `spectrum_visualizer` |
| **Displays** | `*_display` | `waveform_display`, `meter_display` |
| **Combo Boxes** | `*_box` or `*_dropdown` | `lfo_waveform_box`, `filter_type_box` |
| **Indicators** | `*_indicator` | `midi_indicator`, `kbd_indicator` |
| **Decorative Elements** | Any name (skipped) | `panel_background`, `logo`, `section_border` |

### Setting Object IDs in Inkscape

1. Select the component
2. Open **Object → Object Properties** (Shift+Ctrl+O)
3. Set **ID** field to your component name
4. Optional: Set **Label** field for human-readable name

**Example:**
- **ID**: `knob_cutoff`
- **Label**: "Cutoff Knob"

---

## Organizing with Groups and Layers

### Layer Structure

Organize your SVG into logical layers for maintainability:

```
svg_sunshinerecorder (root)
├─ Panel (background layer)
│  └─ panel_background
├─ Graphics (main layer)
│  ├─ Header
│  ├─ Waveform Section
│  │  ├─ waveform_display
│  │  ├─ load_btn
│  │  └─ zoom_in_btn
│  ├─ Grain Controls
│  │  ├─ knob_position
│  │  ├─ knob_size
│  │  └─ knob_density
│  ├─ ADSR Controls
│  │  ├─ knob_attack
│  │  ├─ knob_decay
│  │  ├─ knob_sustain
│  │  └─ knob_release
│  └─ Effects Section
│     ├─ knob_reverb
│     └─ knob_delay
```

### Creating Groups

Groups help organize related components and provide section context:

1. Select multiple related components (Shift+Click)
2. Group them: **Object → Group** (Ctrl+G)
3. Set the group's **Object Properties**:
   - **ID**: `grain_controls` (group ID)
   - **Label**: "Grain Controls" (human-readable, used for comments in generated code)

### Layer Properties

Right-click layer in **Layers and Objects** panel (Shift+Ctrl+L):

- **inkscape:label**: Set to section name (e.g., "Grain Controls")
- **inkscape:groupmode**: Set to "layer"

The converter reads `inkscape:label` attributes to assign section names to components, which appear as comments in generated code.

---

## Component Types

### 1. Knobs (Circles)

**Shape**: Circle
**Naming**: `knob_*`
**Positioning**: Center point (cx, cy)

**Best Practices:**
- Use consistent radius (e.g., 5mm for standard knobs)
- Position by center, not bounding box
- Keep knobs at least 15mm apart for comfortable spacing

**Creating a Knob:**
1. Draw circle with **Circle Tool** (F5)
2. Set **Object ID**: `knob_position`
3. Set **Radius** (rx = ry): 5mm
4. Position with **cx** and **cy** coordinates

### 2. Buttons (Rectangles)

**Shape**: Rectangle
**Naming**: `*_btn`
**Positioning**: Top-left corner (x, y) + width/height

**Best Practices:**
- Minimum size: 10mm × 4mm for click targets
- Round corners optional (doesn't affect conversion)
- Use consistent height for button rows

**Creating a Button:**
1. Draw rectangle with **Rectangle Tool** (F4)
2. Set **Object ID**: `load_btn`
3. Set **Width**: 15mm, **Height**: 5mm
4. Position with **x** and **y** coordinates

### 3. Displays and Visualizers (Rectangles)

**Shape**: Rectangle
**Naming**: `*_display` or `*_visualizer`
**Positioning**: Top-left corner (x, y) + width/height

**Component Type Detection:**
- `waveform_display` → WaveformDisplay
- `grain_visualizer` → GrainVisualizer
- `lfo_visualizer` → LFOVisualizer
- Generic `*_display` → Unknown (add to mapping manually)

### 4. Decorative Elements

**Purpose**: Visual-only elements (borders, backgrounds, text labels)

**Naming**: Any name (avoid reserved patterns)

**Recommended IDs:**
- `panel_background`
- `section_border_grain`
- `label_text_*`
- `decoration_*`

These are automatically skipped during code generation.

---

## Positioning Guidelines

### Coordinate System

- **Origin**: Top-left corner (0, 0)
- **Units**: Millimeters (mm)
- **Bounds**: X: 0–225mm, Y: 0–190mm

### Alignment Tips

1. **Use Inkscape's Align Tools** (Shift+Ctrl+A):
   - Align centers horizontally for knob rows
   - Distribute spacing evenly for uniform layouts

2. **Snap to Grid**:
   - Enable **View → Snap to Grid**
   - Set grid spacing to 5mm for major alignment
   - Use 1mm grid for fine-tuning

3. **Minimum Spacing**:
   - Knobs: 15mm apart (center to center)
   - Buttons: 2mm gap minimum
   - Components and borders: 3mm margin

### Common Layouts

**Knob Row (Horizontal):**
```
Y: 50mm (all knobs same Y coordinate)
X: 20mm, 45mm, 70mm, 95mm, 120mm (25mm spacing)
```

**Button Group (Vertical Stack):**
```
X: 10mm (all buttons same X coordinate)
Y: 30mm, 38mm, 46mm (8mm spacing)
Width: 15mm (consistent)
```

---

## Step-by-Step Workflow

### Scenario: Adding a New Knob

1. **Draw the Circle:**
   - Select **Circle Tool** (F5)
   - Draw a circle (hold Ctrl for perfect circle)
   - Set radius: 5mm

2. **Position the Knob:**
   - Use **Selector Tool** (F1)
   - Drag to approximate position
   - Fine-tune via **Object → Transform** (Shift+Ctrl+M)
   - Set **cx**: 50mm, **cy**: 80mm

3. **Name the Component:**
   - Open **Object → Object Properties** (Shift+Ctrl+O)
   - Set **ID**: `knob_filter`
   - Set **Label**: "Filter Cutoff Knob" (optional)

4. **Add to Group (Optional):**
   - Select knob + related components
   - Group: **Object → Group** (Ctrl+G)
   - Name group ID: `filter_section`

5. **Save SVG:**
   - **File → Save** (Ctrl+S)
   - Format: Plain SVG or Inkscape SVG (both work)

6. **Generate Code:**
   ```bash
   # If first time with this component:
   python3 svg_to_juce.py --init

   # Generate code snippets:
   python3 svg_to_juce.py
   ```

7. **Review Mapping:**
   - Open `component_mapping.json`
   - Verify auto-detected type and suggested `paramId`
   - Adjust `member` name, `label`, and `paramId` as needed

### Scenario: Updating Component Positions

1. **Edit SVG in Inkscape:**
   - Move components to new positions
   - Adjust sizes if needed
   - Save

2. **Regenerate Code:**
   ```bash
   python3 svg_to_juce.py
   ```

3. **Review Changes:**
   - Check `generated/resized_snippet.cpp`
   - Compare new coordinates with existing code
   - Manually integrate changes into `PluginEditor.cpp`

---

## Common Pitfalls

### 1. Unnamed Components

**Problem:** Components without IDs are ignored.

**Solution:**
- Always set **Object ID** for interactive components
- Run `python3 svg_to_juce.py --validate` to find unmapped components

### 2. Wrong Naming Pattern

**Problem:** Component named `filter_knob` instead of `knob_filter` gets detected as `Unknown`.

**Solution:**
- Follow naming conventions strictly
- Prefix matters: `knob_*` for knobs, `*_btn` for buttons

### 3. Overlapping Components

**Problem:** Two components share the same position, causing click detection issues in UI.

**Solution:**
- Use **View → Wireframe** (Alt+5) to see overlaps
- Run `--validate` to detect components closer than 2mm
- Adjust positions manually

### 4. Components Outside Canvas

**Problem:** Component positioned at X: -5mm or Y: 200mm (outside 225×190 bounds).

**Solution:**
- Check **Object → Transform** for coordinate values
- Use **Edit → Select All in All Layers** (Ctrl+Alt+A) to find stray objects
- Run `--validate` to detect out-of-bounds components

### 5. Groups Interfering with Parsing

**Problem:** Nested groups or incorrectly structured groups cause parser to miss components.

**Solution:**
- Keep group hierarchy shallow (max 2 levels)
- Ensure components are direct children of groups, not nested deeply
- Ungroup and regroup if issues persist: **Object → Ungroup** (Shift+Ctrl+G)

### 6. Inkscape-Specific Features

**Problem:** Using Inkscape extensions, effects, or live path effects that don't export to plain SVG.

**Solution:**
- Convert effects to paths: **Path → Object to Path** (Shift+Ctrl+C)
- Save as **Plain SVG** instead of Inkscape SVG for cleaner output
- Avoid using clones, symbols, or linked objects

---

## Advanced Techniques

### Custom Component Detection

If you have a custom component type not auto-detected, manually edit `component_mapping.json` after running `--init`:

```json
{
  "custom_meter": {
    "type": "CustomMeterComponent",
    "member": "customMeter",
    "label": "METER"
  }
}
```

### Responsive Scaling

The converter generates dynamic scaling code:

```cpp
const float scaleX = getWidth() / 850.0f;
const float scaleY = getHeight() / 720.0f;
```

This means your JUCE plugin can be resized, and components will scale proportionally.

**To maintain aspect ratio:**
- Keep your SVG design's aspect ratio consistent with JUCE window
- 225mm:190mm ≈ 1.18:1 matches 850px:720px ≈ 1.18:1

### Using Inkscape Extensions for Layout

**Distribute Extensions:**
1. Select multiple components
2. **Extensions → Arrange → Distribute Nodes**
3. Set spacing and direction
4. Apply to evenly space knobs or buttons

**Grid Alignment Extension:**
1. **Extensions → Arrange → Layout**
2. Arrange in grid pattern (e.g., 4×2 knob grid)
3. Set margins and spacing

### Version Control Best Practices

**What to commit:**
- `SR.svg` (or your main SVG file)
- `component_mapping.json` (configuration)

**What NOT to commit (add to .gitignore):**
- `generated/` directory (auto-generated code)

**Workflow:**
1. Edit SVG, commit changes
2. Regenerate code locally
3. Manually integrate code snippets into source
4. Commit updated source files

---

## Quick Reference

### Essential Inkscape Shortcuts

| Shortcut | Action |
|----------|--------|
| **F1** | Selector Tool (move, resize) |
| **F4** | Rectangle Tool |
| **F5** | Circle Tool |
| **Shift+Ctrl+O** | Object Properties (set ID) |
| **Shift+Ctrl+L** | Layers and Objects Panel |
| **Shift+Ctrl+A** | Align and Distribute |
| **Ctrl+G** | Group |
| **Shift+Ctrl+G** | Ungroup |
| **Ctrl+D** | Duplicate |
| **Alt+5** | Wireframe View (see overlaps) |

### Command Reference

```bash
# Initialize mapping for first time or add new components
python3 svg_to_juce.py --init

# Generate C++ code snippets
python3 svg_to_juce.py

# Preview output without writing files
python3 svg_to_juce.py --dry-run

# Check for issues (unmapped, overlaps, out-of-bounds)
python3 svg_to_juce.py --validate
```

### Component Type Cheat Sheet

| SVG Shape | Naming Pattern | Detected As | JUCE Type |
|-----------|----------------|-------------|-----------|
| Circle | `knob_*` | OccultKnob | Custom rotary knob |
| Rect | `*_btn` | TextButton | juce::TextButton |
| Rect | `mod_*` | ModButton | Modulation button (skipped) |
| Rect | `*_visualizer` | Custom Visualizer | WaveformDisplay, GrainVisualizer, etc. |
| Rect | `*_display` | Custom Display | WaveformDisplay |
| Rect | `*_box`, `*_dropdown` | ComboBox | juce::ComboBox |
| Rect | `*_indicator` | Indicator | Decorative (skipped) |
| Any | Other | Unknown | Skipped |

---

## Example: Complete Knob Addition

**Goal:** Add a "Filter Resonance" knob to the "Filter Section."

### 1. Open SVG in Inkscape

```bash
inkscape SR.svg
```

### 2. Draw the Knob

- Select **Circle Tool** (F5)
- Draw circle, set radius to 5mm
- Position at **cx: 150mm, cy: 120mm**

### 3. Name the Knob

- Open **Object Properties** (Shift+Ctrl+O)
- Set **ID**: `knob_resonance`
- Set **Label**: "Resonance Knob"

### 4. Add to Group (Optional)

- Find the "Filter Section" group in **Layers and Objects**
- Drag the new knob into the group

### 5. Save SVG

```bash
File → Save (Ctrl+S)
```

### 6. Update Mapping

```bash
cd Source/Tools/Parser
python3 svg_to_juce.py --init  # Preserves existing mappings, adds new ones
```

### 7. Review `component_mapping.json`

Look for the new entry:

```json
"knob_resonance": {
  "type": "OccultKnob",
  "member": "resonanceKnob",
  "label": "RESONANCE",
  "paramId": "",  // Add manually if not auto-detected
  "modulation": true
}
```

Edit to add `paramId`:

```json
"knob_resonance": {
  "type": "OccultKnob",
  "member": "resonanceKnob",
  "label": "RESONANCE",
  "paramId": "ParamIDs::resonance",  // ← Add this
  "modulation": true
}
```

### 8. Generate Code

```bash
python3 svg_to_juce.py
```

### 9. Integrate Code

Open `generated/header_snippet.hpp` and find:

```cpp
OccultKnob resonanceKnob{"RESONANCE"};
```

Copy this line into `PluginEditor.h` under the "Filter Section" member variables.

Open `generated/constructor_snippet.cpp` and find:

```cpp
setupKnob(resonanceKnob, ParamIDs::resonance, "RESONANCE");
addAndMakeVisible(resonanceKnob);
```

Copy into `PluginEditor` constructor.

Open `generated/resized_snippet.cpp` and find:

```cpp
resonanceKnob.setBounds(
    static_cast<int>(567 * scaleX) - knobWidth/2,  // SVG: 150.0mm
    static_cast<int>(454 * scaleY) - knobHeight/2, // SVG: 120.0mm
    knobWidth,
    knobHeight
);
```

Copy into `PluginEditor::resized()` method under the "Filter Section" comment.

### 10. Build and Test

```bash
cmake --build build
cmake --build build --target install
```

Open your DAW and verify the new knob appears at the correct position.

---

## Troubleshooting

### Issue: "No mapping configuration found"

**Cause:** `component_mapping.json` doesn't exist.

**Fix:**
```bash
python3 svg_to_juce.py --init
```

### Issue: Component not appearing in generated code

**Cause:** Component type is `Unknown`, `ModButton`, or `Indicator`.

**Fix:** Check `component_mapping.json` and change `type` if needed.

### Issue: Coordinates don't match SVG

**Cause:** Config dimensions in `component_mapping.json` don't match your SVG or JUCE window.

**Fix:** Edit `config` section:

```json
{
  "config": {
    "svg_width_mm": 225,
    "svg_height_mm": 190,
    "juce_width_px": 850,
    "juce_height_px": 720
  }
}
```

### Issue: New components not detected after editing SVG

**Cause:** Mapping file hasn't been regenerated.

**Fix:**
```bash
python3 svg_to_juce.py --init  # Updates mapping with new components
python3 svg_to_juce.py          # Regenerate code
```

---

## Conclusion

By following these conventions and best practices, you can create SVG layouts in Inkscape that seamlessly convert to JUCE C++ code. This workflow enables rapid UI iteration, pixel-perfect designs, and reduces manual coordinate entry errors.

For tool usage details and integration workflow, see the main [README.md](README.md).

For issues or questions, refer to the project's issue tracker or documentation.
