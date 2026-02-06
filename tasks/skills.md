# Skills Reference - Tools and Usage Patterns

This document catalogs tools, techniques, and usage patterns for future Claude Code instances working on SunshineRecorder.

---

## Tool Usage Patterns

### 1. Read Tool - File Reading
**When to use**: Always read before editing existing files

```python
# ✅ Good: Read first to understand context
Read("Source/PluginEditor.h", offset=54, limit=50)
# Then edit based on what you found

# ❌ Bad: Edit without reading
Edit("Source/PluginEditor.h", ...)  # Might break existing code
```

**Pro tips**:
- Use `offset` and `limit` for large files (only read relevant sections)
- Read multiple files in parallel with single message, multiple tool calls
- Always read before Edit or Write to understand existing patterns

### 2. Edit Tool - Surgical File Modifications
**When to use**: Modifying existing files with known content

```python
# ✅ Good: Exact string replacement
Edit(
    file_path="config.json",
    old_string='"member": "sizeKnob"',
    new_string='"member": "grainSizeKnob"'
)

# ✅ Good: Multiple edits to same file in sequence
# Edit 1, then Edit 2, then Edit 3 - all applied sequentially

# ❌ Bad: Editing without reading first
# Always read the file to get exact string matches
```

**Pro tips**:
- `old_string` must match exactly (including whitespace)
- Use `replace_all=true` for renaming throughout file
- For multiple changes in one file, make separate Edit calls
- Preserve exact indentation from Read output

### 3. Write Tool - Create New Files
**When to use**: Creating new files from scratch

```python
# ✅ Good: New file creation
Write(
    file_path="/absolute/path/to/new_file.py",
    content="#!/usr/bin/env python3\n..."
)

# ⚠️ Warning: Overwrites existing files completely
# Always Read existing files first if unsure

# ❌ Bad: Using Write for small edits to existing files
# Use Edit instead for surgical changes
```

**Pro tips**:
- Always use absolute paths
- For scripts, include shebang line (`#!/usr/bin/env python3`)
- For Python, follow PEP 8 style
- For C++, match existing project style (see CLAUDE.md)

### 4. Bash Tool - Command Execution
**When to use**: Git, build, test, file operations that need shell

```bash
# ✅ Good: Running build commands
cmake -B build -S .
cmake --build build

# ✅ Good: Git operations
git status
git diff
git log --oneline -10

# ✅ Good: Testing tools
python3 script.py --validate
python3 -m pytest tests/

# ✅ Good: File operations when Read/Write aren't suitable
ls -lh Source/Parser/
wc -l generated/*.cpp
chmod +x script.py

# ❌ Bad: Using for file reading (use Read tool)
cat file.txt  # NO - use Read instead

# ❌ Bad: Using for file editing (use Edit tool)
sed -i 's/old/new/g' file.txt  # NO - use Edit instead
```

**Pro tips**:
- Quote paths with spaces: `cd "path with spaces"`
- Chain with `&&` for sequential operations
- Use `2>&1 | head -n 50` to limit output
- Always provide clear description parameter
- Check command success before proceeding

### 5. Glob Tool - File Pattern Matching
**When to use**: Finding files by name/pattern

```python
# ✅ Good: Finding specific file types
Glob(pattern="**/*.py")
Glob(pattern="Source/**/*.h")
Glob(pattern="Source/DSP/*.cpp")

# ✅ Good: Finding test files
Glob(pattern="**/*test*.cpp")

# ❌ Bad: Using Bash ls/find instead
# Glob is faster and more reliable
```

**Pro tips**:
- `**` for recursive search
- `*` for wildcard within directory
- Returns sorted by modification time
- Fast even on large codebases

### 6. Grep Tool - Content Search
**When to use**: Finding code by content

```python
# ✅ Good: Finding class definitions
Grep(pattern="class.*Knob", type="cpp")

# ✅ Good: Finding parameter usage
Grep(pattern="ParamIDs::position", output_mode="files_with_matches")

# ✅ Good: Finding function calls with context
Grep(pattern="setupKnob", output_mode="content", -B=2, -A=2)

# ✅ Good: Case-insensitive search
Grep(pattern="midi learn", -i=True)

# ❌ Bad: Using Bash grep/rg instead
# Grep tool has correct permissions and access
```

