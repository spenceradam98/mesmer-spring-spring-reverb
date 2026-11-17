cmake_minimum_required(VERSION 3.22)

# macOS 11 (Big Sur) on Intel is the minimum; build universal (x86_64 + arm64) on CI.
# No -march flags anywhere: the code is scalar C++ and runs on any x86-64 (no AVX2 needed).
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architectures")
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # static CRT: no VC++ redist needed
endif()
project(MesmerSpring VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Core engine (no JUCE dependency, unit-testable) ----
add_library(mesmer_engine STATIC Source/engine/Engine.cpp Source/engine/Presets.cpp)
target_include_directories(mesmer_engine PUBLIC Source/engine)
set_target_properties(mesmer_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(MESMER_BUILD_TESTS "Build engine tests" ON)
if(MESMER_BUILD_TESTS)
    add_executable(mesmer_tests tests/EngineTests.cpp)
    target_link_libraries(mesmer_tests PRIVATE mesmer_engine)
    enable_testing()
    add_test(NAME engine COMMAND mesmer_tests)
    add_executable(mesmer_demo tools/RenderDemo.cpp)   # renders docs/demo.wav
    target_link_libraries(mesmer_demo PRIVATE mesmer_engine)
endif()

option(MESMER_BUILD_PLUGIN "Build the JUCE plugin" ON)
if(NOT MESMER_BUILD_PLUGIN)
    return()
endif()

# ---- JUCE ----
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MesmerSpring
    COMPANY_NAME "MesmerAudio"
    PLUGIN_MANUFACTURER_CODE Msmr
    PLUGIN_CODE Ms1s
    FORMATS VST3 Standalone
    PRODUCT_NAME "Mesmer Spring"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Reverb Delay Modulation)

target_sources(MesmerSpring PRIVATE
    Source/plugin/PluginProcessor.cpp
    Source/plugin/PluginEditor.cpp)

target_compile_definitions(MesmerSpring PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MesmerSpring
    PRIVATE
        mesmer_engine
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Developer tool: render the UI in every theme to PNG (not shipped) ----
option(MESMER_BUILD_SNAPSHOT "Build the UI snapshot tool" OFF)
if(MESMER_BUILD_SNAPSHOT)
    add_executable(mesmer_snapshot tools/Snapshot.cpp)
    target_link_libraries(mesmer_snapshot PRIVATE MesmerSpring mesmer_engine juce::juce_audio_utils)
endif()
