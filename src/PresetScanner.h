#pragma once

#include <JuceHeader.h>
#include <map>
#include <string>
#include <vector>

struct PresetInfo
{
    std::string name;
    std::string source;      // "factory" or "user"
    int programIndex;        // factory index, -1 for user presets
    std::string presetFile;  // path to .vstpreset (user presets only)
    std::string rawStateB64; // base64-encoded getStateInformation() output
    std::map<std::string, float> parameters; // paramID -> normalized 0..1
};

class PresetScanner
{
  public:
    explicit PresetScanner(const juce::File& pluginFile);

    bool loadPlugin();

    std::vector<PresetInfo> scanFactory();
    std::vector<PresetInfo> scanUser();

    juce::String getPluginName() const;
    juce::String getManufacturerName() const;

  private:
    PresetInfo captureState(const std::string& name,
                            const std::string& source,
                            int programIndex,
                            const std::string& presetFile);

    bool applyVst3PresetFile(const juce::File& presetFile);

    juce::File pluginFile_;
    juce::AudioPluginFormatManager formatManager_;
    juce::PluginDescription description_;
    std::unique_ptr<juce::AudioPluginInstance> plugin_;
};
