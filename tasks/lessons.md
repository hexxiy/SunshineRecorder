# Lessons Learned - SunshineRecorder

## 2026-02-05: SVG-to-JUCE Code Converter

### What Worked Well

#### 1. Plan Mode First
- **Pattern**: Entered plan mode before implementation for non-trivial task
- **Result**: Clear architecture and file structure defined upfront
- **Benefit**: No backtracking or major refactors needed during implementation
- **Lesson**: For tools/utilities with 3+ components, planning saves time overall

#### 2. Iterative Testing Throughout
- **Pattern**: Test after each major component (parser → mapper → generator)
- **Result**: Caught issues early (e.g., member name mismatches)
- **Benefit**: Small, fixable issues vs. large debugging sessions later
- **Lesson**: Run `--init` and `--validate` after each code generation enhancement

#### 3. Non-Destructive Code Generation
- **Pattern**: Generate snippets for manual integration, not direct file replacement
- **Result**: Preserves existing logic (MIDI learn, callbacks, etc.)
- **Benefit**: Developers can cherry-pick what to update
- **Lesson**: For code generators in active projects, snippets > full file generation

#### 4. Configuration-Driven Approach
- **Pattern**: JSON mapping file separates SVG IDs from JUCE implementation
- **Result**: Designers and developers can work independently
- **Benefit**: UI changes don't require code understanding
- **Lesson**: External config files are essential for designer/developer collaboration

#### 5. Validation Before Generation
- **Pattern**: `--validate` flag to check mapping completeness
- **Result**: Catches unmapped components, overlaps, bounds issues
- **Benefit**: Prevents generating broken code
- **Lesson**: Always provide a "check first" mode for code generators

### What Could Be Improved

#### 1. Coordinate Accuracy Verification
- **Issue**: Didn't compare generated coordinates against existing PluginEditor.cpp
- **Impact**: Can't measure coordinate drift quantitatively
- **Fix**: Add `--compare` mode to diff generated vs. existing coordinates
- **Lesson**: For replacement tools, always validate against ground truth

#### 2. Member Name Suggestions
- **Issue**: Initial auto-generated names needed manual correction (e.g., `sizeKnob` → `grainSizeKnob`)
- **Impact**: Required 14 manual edits to component_mapping.json
- **Fix**: Smarter heuristics or lookup table for common patterns
- **Lesson**: Auto-suggestions should match existing naming conventions

#### 3. Modulation Button Handling
- **Issue**: Initially included mod buttons in header snippet, causing duplicates
- **Impact**: Had to add skip logic for ModButton type
- **Fix**: Better upfront understanding of which SVG elements map to code
- **Lesson**: Study existing code structure before designing mapping schema

#### 4. Section Assignment
- **Issue**: Section labels from SVG groups not consistently populated
- **Impact**: Components grouped correctly but could be better
- **Fix**: Improve `_assign_sections()` to handle nested groups
- **Lesson**: SVG group hierarchy parsing needs recursive traversal

### Patterns to Reuse

#### 1. Three-Stage Code Generation
```
Parse Input → Map to Target → Generate Output
(SVG)      → (JSON config) → (C++ snippets)
```
- **Reusable for**: Any visual design → code pipeline
- **Key**: Middle mapping layer allows independent updates

#### 2. CLI Flag Pattern
```
--init      : Generate initial configuration
--validate  : Check for issues
--dry-run   : Preview without writing
<no flags>  : Generate output
```
- **Reusable for**: All code generation tools
- **Key**: Safety-first workflow, users never lose data

#### 3. Component Type Detection
```python
def detect_type(element_id, element_type):
    if element_id.startswith('prefix_'):
        return 'ComponentType'
    if element_id.endswith('_suffix'):
        return 'OtherType'
    # fallback
    return 'Unknown'
```
- **Reusable for**: Any ID-based component detection
- **Key**: Unknown type allows graceful handling of unrecognized elements

#### 4. Coordinate Conversion with Comments
```cpp
positionKnob.setBounds(
    static_cast<int>(64 * scaleX),  // SVG: 17.0mm
    ...
);
```
- **Reusable for**: All coordinate transformations
- **Key**: Comments preserve traceability to source

