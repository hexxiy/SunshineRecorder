cmake_minimum_required(VERSION 3.22)

project(SunshineRecorder VERSION 1.0.0)

# Generate build timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S" UTC)
set(PROJECT_VERSION_WITH_BUILD "${PROJECT_VERSION} (${BUILD_TIMESTAMP})")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable JUCE features we don't need (avoids webkit/curl dependencies)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# These must be set before add_subdirectory to affect juceaide build
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DJUCE_WEB_BROWSER=0 -DJUCE_USE_CURL=0")

add_subdirectory(JUCE)

juce_add_plugin(SunshineRecorder
    COMPANY_NAME "Sunshine Audio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Snsh
    PLUGIN_CODE Snrc
    FORMATS VST3 Standalone
    PRODUCT_NAME "Sunshine Recorder"
)

target_sources(SunshineRecorder
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Grain.cpp
        Source/DSP/GrainEngine.cpp
        Source/DSP/SampleBuffer.cpp
        Source/DSP/TapeDelay.cpp
        Source/DSP/TapeDisintegrationEngine.cpp
        Source/DSP/TapeDamageProcessor.cpp
        Source/DSP/Voice.cpp
        Source/Parameters/Parameters.cpp
        Source/UI/OccultKnob.cpp
        Source/UI/WaveformDisplay.cpp
        Source/UI/GrainVisualizer.cpp
        Source/UI/OccultLookAndFeel.cpp
)

target_compile_definitions(SunshineRecorder
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
        BUILD_TIMESTAMP="${BUILD_TIMESTAMP}"
)

target_link_libraries(SunshineRecorder
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
