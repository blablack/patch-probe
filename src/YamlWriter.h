#pragma once

#include "PresetScanner.h"
#include <JuceHeader.h>

class YamlWriter
{
  public:
    YamlWriter(const juce::File& pluginFile,
               const juce::File& outputDir,
               const juce::String& profile,
               const juce::String& format,
               const juce::String& outputPath);

    // Returns the written file.
    juce::File write(const PresetInfo& preset);

  private:
    static juce::String sanitize(const juce::String& name);
    static juce::String yamlQuote(const juce::String& s);

    juce::File pluginFile_;
    juce::File outputDir_;
    juce::String profile_;
    juce::String format_;
    juce::String outputPath_;
    juce::String pluginStem_;
};
