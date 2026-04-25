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

# Platform-specific plugin formats (AU is macOS-only)
if(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
    set(COPY_AFTER_BUILD TRUE)
elseif(UNIX)
    # Linux: VST3 + Standalone. COPY_PLUGIN_AFTER_BUILD installs to ~/.vst3
    # which is writable without admin.
    set(PLUGIN_FORMATS VST3 Standalone)
    set(COPY_AFTER_BUILD TRUE)
else()
    # Windows: don't auto-copy to system VST3 folder (needs admin).
    set(PLUGIN_FORMATS VST3 Standalone)
    set(COPY_AFTER_BUILD FALSE)
endif()

# Plugin target
juce_add_plugin(PitchClassRouter
    COMPANY_NAME "Scale Navigator"
    BUNDLE_ID "com.scalenavigator.pitchclassrouter"
    PLUGIN_MANUFACTURER_CODE Snav
    PLUGIN_CODE Pcrt
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Pitch Class Router"
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD}
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
        PLUGIN_VERSION="${PROJECT_VERSION}"
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
elseif(UNIX)
    # Linux VST3: bundle layout is Contents/Resources/moduleinfo.json + Contents/x86_64-linux/*.so.
    # JUCE's COPY_PLUGIN_AFTER_BUILD copies the bundle to ~/.vst3 BEFORE custom targets run,
    # so we fix moduleinfo.json in BOTH the build dir and the installed dir.
    # No WebApp / Resources to copy — this plugin is pure JUCE Components, no embedded UI.
    set(_LINUX_VST3_BUILD "${CMAKE_BINARY_DIR}/PitchClassRouter_artefacts/$<CONFIG>/VST3/Pitch Class Router.vst3")
    set(_LINUX_VST3_INSTALL "$ENV{HOME}/.vst3/Pitch Class Router.vst3")
    add_custom_target(PitchClassRouter_FixModuleInfo ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Fixing moduleinfo.json in build dir: ${_LINUX_VST3_BUILD}/Contents/Resources/moduleinfo.json"
        COMMAND python3 "${CMAKE_SOURCE_DIR}/scripts/fix-moduleinfo.py"
            "${_LINUX_VST3_BUILD}/Contents/Resources/moduleinfo.json"
        COMMAND ${CMAKE_COMMAND} -E echo "Fixing moduleinfo.json in install dir: ${_LINUX_VST3_INSTALL}/Contents/Resources/moduleinfo.json"
        COMMAND python3 "${CMAKE_SOURCE_DIR}/scripts/fix-moduleinfo.py"
            "${_LINUX_VST3_INSTALL}/Contents/Resources/moduleinfo.json"
        COMMENT "Fixing moduleinfo.json (Linux VST3, build + install)"
    )
    add_dependencies(PitchClassRouter_FixModuleInfo PitchClassRouter_VST3)
endif()
