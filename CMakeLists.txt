cmake_minimum_required(VERSION 3.15)
project(TB303VSTi VERSION 0.1.0 LANGUAGES CXX)

set(JUCE_DIR "C:/JUCE")

add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(TB303VSTi
    COMPANY_NAME "vdgbcrypto"
    PLUGIN_MANUFACTURER_CODE vdgb
    PLUGIN_CODE tb03
    FORMATS VST3
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    PRODUCT_NAME "TB-303 VSTi")

target_sources(TB303VSTi PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_link_libraries(TB303VSTi PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra)

juce_generate_juce_header(TB303VSTi)

target_compile_definitions(TB303VSTi PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)
