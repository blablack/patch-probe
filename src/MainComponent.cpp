#include "MainComponent.h"


MainComponent::MainComponent()
{
    outputDir_ = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("patch-probe");

    addAndMakeVisible(loadButton_);
    loadButton_.onClick = [this] { loadPlugin(); };

    addAndMakeVisible(pluginLabel_);
    pluginLabel_.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(nameLabel_);
    addAndMakeVisible(nameField_);
    nameField_.setMultiLine(false);
    nameField_.setText("Preset_001", juce::dontSendNotification);

    addAndMakeVisible(saveButton_);
    saveButton_.onClick = [this] { savePreset(); };
    saveButton_.setEnabled(false);

    addAndMakeVisible(savedLabel_);
    savedLabel_.setJustificationType(juce::Justification::centredLeft);
    savedLabel_.setText("0 saved", juce::dontSendNotification);

    addAndMakeVisible(outputButton_);
    outputButton_.setButtonText("Output: " + outputDir_.getFullPathName());
    outputButton_.onClick = [this]
    {
        fileChooser_ = std::make_unique<juce::FileChooser>("Select output directory", outputDir_);
        fileChooser_->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this](const juce::FileChooser& fc)
            {
                const juce::File result = fc.getResult();
                if (result == juce::File{}) return;
                outputDir_ = result;
                outputButton_.setButtonText("Output: " + outputDir_.getFullPathName());
                if (writer_) writer_ = std::make_unique<YamlWriter>(pluginFile_, outputDir_);
            });
    };

    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    hidePluginEditor();
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(kPad);

    // Top toolbar
    auto toolbar = area.removeFromTop(kToolbarH);
    loadButton_.setBounds(toolbar.removeFromLeft(130));
    toolbar.removeFromLeft(kPad);
    pluginLabel_.setBounds(toolbar);

    // Bottom bar
    area.removeFromBottom(kPad);
    auto bottom = area.removeFromBottom(kBottomH);
    nameLabel_.setBounds(bottom.removeFromLeft(90));
    bottom.removeFromLeft(kPad);
    nameField_.setBounds(bottom.removeFromLeft(220));
    bottom.removeFromLeft(kPad);
    saveButton_.setBounds(bottom.removeFromLeft(70));
    bottom.removeFromLeft(kPad);
    savedLabel_.setBounds(bottom.removeFromLeft(80));
    bottom.removeFromLeft(kPad);
    outputButton_.setBounds(bottom);

    // Plugin editor fills the rest
    area.removeFromTop(kPad);
    if (editor_)
        editor_->setBounds(area);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    if (!scanner_)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(16.0f);
        g.drawText("Load a VST3 plugin to begin", getLocalBounds(), juce::Justification::centred);
    }
}

void MainComponent::applyArgs(const juce::File& pluginFile, const juce::File& outputDir)
{
    if (outputDir != juce::File{})
    {
        outputDir_ = outputDir;
        outputButton_.setButtonText("Output: " + outputDir_.getFullPathName());
    }
    if (pluginFile != juce::File{})
        loadPluginFile(pluginFile);
}

void MainComponent::loadPlugin()
{
    fileChooser_ = std::make_unique<juce::FileChooser>("Select VST3 plugin", juce::File{}, "*.vst3");
    fileChooser_->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const juce::File result = fc.getResult();
            if (result == juce::File{}) return;
            loadPluginFile(result);
        });
}

void MainComponent::loadPluginFile(const juce::File& file)
{
    stopTimer();
    lastSavedState_.reset();
    pendingState_.reset();
    stableTickCount_ = 0;
    hidePluginEditor();
    scanner_.reset();
    writer_.reset();

    pluginFile_ = file;
    pluginLabel_.setText(juce::CharPointer_UTF8("Loading\xe2\x80\xa6"), juce::dontSendNotification);
    saveButton_.setEnabled(false);
    repaint();

    auto newScanner = std::make_unique<PresetScanner>(pluginFile_);
    if (!newScanner->loadPlugin())
    {
        pluginLabel_.setText("Failed to load: " + pluginFile_.getFileName(), juce::dontSendNotification);
        return;
    }

    scanner_ = std::move(newScanner);
    pluginLabel_.setText(scanner_->getPluginName() + "  \xe2\x80\x94  " + scanner_->getManufacturerName(),
                         juce::dontSendNotification);

    outputDir_.createDirectory();
    writer_ = std::make_unique<YamlWriter>(pluginFile_, outputDir_);
    savedCount_ = 0;
    savedLabel_.setText("0 saved", juce::dontSendNotification);
    saveButton_.setEnabled(true);

    showPluginEditor();
    updateNameField();
    scanner_->getPlugin()->getStateInformation(lastSavedState_);
    startTimerHz(10);
}

void MainComponent::showPluginEditor()
{
    if (!scanner_ || !scanner_->getPlugin()) return;

    editor_.reset(scanner_->getPlugin()->createEditorIfNeeded());
    if (!editor_) return;

    addAndMakeVisible(*editor_);
    resized();
}

void MainComponent::hidePluginEditor()
{
    if (editor_)
    {
        removeChildComponent(editor_.get());
        editor_.reset();
    }
}

void MainComponent::updateNameField()
{
    if (!scanner_) return;

    const juce::String detected = scanner_->tryGetPresetName();
    if (detected.isNotEmpty())
    {
        nameField_.setText(detected, juce::dontSendNotification);
    }
    else
    {
        nameField_.setText("Preset_" + juce::String(savedCount_ + 1).paddedLeft('0', 3),
                           juce::dontSendNotification);
    }
}

void MainComponent::timerCallback()
{
    if (!scanner_ || !scanner_->getPlugin()) return;

    juce::MemoryBlock currentState;
    scanner_->getPlugin()->getStateInformation(currentState);

    if (pendingState_.getSize() == 0)
    {
        // Not debouncing — any change from the last save kicks off the window
        if (currentState != lastSavedState_)
        {
            pendingState_ = currentState;
            stableTickCount_ = 1;
        }
    }
    else
    {
        // Debouncing — wait for the state to stop changing
        if (currentState == pendingState_)
        {
            if (++stableTickCount_ >= 3) // 300 ms of stability
            {
                lastSavedState_ = pendingState_;
                pendingState_.reset();
                stableTickCount_ = 0;
                updateNameField();
                savePreset();
            }
        }
        else
        {
            // Still changing — update candidate, reset stability counter
            pendingState_ = currentState;
            stableTickCount_ = 1;
        }
    }
}

void MainComponent::savePreset()
{
    if (!scanner_ || !writer_) return;

    const std::string name = nameField_.getText().toStdString();
    const PresetInfo info = scanner_->captureState(name);
    writer_->write(info);

    // Keep detection state in sync so manual saves don't re-trigger auto-save
    scanner_->getPlugin()->getStateInformation(lastSavedState_);
    pendingState_.reset();
    stableTickCount_ = 0;

    ++savedCount_;
    savedLabel_.setText(juce::String(savedCount_) + " saved", juce::dontSendNotification);
    updateNameField();

    saveButton_.setButtonText("Saved!");
    juce::Timer::callAfterDelay(800, [this] { saveButton_.setButtonText("Save"); });
}
