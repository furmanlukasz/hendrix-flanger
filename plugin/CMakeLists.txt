cmake_minimum_required(VERSION 3.22)

# =============================================================================
# TODO: Change these for your plugin
# =============================================================================
project(MyPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS: universal binary (Apple Silicon + Intel)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary")
endif()

# ---------------------------------------------------------------------------
# Fetch JUCE (pinned to 8.0.4 — update as needed)
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
# Plugin sources — add your DSP modules here
# ---------------------------------------------------------------------------
set(PLUGIN_SOURCES
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    # DSP modules (add yours below)
    src/dsp/GainProcessor.h
)

# =============================================================================
# TODO: Customise all fields below for your plugin
# =============================================================================
juce_add_plugin(MyPlugin
    COMPANY_NAME           "MyCompany"
    BUNDLE_ID              "com.mycompany.myplugin"
    PLUGIN_MANUFACTURER_CODE Myco          # 4-char manufacturer code
    PLUGIN_CODE            Mypl            # 4-char plugin code

    FORMATS AU VST3 Standalone             # Add AUv3 for iOS

    PRODUCT_NAME "My Plugin"               # Name shown in DAW

    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    PLUGIN_AU_EXPORT_PREFIX MyPluginAU
    AU_MAIN_TYPE            kAudioUnitType_Effect

    # Auto-install to system plugin folder after build (handy for testing)
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(MyPlugin)

target_sources(MyPlugin PRIVATE ${PLUGIN_SOURCES})

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(MyPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compiler warnings
if(MSVC)
    target_compile_options(MyPlugin PRIVATE /W4)
else()
    target_compile_options(MyPlugin PRIVATE -Wall -Wextra -Wpedantic)
endif()
