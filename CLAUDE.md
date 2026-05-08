# CLAUDE.md

## What This Project Is

patch-probe is a GUI desktop app (JUCE 8) that loads a VST3 plugin in an interactive window. The user browses presets using the plugin's own UI, then clicks **Save** to write a YAML snapshot of the current state. It is part of a small toolchain:

- **pedalboard** — can restore a plugin's state from a saved blob but cannot change presets at runtime; patch-probe captures those blobs upfront.
- **patch-render** — headless CLI renderer that accepts a `raw_state` blob directly in its JSON config to set a specific preset before rendering.

Why interactive rather than headless batch scanning: VST3 has no universal preset enumeration API that works across all plugins. `setCurrentProgram()` only covers factory programs; `.vstpreset` loading requires the plugin to use standard VST3 IComponent state format, which many plugins (e.g. Odin2 with `.odin` blobs) do not.

## Build Commands

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
ninja -C build

# Binary lands at:
build/src/patch-probe_artefacts/Release/patch-probe
```

## Usage

Launch the app, click **Load Plugin…** to open a `.vst3` file, interact with the plugin UI to select a preset, then click **Save**. Repeat for each preset. Output files land in `~/patch-probe/` by default (configurable via the Output button).

The app also accepts CLI arguments to skip the file-chooser dialogs:

```bash
build/src/patch-probe_artefacts/Release/patch-probe <plugin.vst3> <output-dir>
```

**Auto-save**: enabled by default (toggle in the bottom bar). When on, the app polls the plugin state every 100 ms; once the state has been stable for 300 ms it saves automatically. The preset name field is pre-filled by `tryGetPresetName()` and can be edited before the save fires.

Each output YAML contains a single `source` block:
- `type` — always `vst`
- `plugin` — path to the `.vst3` file
- `preset` — preset name (auto-detected or user-entered)
- `raw_state` — base64-encoded `getStateInformation()` bytes — consumed by pedalboard to restore state and by patch-render's `raw_state` field to set a preset before rendering
- `parameter_state` — per-parameter normalized values — fallback if raw_state isn't accepted

## Architecture

| File | Role |
|---|---|
| `src/Main.cpp` | JUCE app entry point; `MainWindow` subclasses `DocumentWindow` |
| `src/MainComponent.h/.cpp` | Main UI: toolbar (Load), plugin editor area, bottom save bar |
| `src/PresetScanner.h/.cpp` | Loads the plugin, exposes editor, captures state on demand |
| `src/YamlWriter.h/.cpp` | Writes one YAML file per captured preset |
| `lib/JUCE` | JUCE submodule (builds juceaide automatically during cmake configure) |

## Code Style

Matches nine-strip: Google style, 4-space indent, 128-column limit, Allman braces.

```bash
clang-format -i src/*.cpp src/*.h
```

## Design Constraints

**Plugin-agnostic**: state capture uses only `getStateInformation()` + `getParameters()`. Never hardcode plugin names, vendor names, or preset formats.

## JUCE Notes

- Uses `juce_audio_processors` + `juce_gui_basics` (GUI app, requires display)
- Format registration: `juce::addDefaultFormatsToManager(formatManager)`
- Plugin editor hosting: `plugin->createEditorIfNeeded()`, `addAndMakeVisible(*editor_)`
- File chooser: async `launchAsync()` API; keep `std::unique_ptr<juce::FileChooser>` as member
- `DocumentWindow`: must subclass and override `closeButtonPressed()` — no callback member in JUCE 8
- Preset name detection fallback chain:
  1. `getProgramName(getCurrentProgram())` — skip generics ("Default", "Init", "Untitled", "Program N")
  2. Parse JUCE XML state blob: decode `IComponent` base64 → inner XML → check `patch_name`/`presetName`/`name`/`preset_name`/`currentPreset` attributes
  3. Auto-counter fallback: `Preset_NNN`
- `PresetScanner::captureState()` calls `clearNoteState()` first: sends All Notes Off on all 16 MIDI channels via `processBlock()` so an active note is not baked into the serialized state
- `PresetScanner` exposes `getManufacturerName()` in addition to `getPluginName()`
