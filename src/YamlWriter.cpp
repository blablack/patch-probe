#include "YamlWriter.h"

YamlWriter::YamlWriter(const juce::File& pluginFile, const juce::File& outputDir)
    : pluginFile_(pluginFile), outputDir_(outputDir)
{
}

juce::File YamlWriter::write(const PresetInfo& preset)
{
    const juce::String safeName = sanitize(juce::String::fromUTF8(preset.name.c_str()));
    const juce::String presetName = juce::String::fromUTF8(preset.name.c_str());

    juce::String content;
    content << "source:\n";
    content << "  type: vst\n";
    content << "  plugin: " << pluginFile_.getFullPathName() << "\n";
    content << "  preset: " << yamlQuote(presetName) << "\n";

    if (!preset.rawStateB64.empty())
        content << "  raw_state: " << yamlQuote(juce::String::fromUTF8(preset.rawStateB64.c_str())) << "\n";

    juce::File outFile = outputDir_.getChildFile(safeName + ".yaml");
    for (int idx = 2; outFile.exists(); ++idx)
        outFile = outputDir_.getChildFile(safeName + "_" + juce::String(idx) + ".yaml");

    outFile.replaceWithText(content, false, false);
    return outFile;
}

juce::String YamlWriter::sanitize(const juce::String& name)
{
    juce::String result;
    for (int i = 0; i < name.length(); ++i)
    {
        const juce::juce_wchar c = name[i];
        if (juce::CharacterFunctions::isLetterOrDigit(c))
            result += c;
        else if (c == ' ' || c == '-' || c == '_')
            result += '_';
    }
    while (result.contains("__"))
        result = result.replace("__", "_");
    return result.trimCharactersAtStart("_").trimCharactersAtEnd("_").toLowerCase();
}

juce::String YamlWriter::yamlQuote(const juce::String& s)
{
    return "\"" + s.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
}
