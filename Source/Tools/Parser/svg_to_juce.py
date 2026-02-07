#!/usr/bin/env python3
"""
SVG-to-JUCE Code Converter
Parses SR.svg and generates JUCE C++ code snippets for PluginEditor layout.
"""

import xml.etree.ElementTree as ET
import json
import os
import sys
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional


class SVGComponent:
    """Represents a UI component extracted from SVG"""
    def __init__(self, element_id: str, element_type: str, x: float, y: float,
                 width: float = 0, height: float = 0, radius: float = 0,
                 label: str = "", section: str = ""):
        self.id = element_id
        self.type = element_type  # 'circle', 'rect', 'text'
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.radius = radius
        self.label = label
        self.section = section

    def __repr__(self):
        return f"SVGComponent(id='{self.id}', type='{self.type}', x={self.x}, y={self.y})"


class SVGParser:
    """Parses SR.svg and extracts component information"""

    # SVG namespace
    SVG_NS = {
        'svg': 'http://www.w3.org/2000/svg',
        'inkscape': 'http://www.inkscape.org/namespaces/inkscape',
    }

    def __init__(self, svg_path: str):
        self.svg_path = svg_path
        self.tree = ET.parse(svg_path)
        self.root = self.tree.getroot()
        self.components: List[SVGComponent] = []

    def parse(self) -> List[SVGComponent]:
        """Parse SVG and extract all UI components"""
        self.components = []

        # Find all circles (knobs)
        for circle in self.root.iter('{http://www.w3.org/2000/svg}circle'):
            comp = self._parse_circle(circle)
            if comp:
                self.components.append(comp)

        # Find all rectangles (buttons, displays)
        for rect in self.root.iter('{http://www.w3.org/2000/svg}rect'):
            comp = self._parse_rect(rect)
            if comp:
                self.components.append(comp)

        # Find section information from groups
        self._assign_sections()

        return self.components

    def _parse_circle(self, element: ET.Element) -> Optional[SVGComponent]:
        """Parse a circle element (typically a knob)"""
        element_id = element.get('id', '')

        # Skip decorative elements
        if not element_id or element_id.startswith(('grain_particle', 'midi_dot', 'kbd_key')):
            return None

        cx = float(element.get('cx', 0))
        cy = float(element.get('cy', 0))
        r = float(element.get('r', 0))

        return SVGComponent(element_id, 'circle', cx, cy, radius=r)

    def _parse_rect(self, element: ET.Element) -> Optional[SVGComponent]:
        """Parse a rectangle element (buttons, displays, borders)"""
        element_id = element.get('id', '')

        # Skip background, borders, and decorative elements
        skip_ids = ['panel_background', 'header_bg', 'waveform_path']
        if not element_id or element_id.endswith('_border') or element_id in skip_ids:
            return None

        x = float(element.get('x', 0))
        y = float(element.get('y', 0))
        width = float(element.get('width', 0))
        height = float(element.get('height', 0))

        return SVGComponent(element_id, 'rect', x, y, width, height)

    def _assign_sections(self):
        """Assign section names to components based on SVG groups"""
        # Find all group elements with inkscape:label
        for group in self.root.iter('{http://www.w3.org/2000/svg}g'):
            section_label = group.get('{http://www.inkscape.org/namespaces/inkscape}label', '')
            group_id = group.get('id', '')

            if not section_label:
                continue

            # Get all elements within this group
            for comp in self.components:
                # Check if component is in this group by checking parent hierarchy
                for elem in group.iter():
                    if elem.get('id') == comp.id:
                        comp.section = section_label
                        break


class ComponentMapper:
    """Maps SVG components to JUCE component types and names"""

    def __init__(self, mapping_file: str):
        self.mapping_file = mapping_file
        self.mapping = {}
        self.config = {}

    def load(self):
        """Load mapping configuration from JSON file"""
        if os.path.exists(self.mapping_file):
            with open(self.mapping_file, 'r') as f:
                data = json.load(f)
                self.config = data.get('config', {})
                self.mapping = data.get('components', {})

    def save(self):
        """Save mapping configuration to JSON file"""
        data = {
            'config': self.config,
            'components': self.mapping
        }
        with open(self.mapping_file, 'w') as f:
            json.dump(data, f, indent=2)

    def get_component_info(self, svg_id: str) -> Optional[Dict]:
        """Get JUCE component info for a given SVG ID"""
        return self.mapping.get(svg_id)

    def detect_component_type(self, component: SVGComponent) -> str:
        """Detect JUCE component type from SVG component"""
        svg_id = component.id

        # Check if it's a knob
        if svg_id.startswith('knob_'):
            return 'OccultKnob'

        # Check if it's a button
        if svg_id.endswith('_btn'):
            return 'TextButton'

        # Check if it's a modulation button
        if svg_id.startswith('mod_'):
            return 'ModButton'

        # Check for specific component types
        if 'visualizer' in svg_id or 'display' in svg_id:
            if 'waveform' in svg_id:
                return 'WaveformDisplay'
            elif 'grain' in svg_id:
                return 'GrainVisualizer'
            elif 'lfo' in svg_id:
                return 'LFOVisualizer'

        # Check for combo box
        if 'waveform_box' in svg_id or 'dropdown' in svg_id:
            return 'ComboBox'

        # Check for indicators
        if 'indicator' in svg_id:
            return 'Indicator'

        return 'Unknown'


