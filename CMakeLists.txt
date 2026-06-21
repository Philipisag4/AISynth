cmake_minimum_required(VERSION 3.22)

project(AISynth VERSION 1.0.0
        DESCRIPTION "AI-powered text-to-preset synthesizer"
        LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# JUCE – fetched automatically if not provided by the user
# ---------------------------------------------------------------------------
option(AISYNTH_USE_SYSTEM_JUCE "Use a system-installed JUCE instead of FetchContent" OFF)

if(AISYNTH_USE_SYSTEM_JUCE)
    # User must point JUCE_PATH to their local JUCE clone
    if(NOT DEFINED JUCE_PATH)
        message(FATAL_ERROR "AISYNTH_USE_SYSTEM_JUCE=ON but JUCE_PATH is not set. "
                            "Pass -DJUCE_PATH=/path/to/JUCE")
    endif()
    add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(AISynth
    COMPANY_NAME           "AI Synth Labs"
    PLUGIN_MANUFACTURER_ID AISL
    PLUGIN_CODE            AIsn
    FORMATS                VST3 Standalone
    PRODUCT_NAME           "AI Synth"
    IS_SYNTH               TRUE
    NEEDS_MIDI_INPUT       TRUE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
target_sources(AISynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthEngine.cpp
    Source/Voice.cpp
    Source/Oscillator.cpp
    Source/Filter.cpp
    Source/Envelope.cpp
    Source/FXChain.cpp
    Source/TextToPreset.cpp
    Source/PresetManager.cpp
    Source/SpectrumAnalyzer.cpp
    Source/Wavetable.cpp
    Source/Components/KnobComponent.cpp
    Source/Components/WaveformDisplay.cpp
    Source/Components/SpectrumDisplay.cpp
    Source/Components/PresetBrowser.cpp
    Source/Components/TextInputBox.cpp)

target_compile_definitions(AISynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AISynth PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Copy the built VST3 into a well-known folder on Windows for easy install
# (after build, the plugin will be in build/AISynth_artefacts/Release/VST3)
# ---------------------------------------------------------------------------
if(WIN32)
    add_custom_command(TARGET AISynth POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
        "VST3 built. Run the Inno Setup script in Installer/ to create the installer.")
endif()
