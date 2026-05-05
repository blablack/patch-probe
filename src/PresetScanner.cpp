#include "PresetScanner.h"

#include <iostream>


PresetScanner::PresetScanner(const juce::File& pluginFile) : pluginFile_(pluginFile)
{
    juce::addDefaultFormatsToManager(formatManager_);
}

PresetScanner::~PresetScanner()
{
    if (plugin_) plugin_->releaseResources();
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

    if (descs.isEmpty()) return false;

    description_ = *descs.getFirst();

    juce::String errorMessage;
    plugin_ = formatManager_.createPluginInstance(description_, 44100.0, 512, errorMessage);
    if (!plugin_) return false;

    plugin_->setPlayConfigDetails(0, 2, 44100.0, 512);
    plugin_->prepareToPlay(44100.0, 512);

    return true;
}

juce::String PresetScanner::getPluginName() const
{
    return plugin_ ? plugin_->getName() : description_.name;
}

juce::String PresetScanner::getManufacturerName() const { return description_.manufacturerName; }

juce::String PresetScanner::tryGetPresetName() const
{
    if (!plugin_) return {};

    // 1. Try getProgramName(getCurrentProgram())
    const int prog = plugin_->getCurrentProgram();
    const juce::String progName = plugin_->getProgramName(prog);
    const juce::StringArray generics{"Default", "Init", "Untitled"};
    if (progName.isNotEmpty() && !generics.contains(progName) && !progName.startsWith("Program "))
        return progName;

    // 2. Try parsing the inner JUCE XML state blob for a name attribute
    juce::MemoryBlock state;
    plugin_->getStateInformation(state);

    if (const auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), (int)state.getSize()))
    {
        static constexpr const char* kNameAttrs[] = {"patch_name", "presetName", "name", "preset_name", "currentPreset"};

        // Check root element attributes first
        for (const char* attr : kNameAttrs)
        {
            const juce::String val = xml->getStringAttribute(attr);
            if (val.isNotEmpty()) return val;
        }

        for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
        {
            for (const char* attr : kNameAttrs)
            {
                const juce::String val = child->getStringAttribute(attr);
                if (val.isNotEmpty()) return val;
            }
        }

        if (const auto* comp = xml->getChildByName("IComponent"))
        {
            juce::MemoryBlock inner;
            inner.fromBase64Encoding(comp->getAllSubText());

            if (const auto inner_xml =
                    juce::AudioProcessor::getXmlFromBinary(inner.getData(), (int)inner.getSize()))
            {
                for (const char* attr : kNameAttrs)
                {
                    const juce::String val = inner_xml->getStringAttribute(attr);
                    if (val.isNotEmpty()) return val;
                }

                for (auto* child = inner_xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
                {
                    for (const char* attr : kNameAttrs)
                    {
                        const juce::String val = child->getStringAttribute(attr);
                        if (val.isNotEmpty()) return val;
                    }
                }

                std::cout << "[patch-probe] tryGetPresetName: no name found.\n"
                          << "  inner xml:\n" << inner_xml->toString().toStdString() << "\n";
            }
            else
            {
                std::cout << "[patch-probe] tryGetPresetName: IComponent decoded but getXmlFromBinary failed "
                          << "(non-XML inner state, " << inner.getSize() << " bytes).\n"
                          << "  outer xml:\n" << xml->toString().toStdString() << "\n";
            }
        }
        else
        {
            std::cout << "[patch-probe] tryGetPresetName: no IComponent found.\n"
                      << "  outer xml:\n" << xml->toString().toStdString() << "\n";
        }
    }
    else
    {
        std::cout << "[patch-probe] tryGetPresetName: getXmlFromBinary failed on outer state.\n";
    }

    return {};
}

void PresetScanner::clearNoteState() const
{
    if (!plugin_) return;

    // All Notes Off on all channels — clears the mono/legato note stack so plugins
    // like Odin2 don't bake an active note into the serialized state.
    juce::MidiBuffer midi;
    for (int ch = 1; ch <= 16; ++ch)
        midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);

    juce::AudioBuffer<float> audio(plugin_->getTotalNumOutputChannels(), 512);
    audio.clear();
    plugin_->processBlock(audio, midi);

    juce::MidiBuffer empty;
    for (int i = 0; i < 8; ++i)
    {
        audio.clear();
        plugin_->processBlock(audio, empty);
    }
}

PresetInfo PresetScanner::captureState(const std::string& name) const
{
    clearNoteState();

    PresetInfo info;
    info.name = name;

    juce::MemoryBlock state;
    plugin_->getStateInformation(state);
    info.rawStateB64 = juce::Base64::toBase64(state.getData(), state.getSize()).toStdString();

    for (auto* param : plugin_->getParameters())
    {
        if (param)
            info.parameterState.emplace_back(param->getName(128).toStdString(), param->getValue());
    }

    return info;
}
