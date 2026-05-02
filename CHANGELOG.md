# Changelog

## [v0.2.0] - 
### Added
- Complete change of strategy - user manually switching presets

## [v0.1.2] - 2026-04-26
### Fixed
- Odin2 and other JUCE-based VST3 plugins now load without deadlocking
- Fix leaked `VST3HostContextHeadless` instance on close

## [v0.1.1] - 2026-04-26
### Fixed
- Tear down `MessageManager` after all JUCE objects are destroyed to avoid leak assertion on exit

## [v0.1.0] - 2026-04-26
### Added
- Initial release: interactive GUI for capturing VST3 preset state as YAML
- Load any `.vst3` plugin via file chooser or command-line argument
- Browse presets using the plugin's own editor UI
- Auto-detect preset name from `getProgramName` or embedded XML state blob
- Debounced auto-save on preset change (300 ms stability window)
- Configurable output directory
- `raw_state` (base64 `getStateInformation`) + `parameter_state` fallback per YAML output
