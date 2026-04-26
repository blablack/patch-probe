# CLAUDE.md

## What This Project Is

patch-probe is a headless C++ CLI tool (JUCE 8) that loads a VST3 plugin, enumerates all its presets (factory + user), and writes one YAML config file per preset. The YAML files are directly consumable by [patch-press](../patch-press) which uses pedalboard to render audio samples.

## Build Commands

```bash
# Release build (requires nine-strip to be built first — reuses its juceaide binary)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
ninja -C build

# Binary lands at:
build/src/patch-probe_artefacts/Release/patch-probe
```

The cmake setup intentionally reuses nine-strip's pre-built `juceaide` helper binary (see `cmake/UsePrebuiltJUCE.cmake`) to avoid requiring X11/FreeType development headers for the build tool itself. The tool itself is fully headless at runtime.

## Usage

```bash
patch-probe <plugin.vst3> <output-dir> [OPTIONS]

Options:
  --factory-only          Only scan factory presets (skip user preset dirs)
  --user-only             Only scan user preset files
  --profile synth|drums|pad   Profile written into YAML (default: synth)
  --format deluge|wav         Output format written into YAML (default: deluge)
  --output-path <path>        Destination path field in YAML output block
```

Each output YAML contains:
- `source.preset` — preset name
- `source.program_index` — factory index (for pedalboard program selection)
- `source.preset_file` — path to .vstpreset file (user presets)
- `source.raw_state` — base64-encoded `getStateInformation()` bytes (exact state)
- `source.parameter_state` — per-parameter normalized values (fallback)
- Default capture/analysis blocks that the user should tune

## Architecture

| File | Role |
|---|---|
| `src/Main.cpp` | CLI entry point, argument parsing |
| `src/PresetScanner.h/.cpp` | Loads the plugin, scans factory & user presets, captures state |
| `src/YamlWriter.h/.cpp` | Writes patch-press-compatible YAML for each preset |
| `cmake/UsePrebuiltJUCE.cmake` | Bootstraps JUCE using nine-strip's pre-built juceaide |

## Code Style

Matches nine-strip: Google style, 4-space indent, 128-column limit, Allman braces.

```bash
clang-format -i src/*.cpp src/*.h
```

## JUCE Notes

- Uses `juce_audio_processors_headless` (no X11/display dependency at runtime or build)
- Factory preset scanning: `getNumPrograms()` / `setCurrentProgram(i)` / `getStateInformation()`
- User preset loading: `ExtensionsVisitor::VST3Client::setPreset()` — parses `.vstpreset` files
- Format registration: `juce::addHeadlessDefaultFormatsToManager(formatManager)`
- MessageManager init: `juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread()`

## Relationship with patch-press

patch-press's `VSTSourceConfig` was extended to accept the new fields. `VSTAdapter._apply_preset()` now uses them in priority order:
1. `raw_state` (base64 bytes) — most reliable, exact state restoration
2. `program_index` — factory preset by index
3. `preset` name matching via `parameters["program"]`
4. `parameter_state` dict — individual parameter values (fallback)
