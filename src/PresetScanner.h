#pragma once

#include <JuceHeader.h>

#include <string>

struct PresetInfo
{
    std::string name;
    std::string rawStateB64;
};

class PresetScanner
{
   public:
    explicit PresetScanner(const juce::File& pluginFile);
    ~PresetScanner();

    bool loadPlugin();

    juce::String getPluginName() const;
    juce::String getManufacturerName() const;

    // Best-effort: tries getProgramName → state blob XML → "" (caller supplies fallback).
    juce::String tryGetPresetName() const;

    PresetInfo captureState(const std::string& name) const;

    juce::AudioPluginInstance* getPlugin() const { return plugin_.get(); }

   private:
    juce::File pluginFile_;
    juce::AudioPluginFormatManager formatManager_;
    juce::PluginDescription description_;
    std::unique_ptr<juce::AudioPluginInstance> plugin_;
};
