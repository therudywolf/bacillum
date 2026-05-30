cmake_minimum_required(VERSION 3.22)

project(Bacillum
    VERSION 0.6.0
    DESCRIPTION "Virtual-Analog polyphonic synthesizer"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# Universal binary on macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "" FORCE)
endif()

# Fetch JUCE
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

# Plugin target
juce_add_plugin(Bacillum
    COMPANY_NAME              "Bacillum"
    BUNDLE_ID                 com.bacillum.synth
    PLUGIN_MANUFACTURER_CODE  Bclm
    PLUGIN_CODE               Bcsy
    FORMATS                   VST3 Standalone
    PRODUCT_NAME              "Bacillum"
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Instrument" "Synth"
    AU_MAIN_TYPE              "kAudioUnitType_MusicDevice"
)

target_sources(Bacillum PRIVATE
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/params/Params.cpp
    source/dsp/voice/Voice.cpp
    source/dsp/voice/VoiceManager.cpp
)

target_include_directories(Bacillum PRIVATE source)

target_compile_definitions(Bacillum PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_FLAC=0
    JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_USE_LAME_AUDIO_FORMAT=0
    JUCE_USE_WINDOWS_MEDIA_FORMAT=0
)

target_link_libraries(Bacillum
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compiler-specific tightening
if(MSVC)
    # /utf-8 makes MSVC treat source files as UTF-8 — without this any non-ASCII
    # character in string literals (em-dash, arrows, Cyrillic) gets misinterpreted
    # via the system code page and ends up as mojibake on screen.
    target_compile_options(Bacillum PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
else()
    target_compile_options(Bacillum PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()
