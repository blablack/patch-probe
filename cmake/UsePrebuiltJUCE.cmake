# Bootstrap JUCE using nine-strip's pre-built juceaide to avoid rebuilding it
# (juceaide depends on juce_build_tools -> juce_gui_basics which needs X11/FreeType headers)

set(NINE_STRIP_BUILD "${CMAKE_CURRENT_SOURCE_DIR}/../nine-strip/build/lib/JUCE")
set(JUCE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/lib/JUCE")

set(JUCEAIDE_BINARY
    "${NINE_STRIP_BUILD}/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide")

if(NOT EXISTS "${JUCEAIDE_BINARY}")
    message(FATAL_ERROR
        "Pre-built juceaide not found at:\n  ${JUCEAIDE_BINARY}\n"
        "Build nine-strip first, or remove this file to fall back to building juceaide from source "
        "(requires X11 and FreeType dev headers).")
endif()

if(NOT EXISTS "${JUCE_SOURCE}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE source not found at ${JUCE_SOURCE}. "
        "Run: git submodule update --init lib/JUCE  (or symlink to nine-strip's JUCE)")
endif()

# Set JUCE_BINARY_DIR so JUCE writes its intermediate files into our build tree
set(JUCE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/juce_binary" CACHE INTERNAL "")
file(MAKE_DIRECTORY "${JUCE_BINARY_DIR}")

# Register juceaide so JUCE skips trying to rebuild it
if(NOT TARGET juce::juceaide)
    add_executable(juceaide IMPORTED GLOBAL)
    set_target_properties(juceaide PROPERTIES IMPORTED_LOCATION "${JUCEAIDE_BINARY}")
    add_executable(juce::juceaide ALIAS juceaide)
endif()

# Pull in JUCE's cmake utilities directly
set(JUCE_CMAKE_UTILS_DIR "${JUCE_SOURCE}/extras/Build/CMake")
include("${JUCE_CMAKE_UTILS_DIR}/JUCEModuleSupport.cmake")
include("${JUCE_CMAKE_UTILS_DIR}/JUCEUtils.cmake")

# Register the modules we need
set(JUCE_MODULES_DIR "${JUCE_SOURCE}/modules" CACHE INTERNAL "")
foreach(_mod
        juce_core
        juce_data_structures
        juce_events
        juce_audio_basics
        juce_audio_processors_headless)
    if(NOT TARGET juce::${_mod})
        juce_add_module("${JUCE_MODULES_DIR}/${_mod}" ALIAS_NAMESPACE juce)
    endif()
endforeach()