class CoordinateConverter:
    """Converts SVG millimeter coordinates to JUCE pixel coordinates"""

    def __init__(self, svg_width_mm: float = 225.0, svg_height_mm: float = 190.0,
                 juce_width_px: int = 850, juce_height_px: int = 720):
        self.svg_width_mm = svg_width_mm
        self.svg_height_mm = svg_height_mm
        self.juce_width_px = juce_width_px
        self.juce_height_px = juce_height_px

        self.scale_x = juce_width_px / svg_width_mm
        self.scale_y = juce_height_px / svg_height_mm

    def to_juce(self, x_mm: float, y_mm: float) -> Tuple[int, int]:
        """Convert SVG millimeters to JUCE pixels"""
        x_px = int(round(x_mm * self.scale_x))
        y_px = int(round(y_mm * self.scale_y))
        return x_px, y_px

    def size_to_juce(self, width_mm: float, height_mm: float) -> Tuple[int, int]:
        """Convert SVG size in millimeters to JUCE pixels"""
        width_px = int(round(width_mm * self.scale_x))
        height_px = int(round(height_mm * self.scale_y))
        return width_px, height_px


class CodeGenerator:
    """Generates JUCE C++ code snippets from SVG components"""

    def __init__(self, components: List[SVGComponent], mapper: ComponentMapper,
                 converter: CoordinateConverter):
        self.components = components
        self.mapper = mapper
        self.converter = converter

    def generate_header_snippet(self) -> str:
        """Generate header file member declarations"""
        lines = [
            "// AUTO-GENERATED from SR.svg - Review before integrating",
            "// Member variable declarations",
            ""
        ]

        # Group components by section
        sections = {}
        for comp in self.components:
            section = comp.section or "Other"
            if section not in sections:
                sections[section] = []
            sections[section].append(comp)

        # Generate declarations by section
        for section, comps in sorted(sections.items()):
            if section in ['Panel', 'Decorations', 'Header']:
                continue  # Skip non-control sections

            lines.append(f"// {section}")

            for comp in comps:
                info = self.mapper.get_component_info(comp.id)
                if not info:
                    lines.append(f"// UNMAPPED: {comp.id}")
                    continue

                comp_type = info.get('type', 'Unknown')
                member_name = info.get('member', comp.id)
                label = info.get('label', '')

                # Skip modulation buttons and indicators (handled separately)
                if comp_type in ['ModButton', 'Indicator', 'Unknown']:
                    continue

                if comp_type == 'OccultKnob':
                    lines.append(f'OccultKnob {member_name}{{"{label}"}};')
                elif comp_type == 'TextButton':
                    lines.append(f'juce::TextButton {member_name}{{"{label}"}};')
                elif comp_type == 'ComboBox':
                    lines.append(f'juce::ComboBox {member_name};')
                elif comp_type in ['WaveformDisplay', 'GrainVisualizer', 'LFOVisualizer']:
                    lines.append(f'{comp_type} {member_name};')

            lines.append("")

        return '\n'.join(lines)

    def generate_constructor_snippet(self) -> str:
        """Generate constructor initialization code"""
        lines = [
            "// AUTO-GENERATED from SR.svg - Review before integrating",
            "// Constructor initialization",
            ""
        ]

        # Setup knobs
        lines.append("// Setup knobs with parameter attachments")
        for comp in self.components:
            info = self.mapper.get_component_info(comp.id)
            if not info or info.get('type') != 'OccultKnob':
                continue

            member_name = info.get('member', '')
            param_id = info.get('paramId', '')
            label = info.get('label', '')

            if param_id:
                lines.append(f'setupKnob({member_name}, {param_id}, "{label}");')

        lines.append("")
        lines.append("// Add components to visible")

        # Add all components to visible
        for comp in self.components:
            info = self.mapper.get_component_info(comp.id)
            if not info:
                continue

            member_name = info.get('member', '')
            comp_type = info.get('type', '')

            # Skip modulation buttons and indicators
            if comp_type in ['ModButton', 'Indicator', 'Unknown']:
                continue

            if comp_type in ['OccultKnob', 'TextButton', 'ComboBox',
                           'WaveformDisplay', 'GrainVisualizer', 'LFOVisualizer']:
                lines.append(f'addAndMakeVisible({member_name});')

        return '\n'.join(lines)

    def generate_resized_snippet(self) -> str:
        """Generate resized() method layout code"""
        lines = [
            "// AUTO-GENERATED from SR.svg - Review before integrating",
            "// resized() method layout",
            "",
            "const float scaleX = getWidth() / 850.0f;",
            "const float scaleY = getHeight() / 720.0f;",
            "const int knobWidth = static_cast<int>(85 * scaleX);",
            "const int knobHeight = static_cast<int>(100 * scaleY);",
            ""
        ]

        # Group components by section
        sections = {}
        for comp in self.components:
            section = comp.section or "Other"
            if section not in sections:
                sections[section] = []
            sections[section].append(comp)

        # Generate setBounds calls by section
        for section, comps in sorted(sections.items()):
            if section in ['Panel', 'Decorations', 'Header']:
                continue  # Skip non-control sections

            lines.append(f"// {section}")

            for comp in comps:
                info = self.mapper.get_component_info(comp.id)
                if not info:
                    continue

                member_name = info.get('member', '')
                comp_type = info.get('type', '')

                # Skip modulation buttons and indicators
                if comp_type in ['ModButton', 'Indicator', 'Unknown']:
                    continue

                # Convert coordinates
                if comp.type == 'circle':
                    # For circles, center position needs adjustment
                    x_px, y_px = self.converter.to_juce(comp.x, comp.y)
                    # Adjust for knob size (center to top-left)
                    lines.append(f"{member_name}.setBounds(")
                    lines.append(f"    static_cast<int>({x_px} * scaleX) - knobWidth/2,  // SVG: {comp.x:.1f}mm")
                    lines.append(f"    static_cast<int>({y_px} * scaleY) - knobHeight/2, // SVG: {comp.y:.1f}mm")
                    lines.append(f"    knobWidth,")
                    lines.append(f"    knobHeight")
                    lines.append(");")
                else:
                    # For rectangles, use top-left corner
                    x_px, y_px = self.converter.to_juce(comp.x, comp.y)
                    w_px, h_px = self.converter.size_to_juce(comp.width, comp.height)
                    lines.append(f"{member_name}.setBounds(")
                    lines.append(f"    static_cast<int>({x_px} * scaleX),  // SVG: {comp.x:.1f}mm")
                    lines.append(f"    static_cast<int>({y_px} * scaleY),  // SVG: {comp.y:.1f}mm")
                    lines.append(f"    static_cast<int>({w_px} * scaleX),")
                    lines.append(f"    static_cast<int>({h_px} * scaleY)")
                    lines.append(");")

            lines.append("")

        return '\n'.join(lines)


