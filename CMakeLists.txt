cmake_minimum_required(VERSION 3.22)
project(ToxicRig VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS")
# Universal binary on Mac: runs natively on Apple Silicon and Intel.
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if(MSVC)
    # Static runtime: the plugin doesn't need the VC++ redistributable installed.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# JUCE: use a local checkout if given (-DJUCE_DIR=...), otherwise download it.
set(JUCE_DIR "" CACHE PATH "Path to a JUCE checkout")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Artwork and fonts compiled into the plugin (no loose files to install).
juce_add_binary_data(ToxicRigAssets SOURCES
    assets/stage.jpg
    assets/amp_british.png
    assets/amp_american.png
    assets/amp_modern.png
    assets/speaker.png
    assets/mic_dynamic.png
    assets/mic_ribbon.png
    assets/mic_condenser.png
    assets/fonts/Orbitron-ExtraBold.ttf
    assets/fonts/Rajdhani-SemiBold.ttf
    assets/fonts/Rajdhani-Bold.ttf
    assets/fonts/BarlowCondensed-Medium.ttf
    assets/fonts/Cormorant-SemiBoldItalic.ttf
    assets/fonts/JetBrainsMono-Medium.ttf
    assets/fonts/SpaceGrotesk-Bold.ttf)
set_target_properties(ToxicRigAssets PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(TZ_UI_SOURCES
    plugin/ui/Skin.cpp
    plugin/ui/Widgets.cpp
    plugin/ui/Classic.cpp
    plugin/ui/Stage.cpp)

set(TZ_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND TZ_FORMATS AU)
endif()

juce_add_plugin(ToxicRig
    COMPANY_NAME "ToxicZone"
    COMPANY_WEBSITE "https://toxiczone.it"
    BUNDLE_ID "it.toxiczone.toxicrig"
    PLUGIN_MANUFACTURER_CODE Txzn
    PLUGIN_CODE TxRg
    PRODUCT_NAME "ToxicRig"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.png"
    FORMATS ${TZ_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion
    AU_MAIN_TYPE kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "ToxicRig needs the audio input to process your guitar.")

target_sources(ToxicRig PRIVATE
    plugin/Params.cpp
    plugin/Presets.cpp
    plugin/RigEngine.cpp
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp
    ${TZ_UI_SOURCES})

target_include_directories(ToxicRig PRIVATE dsp plugin)

target_compile_definitions(ToxicRig PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(ToxicRig
    PRIVATE
        ToxicRigAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline test runner: a console app compiling the same processor sources, so the
# tests exercise exactly the code that ships in the plugin.
option(TZ_BUILD_TESTS "Build offline tests" ON)
if(TZ_BUILD_TESTS)
    juce_add_console_app(ToxicRigTests PRODUCT_NAME "ToxicRigTests")
    target_sources(ToxicRigTests PRIVATE
        tests/OfflineTests.cpp
        plugin/Params.cpp
        plugin/Presets.cpp
        plugin/RigEngine.cpp
        plugin/PluginProcessor.cpp
        plugin/PluginEditor.cpp
        ${TZ_UI_SOURCES})
    target_include_directories(ToxicRigTests PRIVATE dsp plugin)
    target_compile_definitions(ToxicRigTests PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        JucePlugin_Name="ToxicRig" JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(ToxicRigTests PRIVATE ToxicRigAssets juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
