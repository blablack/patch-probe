# patch-probe

Headless CLI tool that loads a VST3 plugin, enumerates every preset (factory and user), and writes one YAML file per preset containing the full plugin state.

**Why it exists:** Python's [pedalboard](https://github.com/spotify/pedalboard) library cannot change presets at runtime, but it can restore a plugin's state from a saved blob. patch-probe extracts that state for every preset up front so pedalboard can load each one directly:

```python
synth.raw_state = base64.b64decode(preset["source"]["raw_state"])
```

## Usage

```
patch-probe <plugin.vst3> <output-dir> [OPTIONS]

Options:
  --factory-only    Only scan factory presets (skip user preset dirs)
  --user-only       Only scan user preset files (skip factory programs)
```

Each YAML output file contains a single `source` block:

```yaml
source:
  type: vst
  plugin: /path/to/Synth.vst3
  preset: "Big Pad"
  preset_source: factory
  program_index: 42
  raw_state: "TVNTbgAA..."        # base64 getStateInformation() bytes — primary restore
  parameter_state:                # per-parameter normalized values — fallback
    Cutoff: 0.72
    Resonance: 0.3
```

User presets include a `preset_file` field instead of `program_index`:

```yaml
source:
  type: vst
  plugin: /path/to/Synth.vst3
  preset: "My Patch"
  preset_source: user
  preset_file: /home/user/.vst3/presets/Synth/My Patch.vstpreset
  raw_state: "TVNTbgAA..."
  parameter_state:
    Cutoff: 0.55
```

## Download

Pre-built binaries for Linux, macOS, and Windows are available on the [Releases](../../releases) page.

## Building from source

Requires a C++17 compiler, CMake 3.22+, and Ninja. On Linux, also install the JUCE system dependencies (ALSA, X11, FreeType — see the CI workflow for the full list).

```bash
git clone --recurse-submodules https://github.com/blablack/patch-probe
cd patch-probe
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

# Binary lands at:
build/src/patch-probe_artefacts/Release/patch-probe
```

## Python consumer example

```python
import yaml, base64, pathlib
from pedalboard import load_plugin

plugin = load_plugin("/path/to/Synth.vst3")
preset_dir = pathlib.Path("/path/to/presets")

def load_preset(yaml_path):
    with open(yaml_path) as f:
        data = yaml.safe_load(f)
    src = data["source"]
    plugin.raw_state = base64.b64decode(src["raw_state"])
```
