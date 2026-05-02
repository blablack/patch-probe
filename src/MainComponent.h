#pragma once

#include <JuceHeader.h>

#include "PresetScanner.h"
#include "YamlWriter.h"

class MainComponent : public juce::Component, private juce::Timer
{
   public:
    MainComponent();
    ~MainComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void applyArgs(const juce::File& pluginFile, const juce::File& outputDir);

   private:
    void loadPlugin();
    void loadPluginFile(const juce::File& file);
    void savePreset();
    void updateNameField();
    void showPluginEditor();
    void hidePluginEditor();
    void timerCallback() override;

    // --- Plugin state ---
    juce::File pluginFile_;
    juce::File outputDir_;
    std::unique_ptr<PresetScanner> scanner_;
    std::unique_ptr<YamlWriter> writer_;
    int savedCount_ = 0;
    juce::MemoryBlock lastSavedState_;
    juce::MemoryBlock pendingState_;
    int stableTickCount_ = 0;

    // --- Plugin editor ---
    std::unique_ptr<juce::AudioProcessorEditor> editor_;

    // --- UI ---
    juce::TextButton loadButton_{juce::CharPointer_UTF8("Load Plugin\xe2\x80\xa6")};
    juce::Label pluginLabel_;

    juce::Label nameLabel_{"nameLabel", "Preset name:"};
    juce::TextEditor nameField_;
    juce::TextButton saveButton_{"Save"};
    juce::Label savedLabel_;
    juce::TextButton outputButton_{"Output: (none)"};
    std::unique_ptr<juce::FileChooser> fileChooser_;

    static constexpr int kToolbarH = 36;
    static constexpr int kBottomH = 40;
    static constexpr int kPad = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
