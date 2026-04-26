#include "PresetScanner.h"

#include <cctype>
#include <iostream>

PresetScanner::PresetScanner(const juce::File& pluginFile)
    : pluginFile_(pluginFile)
{
    juce::addHeadlessDefaultFormatsToManager(formatManager_);
}

bool PresetScanner::loadPlugin()
{
    juce::OwnedArray<juce::PluginDescription> descs;

    for (auto* format : formatManager_.getFormats())
    {
        juce::OwnedArray<juce::PluginDescription> tmp;
        format->findAllTypesForFile(tmp, pluginFile_.getFullPathName());
        if (!tmp.isEmpty())
        {
            descs.addCopiesOf(tmp);
            break;
        }
    }

    if (descs.isEmpty())
    {
        std::cerr << "No plugin types found in: " << pluginFile_.getFullPathName() << "\n";
        return false;
    }

    description_ = *descs.getFirst();

    juce::String errorMessage;
    plugin_ = formatManager_.createPluginInstance(description_, 44100.0, 512, errorMessage);

    if (!plugin_)
    {
        std::cerr << "Failed to create plugin: " << errorMessage << "\n";
        return false;
    }

    plugin_->setPlayConfigDetails(0, 2, 44100.0, 512);
    plugin_->prepareToPlay(44100.0, 512);
    return true;
}

juce::String PresetScanner::getPluginName() const
{
    return plugin_ ? plugin_->getName() : description_.name;
}

juce::String PresetScanner::getManufacturerName() const
{
    return description_.manufacturerName;
}

std::vector<PresetInfo> PresetScanner::scanFactory()
{
    std::vector<PresetInfo> result;
    if (!plugin_)
        return result;

    int numPrograms = plugin_->getNumPrograms();
    if (numPrograms == 0)
        return result;

    for (int i = 0; i < numPrograms; ++i)
    {
        plugin_->setCurrentProgram(i);
        juce::Thread::sleep(20);

        std::string name = plugin_->getProgramName(i).toStdString();
        if (name.empty())
            name = "Program " + std::to_string(i);

        result.push_back(captureState(name, "factory", i, ""));
    }

    return result;
}

std::vector<PresetInfo> PresetScanner::scanUser()
{
    std::vector<PresetInfo> result;
    if (!plugin_)
        return result;

    const juce::String manufacturer = description_.manufacturerName;
    const juce::String pluginName = description_.name;

    juce::Array<juce::File> searchDirs;

    // Linux
    searchDirs.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                       .getChildFile(".vst3/presets")
                       .getChildFile(manufacturer)
                       .getChildFile(pluginName));
    searchDirs.add(juce::File("/usr/lib/vst3/presets").getChildFile(manufacturer).getChildFile(pluginName));
    searchDirs.add(juce::File("/usr/local/lib/vst3/presets").getChildFile(manufacturer).getChildFile(pluginName));

    // macOS
    searchDirs.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                       .getChildFile("Library/Audio/Presets")
                       .getChildFile(manufacturer)
                       .getChildFile(pluginName));
    searchDirs.add(juce::File("/Library/Audio/Presets").getChildFile(manufacturer).getChildFile(pluginName));

    for (const auto& dir : searchDirs)
    {
        if (!dir.exists())
            continue;

        juce::Array<juce::File> presetFiles;
        dir.findChildFiles(presetFiles, juce::File::findFiles, true, "*.vstpreset");

        for (const auto& presetFile : presetFiles)
        {
            const std::string name = presetFile.getFileNameWithoutExtension().toStdString();

            // Try to load preset state directly from the file
            const bool loaded = applyVst3PresetFile(presetFile);

            if (loaded)
            {
                result.push_back(captureState(name, "user", -1, presetFile.getFullPathName().toStdString()));
            }
            else
            {
                // Can't load state — still record path so patch-press can try
                PresetInfo info;
                info.name = name;
                info.source = "user";
                info.programIndex = -1;
                info.presetFile = presetFile.getFullPathName().toStdString();
                result.push_back(info);
            }
        }
    }

    return result;
}

bool PresetScanner::applyVst3PresetFile(const juce::File& presetFile)
{
    juce::MemoryBlock fileData;
    if (!presetFile.loadFileAsData(fileData))
        return false;

    struct Loader : juce::ExtensionsVisitor
    {
        const juce::MemoryBlock& data;
        bool success = false;
        explicit Loader(const juce::MemoryBlock& d) : data(d) {}
        void visitVST3Client(const VST3Client& vst3) override { success = vst3.setPreset(data); }
    };

    Loader loader(fileData);
    plugin_->getExtensions(loader);
    return loader.success;
}

PresetInfo PresetScanner::captureState(const std::string& name,
                                       const std::string& source,
                                       int programIndex,
                                       const std::string& presetFile)
{
    PresetInfo info;
    info.name = name;
    info.source = source;
    info.programIndex = programIndex;
    info.presetFile = presetFile;

    juce::MemoryBlock state;
    plugin_->getStateInformation(state);
    info.rawStateB64 = juce::Base64::toBase64(state.getData(), state.getSize()).toStdString();

    for (auto* param : plugin_->getParameters())
    {
        std::string key;
        if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            key = p->paramID.toStdString();
        else
            key = param->getName(64).toStdString();

        // Normalize: lowercase, spaces -> underscores
        for (auto& c : key)
        {
            if (c == ' ' || c == '-')
                c = '_';
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        info.parameters[key] = param->getValue();
    }

    return info;
}