**Pro tips**:
- Use `output_mode="files_with_matches"` to find files (default)
- Use `output_mode="content"` to see matching lines
- Use `-A`, `-B`, `-C` for context (requires content mode)
- Use `type` parameter for language filtering
- Use `glob` for file filtering

### 7. Task Tools - Progress Tracking
**When to use**: Complex multi-step tasks

```python
# ✅ Good: Start of session
TaskCreate(
    subject="Implement SVG parser",
    description="Parse SR.svg and extract component positions...",
    activeForm="Implementing SVG parser"
)

# ✅ Good: Starting work
TaskUpdate(taskId="1", status="in_progress")

# ✅ Good: Completing work
TaskUpdate(taskId="1", status="completed")

# ✅ Good: Checking status
TaskList()

# ❌ Bad: Creating tasks for single-step operations
# Only use for 3+ step tasks
```

**Pro tips**:
- Create tasks at session start for complex work
- Update to `in_progress` when starting
- Update to `completed` when done
- Use `TaskList()` to find next task
- Include clear descriptions for teammates

### 8. EnterPlanMode - Planning Complex Changes
**When to use**: 3+ file changes or architectural decisions

```python
# ✅ Good: Multi-file feature
"Add authentication system" → EnterPlanMode()

# ✅ Good: Architectural decision
"Refactor DSP pipeline" → EnterPlanMode()

# ✅ Good: Multiple approaches possible
"Add caching layer" → EnterPlanMode()

# ❌ Bad: Simple one-file fixes
"Fix typo in README" → Just fix it

# ❌ Bad: User gave detailed requirements
# If user provided step-by-step plan, follow it
```

**Pro tips**:
- Use for ANY non-trivial task (err on side of planning)
- Write detailed implementation steps
- Identify all files that need changes
- Consider edge cases and dependencies
- Exit with ExitPlanMode when ready

---

## Code Generation Patterns

### Pattern 1: Parse → Map → Generate
**Use case**: Converting one format to another (SVG → JUCE, API spec → code, etc.)

```
1. Parse input (Read source, extract data)
2. Map to intermediate representation (JSON config)
3. Generate output (Write target code)
```

**Example**: SVG-to-JUCE converter
- Parse: `SVGParser` extracts components from XML
- Map: `component_mapping.json` defines SVG ID → JUCE component
- Generate: `CodeGenerator` produces C++ snippets

**When to use**:
- Design files → code
- API specs → client code
- Config files → runtime code
- Documentation → test cases

### Pattern 2: CLI Tool Structure
**Use case**: Python scripts with multiple modes

```python
parser = argparse.ArgumentParser()
parser.add_argument('--init', help='Initialize')
parser.add_argument('--validate', help='Check for errors')
parser.add_argument('--dry-run', help='Preview output')
args = parser.parse_args()

if args.init:
    # Generate initial config
elif args.validate:
    # Check for issues
elif args.dry_run:
    # Preview without writing
else:
    # Normal operation
```

**Pro tips**:
- Always provide `--help`
- `--dry-run` prevents accidental overwrites
- `--validate` catches issues before generation
- `--init` for first-time setup

### Pattern 3: Configuration-Driven Systems
**Use case**: Decoupling data from code

```json
{
  "config": {
    "svg_width_mm": 225,
    "juce_width_px": 850
  },
  "components": {
    "knob_position": {
      "type": "OccultKnob",
      "member": "positionKnob",
      "paramId": "ParamIDs::position"
    }
  }
}
```

**When to use**:
- Designer/developer collaboration
- Frequent configuration changes
- Runtime behavior modification
- A/B testing different setups

**Pro tips**:
- Use JSON for human-editable config
- Validate config before using
- Provide schema/example in docs
- Version config format

### Pattern 4: Non-Destructive Code Generation
**Use case**: Generating code in active projects

```
Input: design.svg
↓
Process: parser + generator
↓
Output: code_snippet.cpp (for manual integration)
NOT: PluginEditor.cpp (direct overwrite)
```

