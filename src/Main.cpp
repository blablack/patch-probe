#include <JuceHeader.h>
#include <iostream>

#include "PresetScanner.h"
#include "YamlWriter.h"

static void printUsage()
{
    std::cerr << "Usage: patch-probe <plugin-path> <output-dir> [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --factory-only          Skip user presets\n"
              << "  --user-only             Skip factory presets\n"
              << "  --profile <name>        Profile: synth (default), drums, pad\n"
              << "  --format <name>         Output format: deluge (default), wav\n"
              << "  --output-path <path>    Destination path written into YAML output field\n"
              << "\n"
              << "Output: one YAML file per preset, compatible with patch-press.\n";
}

int main(int argc, char* argv[])
{
    // Set up MessageManager for this (single) thread without GUI/display
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

    if (argc < 3)
    {
        printUsage();
        return 1;
    }

    const juce::File pluginFile(juce::String::fromUTF8(argv[1]));
    const juce::File outputDir(juce::String::fromUTF8(argv[2]));

    if (!pluginFile.exists())
    {
        std::cerr << "Plugin not found: " << argv[1] << "\n";
        return 1;
    }

    bool factoryOnly = false;
    bool userOnly = false;
    juce::String profile = "synth";
    juce::String format = "deluge";
    juce::String outputPath = "PLACEHOLDER";

    for (int i = 3; i < argc; ++i)
    {
        juce::String arg(argv[i]);
        if (arg == "--factory-only")
            factoryOnly = true;
        else if (arg == "--user-only")
            userOnly = true;
        else if (arg == "--profile" && i + 1 < argc)
            profile = argv[++i];
        else if (arg == "--format" && i + 1 < argc)
            format = argv[++i];
        else if (arg == "--output-path" && i + 1 < argc)
            outputPath = argv[++i];
        else
        {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    if (factoryOnly && userOnly)
    {
        std::cerr << "--factory-only and --user-only are mutually exclusive\n";
        return 1;
    }

    outputDir.createDirectory();

    PresetScanner scanner(pluginFile);

    std::cout << "Loading: " << pluginFile.getFullPathName() << "\n";

    if (!scanner.loadPlugin())
        return 1;

    std::cout << "Plugin : " << scanner.getPluginName()
              << " by " << scanner.getManufacturerName() << "\n\n";

    std::vector<PresetInfo> presets;

    if (!userOnly)
    {
        auto factory = scanner.scanFactory();
        std::cout << "Factory presets: " << factory.size() << "\n";
        presets.insert(presets.end(), factory.begin(), factory.end());
    }

    if (!factoryOnly)
    {
        auto user = scanner.scanUser();
        std::cout << "User presets   : " << user.size() << "\n";
        presets.insert(presets.end(), user.begin(), user.end());
    }

    std::cout << "Total          : " << presets.size() << "\n\n";

    if (presets.empty())
    {
        std::cout << "No presets found — nothing written.\n";
        return 0;
    }

    YamlWriter writer(pluginFile, outputDir, profile, format, outputPath);

    for (const auto& preset : presets)
    {
        const juce::File out = writer.write(preset);
        std::cout << "[" << (preset.source == "factory" ? "F" : "U") << "] "
                  << preset.name << " -> " << out.getFileName() << "\n";
    }

    std::cout << "\nWrote " << presets.size() << " configs to: "
              << outputDir.getFullPathName() << "\n";

    return 0;
}
