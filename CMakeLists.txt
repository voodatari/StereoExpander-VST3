cmake_minimum_required(VERSION 3.22)

# ccache + MSVC necesita info de depuración embebida (/Z7) en lugar de un .pdb compartido
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

project(StereoExpander VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS: Apple Silicon (M1..M4). Para un binario universal: -DSE_UNIVERSAL=ON
option(SE_UNIVERSAL "Compilar arm64 + x86_64 en macOS" OFF)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
    if(SE_UNIVERSAL)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    else()
        set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    endif()
endif()

# Windows: runtime estático para no depender de vcredist
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.9
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(StereoExpander
    COMPANY_NAME                "VooD"
    PLUGIN_MANUFACTURER_CODE    Vood
    PLUGIN_CODE                 SExS
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Stereo Expander"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Stereo
    AU_MAIN_TYPE                kAudioUnitType_Effect
    BUNDLE_ID                   com.vood.stereoexpander)

target_sources(StereoExpander PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/StereoExpanderDSP.h)

target_compile_definitions(StereoExpander PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(StereoExpander
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Test del DSP (sin JUCE):  cmake -DSE_BUILD_TESTS=ON ... && ctest
option(SE_BUILD_TESTS "Compilar los tests del DSP" OFF)
if(SE_BUILD_TESTS)
    enable_testing()
    add_executable(dsp_test tests/dsp_test.cpp)
    add_test(NAME dsp COMMAND dsp_test)
endif()
