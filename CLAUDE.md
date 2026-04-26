# CLAUDE.md

## What This Project Is

patch-probe is a headless C++ CLI tool (JUCE 8) that loads a VST3 plugin, enumerates all its presets (factory + user), and writes one YAML file per preset containing the full plugin state. The problem it solves: pedalboard cannot change presets at runtime, but it can restore a plugin's state from a saved blob. patch-probe extracts that state for every preset up front so pedalboard can load each one directly.

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
  --factory-only    Only scan factory presets (skip user preset dirs)
  --user-only       Only scan user preset files
```

Each output YAML contains a single `source` block:
- `type` — always `vst`
- `plugin` — path to the `.vst3` file
- `preset` — preset name
- `preset_source` — `factory` or `user`
- `program_index` — factory index (present for factory presets)
- `preset_file` — path to `.vstpreset` file (present for user presets)
- `raw_state` — base64-encoded `getStateInformation()` bytes — the primary restore mechanism
- `parameter_state` — per-parameter normalized values — fallback if raw_state isn't accepted

## Architecture

| File | Role |
|---|---|
| `src/Main.cpp` | CLI entry point, argument parsing |
| `src/PresetScanner.h/.cpp` | Loads the plugin, scans factory & user presets, captures state |
| `src/YamlWriter.h/.cpp` | Writes one YAML file per preset containing the plugin state |
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

