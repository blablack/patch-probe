#include <JuceHeader.h>
#include <iostream>

#include "PresetScanner.h"
#include "YamlWriter.h"

static void printUsage()
{
    std::cerr << "Usage: patch-probe <plugin-path> <output-dir> [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --factory-only    Skip user presets\n"
              << "  --user-only       Skip factory presets\n"
              << "\n"
              << "Output: one YAML file per preset containing the plugin state.\n";
}

static int run(int argc, char* argv[])
{
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

    for (int i = 3; i < argc; ++i)
    {
        juce::String arg(argv[i]);
        if (arg == "--factory-only")
            factoryOnly = true;
        else if (arg == "--user-only")
            userOnly = true;
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

    YamlWriter writer(pluginFile, outputDir);

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

int main(int argc, char* argv[])
{
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

    const int exitCode = run(argc, argv);

    // All JUCE objects created in run() are destroyed before we reach here,
    // so it's safe to tear down the MessageManager.
    juce::DeletedAtShutdown::deleteAll();
    juce::MessageManager::deleteInstance();

    return exitCode;
}
