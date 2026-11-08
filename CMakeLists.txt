# ENCORE. Basement Studio, the desktop DAW.
# Copyright (C) 2026 Encore Music, Inc. Licensed under the GNU AGPLv3 (see LICENSE).
# Built on JUCE (AGPLv3) and Tracktion Engine (GPLv3), with the ENCORE. plugins
# and audio processing. Two git submodules: encore-vst (which brings the audio
# processing, encore-audio-processing, and JUCE with it) and Tracktion Engine.
#
#   git clone https://github.com/Encore-Music-Inc/encore-basement-studio.git
#   cd encore-basement-studio
#   git submodule update --init --depth 1 external/encore-vst external/tracktion_engine
#   git -C external/encore-vst submodule update --init --depth 1 external/encore-audio-processing
#   git -C external/encore-vst/external/encore-audio-processing submodule update --init --depth 1 external/JUCE
#   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
cmake_minimum_required(VERSION 3.22)

# Mac builds run on macOS 11 and newer (set before project() so it sticks)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# The version lives here and only here. The app (About box, start screen, update
# check), the Windows installer, the Linux packages, the Mac disk image and CI
# all read it from this line.
project(EncoreBasementStudio VERSION 0.5.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# One pin decides the plugins, the audio processing and JUCE: encore-vst. The
# audio processing is the one encore-vst pins, and JUCE the one that pins (the
# JUCE version for all of ENCORE.), so Basement Studio builds exactly what the
# ENCORE. VST release was built and tested with. Tracktion Engine is the other
# pin. Fetch them by path, one level at a time, not --recursive.
set(ENCORE_VST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/encore-vst)
set(ENCORE_AUDIO_PROCESSING_DIR ${ENCORE_VST_DIR}/external/encore-audio-processing)
set(ENCORE_JUCE_DIR ${ENCORE_AUDIO_PROCESSING_DIR}/external/JUCE)
foreach(needed ${ENCORE_JUCE_DIR}/CMakeLists.txt
               ${CMAKE_CURRENT_SOURCE_DIR}/external/tracktion_engine/modules/CMakeLists.txt
               ${ENCORE_VST_DIR}/shared/EncoreProcessorBase.h
               ${ENCORE_AUDIO_PROCESSING_DIR}/dsp/MasterChain.h)
    if(NOT EXISTS "${needed}")
        message(FATAL_ERROR "${needed} is missing. Run:\n"
            "  git submodule update --init --depth 1 external/encore-vst external/tracktion_engine\n"
            "  git -C external/encore-vst submodule update --init --depth 1 external/encore-audio-processing\n"
            "  git -C external/encore-vst/external/encore-audio-processing submodule update --init --depth 1 external/JUCE\n"
            "(not --recursive)")
    endif()
endforeach()

add_subdirectory(${ENCORE_JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# Version for the Windows installer (installer/EncoreBasementStudio.iss includes it)
file(WRITE "${CMAKE_BINARY_DIR}/version.iss" "#define AppVersion \"${PROJECT_VERSION}\"\n")

# The 32-bit plugin bridge (Windows): a small 32-bit program Basement Studio runs
# 32-bit plugins in. It's built on its own, in a 32-bit build folder:
#   cmake -B build32 -G Ninja -DCMAKE_BUILD_TYPE=Release -DSTUDIO_BRIDGE_ONLY=ON
#   cmake --build build32
# and goes beside "ENCORE Basement Studio.exe" (the installer puts it there).
# File names leave out the period of "ENCORE."; what people read keeps it.
option(STUDIO_BRIDGE_ONLY "Build only the 32-bit plugin bridge" OFF)
if (STUDIO_BRIDGE_ONLY)
    juce_add_gui_app(EncoreBasementStudioBridge32
        PRODUCT_NAME "ENCORE Basement Studio Bridge32"
        COMPANY_NAME "Encore Music, Inc."
        VERSION ${PROJECT_VERSION}
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/encore-icon.png"
        NEEDS_CURL FALSE
        NEEDS_WEB_BROWSER FALSE)
    juce_generate_juce_header(EncoreBasementStudioBridge32)
    target_sources(EncoreBasementStudioBridge32 PRIVATE Source/bridge/BridgeMain.cpp Source/bridge/Shm.cpp)
    target_include_directories(EncoreBasementStudioBridge32 PRIVATE Source/bridge external/fst)
    target_compile_definitions(EncoreBasementStudioBridge32 PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(EncoreBasementStudioBridge32 PRIVATE
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_recommended_config_flags)
    return()
endif()

# ENCORE.'s few changes to Tracktion (patches/*.patch), applied to the submodule if they aren't already
file(GLOB TRACKTION_PATCHES ${CMAKE_SOURCE_DIR}/patches/tracktion-*.patch)
foreach(patch ${TRACKTION_PATCHES})
    execute_process(COMMAND git apply --reverse --check --ignore-whitespace ${patch}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/tracktion_engine RESULT_VARIABLE patch_present OUTPUT_QUIET ERROR_QUIET)
    if(NOT patch_present EQUAL 0)
        execute_process(COMMAND git apply --ignore-whitespace ${patch}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external/tracktion_engine RESULT_VARIABLE patch_failed)
        if(NOT patch_failed EQUAL 0)
            message(FATAL_ERROR "Couldn't apply ${patch} to external/tracktion_engine")
        endif()
        message(STATUS "Applied ${patch}")
    endif()
endforeach()
add_subdirectory(external/tracktion_engine/modules ${CMAKE_BINARY_DIR}/tracktion)

# Link-time optimisation. Off by default: full LTO (JUCE's juce_recommended_lto_flags)
# is not used, it costs a lot of link time. THIN is Clang's ThinLTO, or MSVC's /GL
# with incremental LTCG, the same switch as encore-vst's.
set(ENCORE_LTO "OFF" CACHE STRING "OFF or THIN")
set_property(CACHE ENCORE_LTO PROPERTY STRINGS OFF THIN)
add_library(encore_lto INTERFACE)
if(ENCORE_LTO STREQUAL "THIN")
    if(MSVC)
        target_compile_options(encore_lto INTERFACE /GL)
        target_link_options(encore_lto INTERFACE /LTCG:INCREMENTAL)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(encore_lto INTERFACE -flto=thin)
        target_link_options(encore_lto INTERFACE -flto=thin)
    else()
        message(FATAL_ERROR "ENCORE_LTO=THIN needs Clang or MSVC")
    endif()
elseif(NOT ENCORE_LTO STREQUAL "OFF")
    message(FATAL_ERROR "ENCORE_LTO must be OFF or THIN")
endif()

# The file is "ENCORE Basement Studio" (.exe, .app); the Mac menu bar and Finder
# show "ENCORE. Basement Studio" (CFBundleName and CFBundleDisplayName below)
juce_add_gui_app(EncoreBasementStudio
    PRODUCT_NAME "ENCORE Basement Studio"
    COMPANY_NAME "Encore Music, Inc."
    COMPANY_COPYRIGHT "Copyright (C) 2026 Encore Music, Inc. GNU AGPLv3."
    BUNDLE_ID com.encoremusic.studio
    VERSION ${PROJECT_VERSION}
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "ENCORE. Basement Studio records from your audio inputs."
    # .encore songs (and .basement ones from before the rename) open with a
    # double-click on a Mac too; the Windows installer and the Linux packages
    # register them themselves
    DOCUMENT_EXTENSIONS encore basement
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/encore-icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/encore-icon.png"
    PLIST_TO_MERGE "<plist><dict><key>CFBundleName</key><string>ENCORE. Basement Studio</string><key>CFBundleDisplayName</key><string>ENCORE. Basement Studio</string></dict></plist>"
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

# the ENCORE. icon, drawn in the app (SVG, so it's sharp at any size)
juce_add_binary_data(StudioAssets SOURCES Assets/encore-icon.svg)

juce_generate_juce_header(EncoreBasementStudio)

target_sources(EncoreBasementStudio PRIVATE Source/Main.cpp $<$<PLATFORM_ID:Windows>:Source/bridge/Shm.cpp>)
target_include_directories(EncoreBasementStudio PRIVATE Source)

# ENCORE.'s own plugins (Vox, Verb, Equalize, Master, Strum), built in: Basement
# Studio hosts their processors directly. Their sources come from the encore-vst
# submodule and the audio processing headers from the encore-audio-processing
# it pins (both set above), so Basement compiles exactly the code the plugins
# ship with. Each plugin's files get its product name.
foreach (entry "Vox|ENCORE. Vox" "Verb|ENCORE. Verb" "EQ|ENCORE. Equalize" "Master|ENCORE. Master" "Strum|ENCORE. Strum")
    string (REPLACE "|" ";" parts "${entry}")
    list (GET parts 0 dir)
    list (GET parts 1 product)
    file (GLOB plugin_sources CONFIGURE_DEPENDS ${ENCORE_VST_DIR}/plugins/${dir}/Source/*.cpp)
    list (APPEND plugin_sources Source/encore/Make${dir}.cpp)
    target_sources(EncoreBasementStudio PRIVATE ${plugin_sources})
    set_source_files_properties(${plugin_sources} PROPERTIES COMPILE_DEFINITIONS "JucePlugin_Name=\"${product}\"")
endforeach()
# encore-vst's shared/ for "EncoreProcessorBase.h" and "ui/...", and the
# encore-audio-processing root for "dsp/...", as in encore-vst itself
target_include_directories(EncoreBasementStudio PRIVATE ${ENCORE_VST_DIR}/shared ${ENCORE_AUDIO_PROCESSING_DIR})
# the app keeps its own default look and fonts; the plugins don't take them over
target_compile_definitions(EncoreBasementStudio PRIVATE ENCORE_PLUGIN_OWNS_DEFAULT_LOOK=0)

# older VST2 plugins: FST, a GPLv3+ stand-in for the VST2 headers
target_include_directories(EncoreBasementStudio PRIVATE external/fst)

# MP3 export: LAME's encoder library (LGPL), built from source as its own static library
file (GLOB lame_sources CONFIGURE_DEPENDS external/lame/libmp3lame/*.c)
add_library(lame STATIC ${lame_sources})
target_include_directories(lame PRIVATE external/lame external/lame/libmp3lame PUBLIC external/lame/include)
target_compile_definitions(lame PRIVATE HAVE_CONFIG_H)
if (MSVC)
    target_compile_options(lame PRIVATE /W0)
else()
    target_compile_options(lame PRIVATE -w)
endif()

target_compile_definitions(EncoreBasementStudio PRIVATE
    $<$<PLATFORM_ID:Windows>:JUCE_ASIO=1>
    BASEMENT_BUILTIN=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LADSPA=1
    $<$<PLATFORM_ID:Linux>:JUCE_PLUGINHOST_LV2=1>
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    TRACKTION_ENABLE_TIMESTRETCH_SOUNDTOUCH=1)

target_link_libraries(EncoreBasementStudio PRIVATE
    StudioAssets
    lame
    tracktion::tracktion_core
    tracktion::tracktion_engine
    tracktion::tracktion_graph
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    encore_lto)

if (MSVC)
    target_compile_options(EncoreBasementStudio PRIVATE /bigobj)
    # /MP only for Visual Studio's own builds (Ninja already compiles in parallel)
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        target_compile_options(EncoreBasementStudio PRIVATE /MP)
    endif()
endif()

# Linux (GCC): Tracktion keeps some atomics too big for one instruction, which need libatomic
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(EncoreBasementStudio PRIVATE atomic)
endif()
