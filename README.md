# patch-probe

GUI tool for capturing VST3 plugin preset state as portable YAML snapshots with base64-encoded `raw_state` blobs.

**Why it exists:** VST3 has no universal preset enumeration API that works across all plugins. `setCurrentProgram()` only covers factory programs; `.vstpreset` loading requires the standard IComponent state format, which many plugins (e.g. Odin2 with `.odin` blobs) do not implement. patch-probe sidesteps this by letting the user browse presets interactively through the plugin's own UI and capturing the state on demand.

## Usage

Launch the app, click **Load Plugin…** to open a `.vst3` file, interact with the plugin UI to select a preset, then click **Save**. Repeat for each preset.

The app also accepts CLI arguments to skip the file-chooser dialogs:

```bash
patch-probe <plugin.vst3> <output-dir>
```

Output files land in `~/patch-probe/` by default (configurable via the Output button in the bottom bar).

**Auto-save** (on by default): the app polls the plugin state every 100 ms and saves automatically once the state has been stable for 300 ms. The preset name field is pre-filled by auto-detection and can be edited before the save fires.

## Output format

Each saved file is named `NNN_preset_name.yaml` and contains a single `source` block:

```yaml
source:
  type: vst
  plugin: "/path/to/Synth.vst3"
  preset: "Big Pad"
  raw_state: "TVNTbgAA..."
  parameter_state:
    Cutoff: 0.720000
    Resonance: 0.300000
```

| Field | Description |
|---|---|
| `raw_state` | Base64-encoded `getStateInformation()` bytes — primary restore mechanism |
| `parameter_state` | Per-parameter normalized values — fallback if `raw_state` isn't accepted |

## Building from source

Requires a C++17 compiler, CMake 3.22+, and Ninja. On Linux, also install the JUCE system dependencies (ALSA, X11, FreeType).

```bash
git clone --recurse-submodules https://github.com/blablack/patch-probe
cd patch-probe
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
ninja -C build

# Binary lands at:
build/src/patch-probe_artefacts/Release/patch-probe
```
