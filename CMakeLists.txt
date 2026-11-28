
cmake_minimum_required(VERSION 3.22)

project(PURELURE VERSION 0.1.0)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PURELURE
    COMPANY_NAME "KRYOZONE AUDIO"
    BUNDLE_ID com.kryozoneaudio.purelure
    PLUGIN_MANUFACTURER_CODE Kryo
    PLUGIN_CODE Purl
    FORMATS VST3
    PRODUCT_NAME "PURELURE"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(PURELURE)

target_sources(PURELURE PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(PURELURE PRIVATE cxx_std_17)

target_compile_definitions(PURELURE PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PURELURE PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
