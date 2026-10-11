cmake_minimum_required(VERSION 3.22)
project(DrumNSpace VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(DrumNSpace
    PRODUCT_NAME "DrumNSpace"
    COMPANY_NAME "Eric Silverman"
    BUNDLE_ID "com.ericsilverman.drumnspace"
    FORMATS VST3 AU Standalone
    PLUGIN_MANUFACTURER_CODE Ersv
    PLUGIN_CODE Spdr
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Drum
    ICON_BIG resources/icon.png  # regenerate with: swift resources/make_icon.swift
    COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(DrumNSpace PRIVATE src/PluginProcessor.cpp src/PluginEditor.cpp)

set(UI_PNG space_drum_assets/png)
juce_add_binary_data(DrumNSpaceAssets
    SOURCES
        assets/fonts/SpaceGrotesk-Bold.ttf
        ${UI_PNG}/enclosure-frame.png
        ${UI_PNG}/cosmic-trails.png
        ${UI_PNG}/stars-sparkles.png
        ${UI_PNG}/planet-orbit.png
        ${UI_PNG}/knob-tick-scale.png
        ${UI_PNG}/knob-large.png
        ${UI_PNG}/knob-medium.png
        ${UI_PNG}/knob-small.png
        ${UI_PNG}/toggle-on.png
        ${UI_PNG}/toggle-off.png
        ${UI_PNG}/strike-pad-idle.png
        ${UI_PNG}/strike-pad-pressed.png
        ${UI_PNG}/led-lit.png
        ${UI_PNG}/led-unlit.png)
target_include_directories(DrumNSpace PRIVATE src)
target_compile_definitions(DrumNSpace PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(DrumNSpace
    PRIVATE juce::juce_audio_utils DrumNSpaceAssets
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