class SVGToJUCEConverter:
    """Main converter class that orchestrates the conversion process"""

    def __init__(self, svg_path: str, mapping_path: str, output_dir: str):
        self.svg_path = svg_path
        self.mapping_path = mapping_path
        self.output_dir = output_dir

        self.parser = SVGParser(svg_path)
        self.mapper = ComponentMapper(mapping_path)
        self.converter = CoordinateConverter()

    def init_mapping(self):
        """Initialize mapping configuration from SVG"""
        print(f"Parsing {self.svg_path}...")
        components = self.parser.parse()

        print(f"Found {len(components)} components")

        # Create initial mapping skeleton
        mapping = {}
        for comp in components:
            # Skip if already mapped
            if comp.id in self.mapper.mapping:
                continue

            comp_type = self.mapper.detect_component_type(comp)

            # Generate member name suggestion
            member_name = self._suggest_member_name(comp.id, comp_type)

            # Generate parameter ID suggestion
            param_id = self._suggest_param_id(comp.id)

            # Generate label suggestion
            label = self._suggest_label(comp.id)

            mapping[comp.id] = {
                'type': comp_type,
                'member': member_name,
                'label': label
            }

            if param_id:
                mapping[comp.id]['paramId'] = param_id

            # Add modulation flag for knobs
            if comp_type == 'OccultKnob':
                mapping[comp.id]['modulation'] = True

        # Save configuration
        self.mapper.config = {
            'svg_width_mm': 225,
            'svg_height_mm': 190,
            'juce_width_px': 850,
            'juce_height_px': 720
        }
        self.mapper.mapping = mapping
        self.mapper.save()

        print(f"Generated mapping configuration: {self.mapping_path}")
        print("Please review and adjust mappings as needed.")

    def _suggest_member_name(self, svg_id: str, comp_type: str) -> str:
        """Suggest a member variable name based on SVG ID"""
        # Remove prefixes
        name = svg_id.replace('knob_', '').replace('mod_', '')

        # Convert to camelCase
        parts = name.split('_')
        if len(parts) > 1:
            name = parts[0] + ''.join(p.capitalize() for p in parts[1:])

        # Add suffix based on type
        if comp_type == 'OccultKnob':
            name += 'Knob'
        elif comp_type == 'TextButton':
            name += 'Button'
        elif comp_type == 'ComboBox':
            name += 'Box'
        elif 'Visualizer' in comp_type or 'Display' in comp_type:
            pass  # Already has suffix

        return name

    def _suggest_param_id(self, svg_id: str) -> str:
        """Suggest a ParamID based on SVG ID"""
        # Map common SVG IDs to parameter IDs
        param_map = {
            'knob_position': 'ParamIDs::position',
            'knob_size': 'ParamIDs::grainSize',
            'knob_density': 'ParamIDs::density',
            'knob_pitch': 'ParamIDs::pitch',
            'knob_spray': 'ParamIDs::spray',
            'knob_pan': 'ParamIDs::panSpread',
            'knob_gatk': 'ParamIDs::grainAttack',
            'knob_grel': 'ParamIDs::grainRelease',
            'knob_attack': 'ParamIDs::voiceAttack',
            'knob_decay': 'ParamIDs::voiceDecay',
            'knob_sustain': 'ParamIDs::voiceSustain',
            'knob_release': 'ParamIDs::voiceRelease',
            'knob_lfo_rate': 'ParamIDs::lfoRate',
            'knob_lfo_amount': 'ParamIDs::lfoAmount',
            'lfo_waveform_box': 'ParamIDs::lfoWaveform',
            'knob_delay': 'ParamIDs::delayTime',
            'knob_flutter': 'ParamIDs::flutter',
            'knob_hiss': 'ParamIDs::tapeHiss',
            'knob_damage': 'ParamIDs::damage',
            'knob_life': 'ParamIDs::life',
            'knob_reverb': 'ParamIDs::reverb',
            'knob_feedback': 'ParamIDs::feedback',
            'knob_mix': 'ParamIDs::mix',
            'knob_output': 'ParamIDs::output',
            'sample_gain_knob': 'ParamIDs::sampleGain',
        }

        return param_map.get(svg_id, '')

    def _suggest_label(self, svg_id: str) -> str:
        """Suggest a UI label based on SVG ID"""
        # Remove prefixes
        label = svg_id.replace('knob_', '').replace('mod_', '').replace('_btn', '')

        # Handle special cases
        if label == 'size':
            return 'SIZE'
        elif label == 'gatk':
            return 'G.ATK'
        elif label == 'grel':
            return 'G.REL'
        elif label == 'pan':
            return 'PAN'
        elif label == 'lfo_rate':
            return 'RATE'
        elif label == 'lfo_amount':
            return 'AMOUNT'
        elif label == 'delay':
            return 'DELAY'

        # Convert to uppercase with spaces
        return label.replace('_', ' ').upper()

    def validate(self):
        """Validate SVG against mapping configuration"""
        # Load mapping
        self.mapper.load()

        if not self.mapper.mapping:
            print("ERROR: No mapping configuration found. Run with --init first.")
            return

        # Parse SVG
        print(f"Parsing {self.svg_path}...")
        components = self.parser.parse()
        print(f"Found {len(components)} components\n")

        # Check for unmapped components
        unmapped = []
        for comp in components:
            if comp.id not in self.mapper.mapping:
                unmapped.append(comp.id)

        if unmapped:
            print(f"WARNING: {len(unmapped)} unmapped SVG components:")
            for comp_id in unmapped:
                print(f"  - {comp_id}")
            print()

        # Check for components with Unknown type
        unknown_type = []
        for comp_id, info in self.mapper.mapping.items():
            if info.get('type') == 'Unknown':
                unknown_type.append(comp_id)

        if unknown_type:
            print(f"WARNING: {len(unknown_type)} components with Unknown type:")
            for comp_id in unknown_type:
                print(f"  - {comp_id}")
            print()

        # Check for position overlaps (within 10px)
        overlaps = []
        for i, comp1 in enumerate(components):
            info1 = self.mapper.get_component_info(comp1.id)
            if not info1 or info1.get('type') in ['ModButton', 'Indicator', 'Unknown']:
                continue

            for comp2 in components[i+1:]:
                info2 = self.mapper.get_component_info(comp2.id)
                if not info2 or info2.get('type') in ['ModButton', 'Indicator', 'Unknown']:
                    continue

                # Calculate distance
                dx = abs(comp1.x - comp2.x)
                dy = abs(comp1.y - comp2.y)
                distance = (dx**2 + dy**2)**0.5

                if distance < 2:  # Less than 2mm apart
                    overlaps.append((comp1.id, comp2.id, distance))

        if overlaps:
            print(f"WARNING: {len(overlaps)} potential component overlaps:")
            for comp1_id, comp2_id, dist in overlaps:
                print(f"  - {comp1_id} & {comp2_id} ({dist:.1f}mm apart)")
            print()

        # Check for components outside SVG bounds
        out_of_bounds = []
        svg_width = 225  # mm
        svg_height = 190  # mm

        for comp in components:
            if comp.x < 0 or comp.x > svg_width or comp.y < 0 or comp.y > svg_height:
                out_of_bounds.append(comp.id)

        if out_of_bounds:
            print(f"WARNING: {len(out_of_bounds)} components outside SVG bounds:")
            for comp_id in out_of_bounds:
                print(f"  - {comp_id}")
            print()

        # Summary
        if not (unmapped or unknown_type or overlaps or out_of_bounds):
            print("✓ Validation passed! No issues found.")
        else:
            print(f"Validation complete with {len(unmapped) + len(unknown_type) + len(overlaps) + len(out_of_bounds)} warnings.")

    def generate_code(self, dry_run=False):
        """Generate JUCE C++ code snippets"""
        # Load mapping
        self.mapper.load()

        if not self.mapper.mapping:
            print("ERROR: No mapping configuration found. Run with --init first.")
            return

        # Parse SVG
        print(f"Parsing {self.svg_path}...")
        components = self.parser.parse()
        print(f"Found {len(components)} components")

        # Create generator
        generator = CodeGenerator(components, self.mapper, self.converter)

        # Generate snippets
        print("Generating code snippets...")

        header_snippet = generator.generate_header_snippet()
        constructor_snippet = generator.generate_constructor_snippet()
        resized_snippet = generator.generate_resized_snippet()

        if dry_run:
            print("\n" + "="*60)
            print("DRY RUN - Generated code preview:")
            print("="*60)
            print("\n--- header_snippet.hpp ---")
            print(header_snippet[:500])
            print("...\n")
            print("--- constructor_snippet.cpp ---")
            print(constructor_snippet[:500])
            print("...\n")
            print("--- resized_snippet.cpp ---")
            print(resized_snippet[:500])
            print("...\n")
            print("="*60)
            print("Use without --dry-run to write files.")
            return

        # Create output directory
        os.makedirs(self.output_dir, exist_ok=True)

        # Write to files
        header_file = os.path.join(self.output_dir, 'header_snippet.hpp')
        constructor_file = os.path.join(self.output_dir, 'constructor_snippet.cpp')
        resized_file = os.path.join(self.output_dir, 'resized_snippet.cpp')

        with open(header_file, 'w') as f:
            f.write(header_snippet)
        print(f"Generated: {header_file}")

        with open(constructor_file, 'w') as f:
            f.write(constructor_snippet)
        print(f"Generated: {constructor_file}")

        with open(resized_file, 'w') as f:
            f.write(resized_snippet)
        print(f"Generated: {resized_file}")

        print("\nCode generation complete!")
        print("Review the generated snippets before integrating into PluginEditor.")


def main():
    parser = argparse.ArgumentParser(
        description='Convert SR.svg to JUCE C++ code snippets'
    )
    parser.add_argument('--init', action='store_true',
                       help='Initialize mapping configuration from SVG')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show output without writing files')
    parser.add_argument('--validate', action='store_true',
                       help='Validate SVG against existing code')

    args = parser.parse_args()

    # Paths
    script_dir = Path(__file__).parent
    svg_path = script_dir / 'SR.svg'
    mapping_path = script_dir / 'component_mapping.json'
    output_dir = script_dir / 'generated'

    # Check if SVG exists
    if not svg_path.exists():
        print(f"ERROR: SVG file not found: {svg_path}")
        sys.exit(1)

    # Create converter
    converter = SVGToJUCEConverter(str(svg_path), str(mapping_path), str(output_dir))

    if args.init:
        converter.init_mapping()
    elif args.validate:
        converter.validate()
    elif args.dry_run:
        converter.generate_code(dry_run=True)
    else:
        converter.generate_code(dry_run=False)


if __name__ == '__main__':
    main()
