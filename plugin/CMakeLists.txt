cmake_minimum_required(VERSION 3.22)
project(PitchClassRouter VERSION 1.0.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build universal binary for macOS (Intel + Apple Silicon)
# Set deployment target BEFORE any targets are created
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
endif()

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

# Plugin target
juce_add_plugin(PitchClassRouter
    COMPANY_NAME "Scale Navigator"
    BUNDLE_ID "com.scalenavigator.pitchclassrouter"
    PLUGIN_MANUFACTURER_CODE Snav
    PLUGIN_CODE Pcrt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Pitch Class Router"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_generate_juce_header(PitchClassRouter)

# Add binary resources (fonts)
juce_add_binary_data(PitchClassRouterBinaryData
    SOURCES
        Source/InterRegular.ttf
        Source/InterBold.ttf
)

target_sources(PitchClassRouter
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(PitchClassRouter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(PitchClassRouter
    PRIVATE
        PitchClassRouterBinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(APPLE)
    # Post-build: Install AU to ~/Library/Audio/Plug-Ins/Components/
    add_custom_command(TARGET PitchClassRouter_AU POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PitchClassRouter_AU>"
            "$ENV{HOME}/Library/Audio/Plug-Ins/Components/Pitch Class Router.component"
        COMMENT "Installing AU to ~/Library/Audio/Plug-Ins/Components/"
    )

    # Post-build: Install VST3 to ~/Library/Audio/Plug-Ins/VST3/
    add_custom_command(TARGET PitchClassRouter_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:PitchClassRouter_VST3>"
            "$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Pitch Class Router.vst3"
        COMMENT "Installing VST3 to ~/Library/Audio/Plug-Ins/VST3/"
    )
endif()
