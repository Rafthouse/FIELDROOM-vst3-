cmake_minimum_required(VERSION 3.22)
project(FIELDROOM VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform architecture configuration
if(APPLE)
    # Support Universal 2 build (arm64 + x86_64) on macOS
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    endif()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target" FORCE)
endif()

# Fetch JUCE framework (v7.0.9)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Optional post-build system folder copy (disabled by default on CI to avoid permission errors)
option(FIELDROOM_COPY_PLUGIN_AFTER_BUILD "Copy plugin to system folders after build" OFF)

# Configure VST3 & AU Plugin Target
juce_add_plugin(FIELDROOM
    COMPANY_NAME "AcousticLab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${FIELDROOM_COPY_PLUGIN_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE AcLb
    PLUGIN_CODE FldR
    FORMATS VST3 AU
    PRODUCT_NAME "FIELDROOM"
    BUNDLE_ID "com.acousticlab.fieldroom"
)

# Compiler warnings and optimization flags
if(MSVC)
    target_compile_options(FIELDROOM PRIVATE
        /W4 /WX- /permissive- /fp:fast /utf-8
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL>
    )
    target_compile_definitions(FIELDROOM PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
    target_link_options(FIELDROOM PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
else()
    target_compile_options(FIELDROOM PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable
        -ffast-math
        $<$<CONFIG:Release>:-O3>
    )
endif()

# Plugin Source Files
target_sources(FIELDROOM PRIVATE
    # Processor & Editor
    src/PluginProcessor.h
    src/PluginProcessor.cpp
    src/PluginEditor.h
    src/PluginEditor.cpp

    # Parameters & State
    src/Parameters/Parameters.h
    src/Parameters/Parameters.cpp

    # DSP Subsystem
    src/DSP/BinauralRenderer.h
    src/DSP/BinauralRenderer.cpp
    src/DSP/HRTFManager.h
    src/DSP/HRTFManager.cpp
    src/DSP/ConvolutionEngine.h
    src/DSP/ConvolutionEngine.cpp
    src/DSP/SpeakerGeometry.h
    src/DSP/SpeakerGeometry.cpp
    src/DSP/RoomEngine.h
    src/DSP/RoomEngine.cpp
    src/DSP/HeadphoneCalibration.h
    src/DSP/HeadphoneCalibration.cpp
    src/DSP/MonitoringTypes.h
    src/DSP/MonitoringUtilities.h
    src/DSP/MonitoringUtilities.cpp

    # User Interface
    src/UI/CustomLookAndFeel.h
    src/UI/CustomLookAndFeel.cpp
    src/UI/SpeakerViewComponent.h
    src/UI/SpeakerViewComponent.cpp

    # Presets & Profiles
    src/Presets/PresetManager.h
    src/Presets/PresetManager.cpp
    src/Profiles/UserProfileManager.h
    src/Profiles/UserProfileManager.cpp
)

# Target include directories
target_include_directories(FIELDROOM PRIVATE
    src
    src/DSP
    src/Parameters
    src/UI
    src/Presets
    src/Profiles
)

target_compile_definitions(FIELDROOM PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link JUCE Modules
target_link_libraries(FIELDROOM PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
)

juce_generate_juce_header(FIELDROOM)

# Embedded Binary Resources (Headphone Profiles & Default HRTF)
juce_add_binary_data(FieldroomResources
    SOURCES
        resources/headphones/profiles_manifest.json
        resources/rooms/room_configs.json
)
target_link_libraries(FIELDROOM PRIVATE FieldroomResources)

# Automated DSP Tests Executable
enable_testing()
add_executable(fieldroom_dsp_tests
    tests/dsp/test_dsp_core.cpp
    src/DSP/BinauralRenderer.cpp
    src/DSP/HRTFManager.cpp
    src/DSP/ConvolutionEngine.cpp
    src/DSP/SpeakerGeometry.cpp
    src/DSP/RoomEngine.cpp
    src/DSP/HeadphoneCalibration.cpp
    src/DSP/MonitoringUtilities.cpp
)

if(MSVC)
    target_compile_options(fieldroom_dsp_tests PRIVATE /utf-8)
    target_compile_definitions(fieldroom_dsp_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_include_directories(fieldroom_dsp_tests PRIVATE
    src
    src/DSP
)

target_compile_definitions(fieldroom_dsp_tests PRIVATE
    JUCE_USE_CURL=0
)

target_link_libraries(fieldroom_dsp_tests PRIVATE
    juce::juce_dsp
    juce::juce_core
)

add_test(NAME FieldroomDSPTests COMMAND fieldroom_dsp_tests)