**Why**:
- Preserves existing business logic
- Allows developer review
- Prevents accidental data loss
- Enables partial adoption

**When to use**:
- Active codebases with existing logic
- Generated code mixed with handwritten code
- Iterative design updates
- Collaborative development

---

## JUCE-Specific Patterns

### Pattern 1: Component Declaration
```cpp
// In header
OccultKnob positionKnob{"POSITION"};
std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> positionAttachment;

// In constructor
positionAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
    audioProcessor.getApvts(), ParamIDs::position, positionKnob
);
addAndMakeVisible(positionKnob);

// In resized()
positionKnob.setBounds(x, y, width, height);
```

### Pattern 2: Parameter Access (Thread-Safe)
```cpp
// Audio thread (lock-free)
float value = parameters.position->load();

// UI thread (read-only)
float displayValue = processor.getParameters().position->load();

// UI thread (complex data, needs lock)
juce::ScopedLock sl(processor.getLock());
auto data = processor.getData();
```

### Pattern 3: JUCE File Organization
```
Source/
├── PluginProcessor.h/cpp    # Audio processing
├── PluginEditor.h/cpp        # UI rendering
├── DSP/                      # Audio algorithms
│   ├── Grain.h/cpp
│   ├── GrainEngine.h/cpp
│   └── Voice.h/cpp
├── Parameters/               # Parameter definitions
│   └── Parameters.h/cpp
└── UI/                       # Custom UI components
    ├── OccultKnob.h/cpp
    └── WaveformDisplay.h/cpp
```

---

## Python Development Patterns

### Pattern 1: XML Parsing with ElementTree
```python
import xml.etree.ElementTree as ET

tree = ET.parse('file.svg')
root = tree.getroot()

# Iterate with namespace
SVG_NS = {'svg': 'http://www.w3.org/2000/svg'}
for element in root.iter('{http://www.w3.org/2000/svg}circle'):
    cx = float(element.get('cx', 0))
    cy = float(element.get('cy', 0))
    element_id = element.get('id', '')
```

### Pattern 2: Coordinate Transformation
```python
class CoordinateConverter:
    def __init__(self, source_width, source_height,
                 target_width, target_height):
        self.scale_x = target_width / source_width
        self.scale_y = target_height / source_height

    def convert(self, x, y):
        return int(x * self.scale_x), int(y * self.scale_y)
```

### Pattern 3: Type Detection from IDs
```python
def detect_type(element_id: str) -> str:
    if element_id.startswith('knob_'):
        return 'OccultKnob'
    if element_id.endswith('_btn'):
        return 'TextButton'
    return 'Unknown'
```

---

## Git Workflow

### Committing Changes
```bash
# 1. Check status
git status

# 2. View changes
git diff

# 3. Stage specific files (NOT git add .)
git add Source/Parser/svg_to_juce.py
git add Source/Parser/component_mapping.json

# 4. Commit with co-author
git commit -m "$(cat <<'EOF'
Add SVG-to-JUCE code converter

Implements Python tool to parse SR.svg and generate JUCE C++ layout code.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
EOF
)"
```

**Never**:
- Don't use `git add -A` or `git add .` (might include secrets)
- Don't use `--no-verify` (skips hooks)
- Don't use `--amend` after hook failure (creates new commit instead)
- Don't commit .env, credentials, or large binaries

### Creating Pull Requests
```bash
# 1. Check branch and commits
git log origin/main..HEAD --oneline
git diff origin/main...HEAD

# 2. Push to remote
git push -u origin feature-branch

# 3. Create PR with gh CLI
gh pr create --title "Add SVG-to-JUCE converter" --body "$(cat <<'EOF'
## Summary
- Implements Python tool to parse SR.svg
- Generates JUCE C++ layout code snippets
- Includes validation and dry-run modes

## Test plan
- [ ] Run svg_to_juce.py --init
- [ ] Run svg_to_juce.py --validate
- [ ] Verify generated code compiles

🤖 Generated with [Claude Code](https://claude.com/claude-code)
EOF
)"
```

---

## Build System (CMake)

