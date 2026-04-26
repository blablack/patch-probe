juce_add_console_app(patch-probe
    PRODUCT_NAME "patch-probe"
)

juce_generate_juce_header(patch-probe)

target_compile_features(patch-probe PRIVATE cxx_std_17)

target_compile_definitions(patch-probe
    PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=0
    JUCE_PLUGINHOST_LADSPA=0
)

target_sources(patch-probe PRIVATE
    Main.cpp
    PresetScanner.cpp
    YamlWriter.cpp
)

target_link_libraries(patch-probe PRIVATE
    juce::juce_audio_processors_headless
    juce::juce_core
    juce::juce_data_structures
)
