cmake_minimum_required(VERSION 3.22)

set(PROJECT_NAME "AppInDAW")
set(VERSION 0.1.0)

set(PRODUCT_NAME "App In DAW")
set(COMPANY_NAME "Nierika")
set(BUNDLE_ID "com.nierika.appindaw")
set(MANUFACTURER_CODE "Nrka")
set(PLUGIN_CODE "AIDW")
set(FORMATS Standalone AU AUv3 VST3)

set(CMAKE_CXX_STANDARD 20)
set(JUCE_VERSION 8.0.14)
set(NIERIKA_DSP_VERSION 0.5.0)
set(AUDIO_CAPTURE_DSP_VERSION 0.2.2)
set(INCLUDE_DIR "Code/Include")
set(SOURCE_DIR "Code/Source")
set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Libs")

set(USE_LOCAL_NIERIKA_DSP ON)
set(LOCAL_NIERIKA_DSP_PATH "/Users/$ENV{USER}/Development/nierika_dsp") # Change for your local path to the nierika_dsp module if you want to use a local version instead of the remote package.

if (USE_LOCAL_NIERIKA_DSP AND NOT EXISTS "${LOCAL_NIERIKA_DSP_PATH}")
    message(STATUS "Local module 'nierika_dsp' with path '${LOCAL_NIERIKA_DSP_PATH}' not found. Falling back to remote package.")
    set(USE_LOCAL_NIERIKA_DSP OFF)
endif()

set(USE_LOCAL_AUDIO_CAPTURE_DSP ON)
set(LOCAL_AUDIO_CAPTURE_DSP_PATH "/Users/$ENV{USER}/Development/audio_capture_dsp") # Change for your local path to the audio_capture_dsp module if you want to use a local version instead of the remote package.

if (USE_LOCAL_AUDIO_CAPTURE_DSP AND NOT EXISTS "${LOCAL_AUDIO_CAPTURE_DSP_PATH}")
    message(STATUS "Local module 'audio_capture_dsp' with path '${LOCAL_AUDIO_CAPTURE_DSP_PATH}' not found. Falling back to remote package.")
    set(USE_LOCAL_AUDIO_CAPTURE_DSP OFF)
endif()

project(${PROJECT_NAME} VERSION ${VERSION})

set(MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR})

if (${VERSION} VERSION_GREATER_EQUAL "2.0.0")
    set(DISPLAY_NAME "${PRODUCT_NAME} v${MAJOR_VERSION}")
else ()
    set(DISPLAY_NAME "${PRODUCT_NAME}")
endif ()

include(${CMAKE_DIR}/cpm.cmake)

cpmaddpackage(
        NAME JUCE
        GIT_TAG ${JUCE_VERSION}
        VERSION ${JUCE_VERSION}
        GITHUB_REPOSITORY juce-framework/JUCE
        SOURCE_DIR ${LIB_DIR}/juce
)

if (USE_LOCAL_NIERIKA_DSP)
    juce_add_module("${LOCAL_NIERIKA_DSP_PATH}" ALIAS_NAMESPACE nierika)
else ()
    cpmaddpackage(
            NAME NIERIKA_DSP
            GIT_TAG "v${NIERIKA_DSP_VERSION}"
            VERSION ${NIERIKA_DSP_VERSION}
            GITHUB_REPOSITORY halbehers/nierika_dsp
            SOURCE_DIR ${LIB_DIR}/nierika_dsp
    )
    juce_add_module("${LIB_DIR}/nierika_dsp" ALIAS_NAMESPACE nierika)
endif()

if (USE_LOCAL_AUDIO_CAPTURE_DSP)
    juce_add_module("${LOCAL_AUDIO_CAPTURE_DSP_PATH}" ALIAS_NAMESPACE audiocapture)
else ()
    cpmaddpackage(
            NAME AUDIO_CAPTURE_DSP
            GIT_TAG "v${AUDIO_CAPTURE_DSP_VERSION}"
            VERSION ${AUDIO_CAPTURE_DSP_VERSION}
            GITHUB_REPOSITORY halbehers/audio_capture_dsp
            SOURCE_DIR ${LIB_DIR}/audio_capture_dsp
    )
    juce_add_module("${LIB_DIR}/audio_capture_dsp" ALIAS_NAMESPACE audiocapture)
endif()

# Test-only dependency (Tests/) - not linked into any shipping plugin target.
cpmaddpackage(
        NAME CATCH2
        GITHUB_REPOSITORY catchorg/Catch2
        GIT_TAG v3.7.1
)

include(${CMAKE_DIR}/warnings.cmake)
include(${CMAKE_DIR}/utils.cmake)

juce_add_binary_data("${PROJECT_NAME}_Assets" SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/Packaging/icon.iconset/icon_512x512@2x.png
        ${CMAKE_CURRENT_LIST_DIR}/Packaging/icon.iconset/icon_128x128.png
        ${CMAKE_CURRENT_LIST_DIR}/Assets/process_categories.json
        ${CMAKE_CURRENT_LIST_DIR}/Assets/Languages/en.lang
        ${CMAKE_CURRENT_LIST_DIR}/Assets/Languages/fr.lang
        ${CMAKE_CURRENT_LIST_DIR}/Assets/Languages/es.lang
        ${CMAKE_CURRENT_LIST_DIR}/Assets/Languages/de.lang
        ${CMAKE_CURRENT_LIST_DIR}/Assets/Languages/it.lang
        ${CMAKE_CURRENT_LIST_DIR}/Assets/Languages/pt.lang
)

juce_add_plugin(
        ${PROJECT_NAME}
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Packaging/icon.iconset/icon_512x512@2x.png"
        ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Packaging/icon.iconset/icon_128x128.png"
        COMPANY_NAME "${COMPANY_NAME}"
        BUNDLE_ID "${BUNDLE_ID}"
        PLUGIN_MANUFACTURER_CODE "${MANUFACTURER_CODE}"
        PLUGIN_CODE "${PLUGIN_CODE}"
        FORMATS ${FORMATS}
        PRODUCT_NAME ${DISPLAY_NAME}

        COPY_PLUGIN_AFTER_BUILD TRUE
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE

        # PLIST_TO_MERGE is parsed as a full plist document (see JUCE's PlistOptions::createXML -
        # a bare <key>/<string> fragment with no <plist><dict> root silently fails to parse and is
        # dropped entirely), not merely an XML fragment to splice in.
        PLIST_TO_MERGE "<plist><dict><key>NSAudioCaptureUsageDescription</key><string>Used to capture audio from another application running on this system and route it into the plugin's own output.</string></dict></plist>"
)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.mm")
file(GLOB_RECURSE HeaderFiles CONFIGURE_DEPENDS "${INCLUDE_DIR}/*.h")
target_sources(${PROJECT_NAME} PRIVATE ${SourceFiles} )
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
        nierika::nierika_dsp
        audiocapture::audio_capture_dsp
        "${PROJECT_NAME}_Assets"
)

target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)

# Treat warnings as errors
set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

enable_testing()
add_subdirectory(Tests)

include(${CMAKE_DIR}/pluginval.cmake)
include(${CMAKE_DIR}/GitHubEnv.cmake)
include(${CMAKE_DIR}/packaging.cmake)