### Building SunshineRecorder
```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Install (copies to ~/.vst3/)
cmake --build build --target install

# Clean
rm -rf build
```

### Testing Changes
```bash
# Build only (no install)
cmake --build build

# Check plugin binary
ls -lh build/SunshineRecorder_artefacts/VST3/SunshineRecorder.vst3/

# Install and test in DAW
cmake --build build --target install
# Then open DAW and load plugin
```

---

## Validation and Testing Patterns

### Pre-Generation Validation
```python
def validate():
    """Check before generating code"""
    errors = []

    # Check 1: All mapped components exist
    for svg_id in mapping.keys():
        if svg_id not in parsed_components:
            errors.append(f"Mapped but not in SVG: {svg_id}")

    # Check 2: All SVG components mapped
    for comp in parsed_components:
        if comp.id not in mapping:
            errors.append(f"In SVG but not mapped: {comp.id}")

    # Check 3: No overlaps
    for comp1, comp2 in combinations(parsed_components, 2):
        if distance(comp1, comp2) < threshold:
            errors.append(f"Overlap: {comp1.id} & {comp2.id}")

    return errors
```

### Post-Generation Verification
```bash
# Syntax check (if clang available)
clang++ -fsyntax-only -std=c++17 generated/snippet.cpp

# Line count sanity check
wc -l generated/*.cpp

# Coordinate spot-check
grep "positionKnob.setBounds" generated/resized_snippet.cpp
```

---

## Common Pitfalls and Solutions

### Pitfall 1: Editing Without Reading
```python
# ❌ Bad
Edit("config.json", old_string='...', ...)  # Might not match exactly

# ✅ Good
Read("config.json")  # First, see the exact content
Edit("config.json", old_string='exact match from output', ...)
```

### Pitfall 2: Using Bash for File Operations
```bash
# ❌ Bad
cat file.txt  # Use Read tool instead
grep "pattern" *.cpp  # Use Grep tool instead
find . -name "*.py"  # Use Glob tool instead

# ✅ Good
Read("file.txt")
Grep(pattern="pattern", type="cpp")
Glob(pattern="**/*.py")
```

### Pitfall 3: Direct File Overwrite in Active Projects
```python
# ❌ Bad: Overwriting active file
Write("Source/PluginEditor.cpp", full_content)  # Loses existing logic

# ✅ Good: Generate snippet for manual integration
Write("Source/Parser/generated/resized_snippet.cpp", snippet)
# Developer reviews and integrates manually
```

### Pitfall 4: Not Using Plan Mode
```python
# ❌ Bad: Diving into complex task without planning
"Refactor the DSP architecture"
# Start editing files immediately

# ✅ Good: Plan first for 3+ file changes
"Refactor the DSP architecture"
EnterPlanMode()  # Design approach, then implement
```

---

## Session Management

### Start of Session
1. Read CLAUDE.md for project context
2. Check git status and recent commits
3. If complex task: EnterPlanMode()
4. If simple task: Just do it
5. Create tasks for multi-step work (TaskCreate)

### During Session
1. Update task status when starting work (TaskUpdate)
2. Test after each major component
3. Validate before finalizing (--validate, --dry-run)
4. Document as you go (comments, docstrings)

### End of Session
1. Complete all tasks (TaskUpdate status="completed")
2. Update changelog.md with changes
3. Update lessons.md with patterns learned
4. Commit changes with proper message
5. Verify build succeeds

---

## Quick Reference Card

| Task | Tool | Example |
|------|------|---------|
| Read file | Read | `Read("file.cpp")` |
| Edit file | Edit | `Edit("file.cpp", old="x", new="y")` |
| Create file | Write | `Write("file.py", content="...")` |
| Find files | Glob | `Glob(pattern="**/*.h")` |
| Search code | Grep | `Grep(pattern="class.*", type="cpp")` |
| Run command | Bash | `Bash(command="cmake --build build")` |
| Track task | TaskCreate | `TaskCreate(subject="...", description="...")` |
| Plan complex | EnterPlanMode | `EnterPlanMode()` |

---

*This skills reference should be consulted at session start and updated with new patterns as they emerge.*
