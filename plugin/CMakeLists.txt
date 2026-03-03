cmake_minimum_required(VERSION 3.22)

project(HendrixFlanger VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS: universal binary (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")
endif()

# ---------------------------------------------------------------------------
# Fetch JUCE (pinned to 8.0.4)
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin sources
# ---------------------------------------------------------------------------
set(PLUGIN_SOURCES
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/dsp/DelayLine.h
    src/dsp/DCBlocker.h
    src/dsp/LFO.h
    src/dsp/EnvelopeFollower.h
    src/dsp/ThroughZeroFlanger.h
    src/dsp/DubEcho.h
)

juce_add_plugin(HendrixFlanger
    COMPANY_NAME           "FurmanAudio"
    BUNDLE_ID              "com.furmanaudio.hendrixflanger"
    PLUGIN_MANUFACTURER_CODE Furm
    PLUGIN_CODE            HxFl

    FORMATS AU VST3 Standalone

    PRODUCT_NAME "Hendrix Flanger"

    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    PLUGIN_AU_EXPORT_PREFIX HendrixFlangerAU
    AU_MAIN_TYPE            kAudioUnitType_Effect

    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(HendrixFlanger)

target_sources(HendrixFlanger PRIVATE ${PLUGIN_SOURCES})

target_compile_definitions(HendrixFlanger
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(HendrixFlanger
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(HendrixFlanger PRIVATE /W4)
else()
    target_compile_options(HendrixFlanger PRIVATE -Wall -Wextra -Wpedantic)
endif()
