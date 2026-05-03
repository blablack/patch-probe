#include <JuceHeader.h>

#include "MainComponent.h"

class MainWindow : public juce::DocumentWindow
{
   public:
    MainWindow()
        : DocumentWindow("patch-probe",
                         juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                             juce::ResizableWindow::backgroundColourId),
                         juce::DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MainComponent(), true);
        setResizable(true, false);
        setResizeLimits(800, 600, 800, 600);
        centreWithSize(800, 600);
        setVisible(true);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class PatchProbeApp : public juce::JUCEApplication
{
   public:
    const juce::String getApplicationName() override { return "patch-probe"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& commandLine) override
    {
        mainWindow = std::make_unique<MainWindow>();

        const juce::StringArray args = juce::StringArray::fromTokens(commandLine, true);
        const juce::File plugin = args.size() >= 1 ? juce::File(args[0]) : juce::File{};
        const juce::File output = args.size() >= 2 ? juce::File(args[1]) : juce::File{};

        if (plugin != juce::File{} || output != juce::File{})
        {
            auto* mc = dynamic_cast<MainComponent*>(mainWindow->getContentComponent());
            if (mc) mc->applyArgs(plugin, output);
        }
    }

    void shutdown() override { mainWindow.reset(); }

    void systemRequestedQuit() override { quit(); }

   private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(PatchProbeApp)