### Architecture Decisions

#### ✅ Good: Class-Based Organization
```
SVGParser      → Extract data
ComponentMapper → Configure mapping
CoordinateConverter → Transform coordinates
CodeGenerator  → Produce output
SVGToJUCEConverter → Orchestrate
```
- **Why**: Single responsibility, easy to test/extend
- **Result**: Can swap parsers or generators independently

#### ✅ Good: Separate Output Files
```
header_snippet.hpp
constructor_snippet.cpp
resized_snippet.cpp
```
- **Why**: Developers integrate each section independently
- **Result**: Can update layout without touching initialization

#### ⚠️ Could Improve: Magic Numbers
```python
if distance < 2:  # Less than 2mm apart - magic number!
```
- **Why**: Hardcoded thresholds should be configurable
- **Better**: Add to config.json as validation settings

### Testing Strategy

#### What Was Tested
✅ `--init` generates valid JSON
✅ `--validate` detects unmapped components
✅ `--dry-run` shows preview without writing
✅ Generated code has correct syntax
✅ Coordinate conversion math verified (17mm → 64px)
✅ All CLI flags functional

#### What Should Be Tested (Future)
- [ ] Generated code compiles without errors
- [ ] Visual comparison: screenshot before/after layout changes
- [ ] All 24 knobs respond to parameter changes
- [ ] Performance: parse time for large SVGs
- [ ] Edge cases: SVG with transforms, nested groups

### Key Metrics

**Development Efficiency**
- Time to implement: ~1 session (including plan, code, test, docs)
- Lines of code: ~800 Python + ~150 docs
- Files created: 8 total
- Tasks completed: 6/6 (100%)

**Code Quality**
- No external dependencies (stdlib only)
- Modular architecture (5 classes)
- Comprehensive error handling
- CLI with multiple modes
- Full documentation

**Output Quality**
- 334 lines of C++ generated
- 52 SVG components parsed
- 48 components mapped
- 0 syntax errors
- 4 intentional "Unknown" types

### Workflow Adherence

✅ **Plan Mode**: Used for initial architecture
✅ **Task Tracking**: Created and completed 6 tasks
✅ **Validation**: Tested each CLI mode
✅ **Documentation**: Comprehensive README created
✅ **Non-Destructive**: Manual integration workflow
✅ **Lessons Captured**: This file!

### Recommendations for Future Sessions

1. **For similar tools**: Always start with `--init` → `--validate` → generate workflow
2. **For code generators**: Provide snippets, not full file replacement
3. **For UI tools**: Coordinate conversion comments are essential for debugging
4. **For configuration**: JSON is great, YAML might be even more readable
5. **For validation**: Check against existing code, not just internal consistency

### Anti-Patterns Avoided

❌ **Direct file overwrite**: Chose snippet generation instead
❌ **Hardcoded paths**: Used Path() and script-relative paths
❌ **Silent failures**: All errors print clear messages
❌ **No validation**: Added --validate before generation
❌ **Magic numbers**: Mostly avoided, with a few exceptions to improve

---

## Pattern Library

### Code Generation Best Practices
1. Parse input → intermediate representation → generate output
2. Separate config from code (JSON mapping)
3. Non-destructive output (snippets > full files)
4. Validation mode before generation
5. Dry-run mode for preview
6. Include source coordinates as comments
7. Single responsibility classes
8. CLI with progressive safety (init → validate → dry-run → generate)

### Python Tool Development
1. No external deps if stdlib works
2. Argparse for CLI
3. Pathlib over os.path
4. Type hints for clarity
5. Docstrings for public methods
6. ET for simple XML parsing
7. JSON for human-editable config

### JUCE Code Generation
1. Use `static_cast<int>()` for pixel calculations
2. Include scale factors (`scaleX`, `scaleY`)
3. Adjust circle center → top-left for setBounds()
4. Group by UI sections (matches designer's mental model)
5. setupKnob() pattern for parameter attachment
6. addAndMakeVisible() for all components

---

*This file should be updated after each significant session or when patterns emerge.*
