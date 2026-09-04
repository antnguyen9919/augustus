# Pantheon: extra build targets for the Augustus fork used by Pantheon.
#
# Included (OPTIONAL) from the very end of the upstream CMakeLists.txt so every
# upstream variable (SOURCE_FILES, ASSETS_DIR, USE_FLAGS, ...) is available.
# Everything Pantheon-specific lives here or in new source directories; the
# upstream targets are untouched while the options below are OFF.

option(PANTHEON_VIEWER "Build the Pantheon viewer: full SDL game as a MODULARIZE'd ES module (Emscripten only)" OFF)

# Pantheon sources shared by the viewer and the headless engine.
set(PANTHEON_API_FILES
    ${PROJECT_SOURCE_DIR}/src/api/aug_api.c
    ${PROJECT_SOURCE_DIR}/src/api/aug_divine.c
    ${PROJECT_SOURCE_DIR}/src/api/aug_govern.c
    ${PROJECT_SOURCE_DIR}/src/api/aug_stats.c
    ${PROJECT_SOURCE_DIR}/src/pantheon/rules.c
    ${PROJECT_SOURCE_DIR}/src/pantheon/zalloc.c
)

if(PANTHEON_VIEWER)
    if(NOT ${TARGET_PLATFORM} STREQUAL "emscripten")
        message(FATAL_ERROR "PANTHEON_VIEWER requires -DTARGET_PLATFORM=emscripten")
    endif()

    set(PANTHEON_VIEWER_FILES ${SOURCE_FILES})
    list(REMOVE_ITEM PANTHEON_VIEWER_FILES ${PROJECT_SOURCE_DIR}/res/shell.html)

    add_executable(augustus-viewer ${PANTHEON_VIEWER_FILES} ${PANTHEON_API_FILES} ${PROJECT_SOURCE_DIR}/src/api/aug_view.c)
    target_compile_definitions(augustus-viewer PRIVATE PANTHEON PANTHEON_VIEWER)
    target_compile_options(augustus-viewer PRIVATE -include ${PROJECT_SOURCE_DIR}/src/pantheon/zalloc.h)

    set(PANTHEON_VIEWER_LINK_FLAGS
        "-s MODULARIZE=1"
        "-s EXPORT_ES6=1"
        "-s EXPORT_NAME=createAugustusViewer"
        "-s ENVIRONMENT=web,worker"
        "-s INVOKE_RUN=0"
        "-s INITIAL_MEMORY=268435456"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s EXPORTED_FUNCTIONS=[\"_main\",\"_malloc\",\"_free\"]"
        "-s EXPORTED_RUNTIME_METHODS=[\"callMain\",\"FS\",\"ccall\",\"cwrap\",\"HEAPU8\",\"HEAP32\",\"HEAPU16\",\"HEAPU32\",\"UTF8ToString\",\"stringToNewUTF8\",\"getValue\",\"setValue\"]"
        "-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=[\"$autoResumeAudioContext\"]"
        "--preload-file ${ASSETS_DIR}@/assets"
    )
    if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
        list(APPEND PANTHEON_VIEWER_LINK_FLAGS "-s ASSERTIONS=1")
    endif()
    string(JOIN " " PANTHEON_VIEWER_LINK_FLAGS_STR ${PANTHEON_VIEWER_LINK_FLAGS})
    set_target_properties(augustus-viewer PROPERTIES
        SUFFIX ".mjs"
        LINK_FLAGS "${PANTHEON_VIEWER_LINK_FLAGS_STR}"
    )
endif()

# ---------------------------------------------------------------------------
# Headless engine: the simulation without SDL, window, renderer, audio or input.
# Native: augustus-headless-native CLI (test harness). Emscripten: augustus-headless module.
# ---------------------------------------------------------------------------
option(PANTHEON_HEADLESS "Build the headless simulation engine" OFF)

if(PANTHEON_HEADLESS)
    set(PANTHEON_HEADLESS_PLATFORM_FILES
        ${PROJECT_SOURCE_DIR}/src/platform/crash_handler.c
        ${PROJECT_SOURCE_DIR}/src/platform/file_manager.c
        ${PROJECT_SOURCE_DIR}/src/platform/icon.c
        ${PROJECT_SOURCE_DIR}/src/platform/log.c
        ${PROJECT_SOURCE_DIR}/src/platform/prefs.c
        ${PROJECT_SOURCE_DIR}/src/platform/user_path.c
        ${PROJECT_SOURCE_DIR}/src/platform/version.c
        ${PROJECT_SOURCE_DIR}/src/platform/headless/input.c
        ${PROJECT_SOURCE_DIR}/src/platform/headless/log.c
        ${PROJECT_SOURCE_DIR}/src/platform/headless/platform.c
        ${PROJECT_SOURCE_DIR}/src/platform/headless/renderer.c
        ${PROJECT_SOURCE_DIR}/src/platform/headless/screen.c
        ${PROJECT_SOURCE_DIR}/src/platform/headless/sound_device.c
    )

    set(PANTHEON_HEADLESS_FILES
        ${PANTHEON_HEADLESS_PLATFORM_FILES}
        ${CORE_FILES}
        ${BUILDING_FILES}
        ${CITY_FILES}
        ${EMPIRE_FILES}
        ${FIGURE_FILES}
        ${FIGURETYPE_FILES}
        ${GAME_FILES}
        ${INPUT_FILES}
        ${MAP_FILES}
        ${ASSETS_FILES}
        ${SCENARIO_FILES}
        ${GRAPHICS_FILES}
        ${SOUND_FILES}
        ${WIDGET_FILES}
        ${WINDOW_FILES}
        ${EDITOR_FILES}
        ${TRANSLATION_FILES}
        ${SPNG_FILES}
        ${SXML_FILES}
        ${ZIP_FILES}
        ${PANTHEON_API_FILES}
    )

    if(${TARGET_PLATFORM} STREQUAL "emscripten")
        # Headless module. Index-only mode needs the asset XML metadata but no PNGs, so only the
        # XML files are embedded (about 700 KB instead of 16 MB).
        set(PANTHEON_HEADLESS_ASSET_DIR ${CMAKE_BINARY_DIR}/headless_assets)
        file(GLOB PANTHEON_ASSET_XMLS CONFIGURE_DEPENDS ${ASSETS_DIR}/Graphics/*.xml)
        file(MAKE_DIRECTORY ${PANTHEON_HEADLESS_ASSET_DIR}/Graphics)
        file(COPY ${PANTHEON_ASSET_XMLS} DESTINATION ${PANTHEON_HEADLESS_ASSET_DIR}/Graphics)

        add_executable(augustus-headless ${PANTHEON_HEADLESS_FILES})
        target_compile_definitions(augustus-headless PRIVATE PANTHEON PANTHEON_HEADLESS)
        target_compile_options(augustus-headless PRIVATE -include ${PROJECT_SOURCE_DIR}/src/pantheon/zalloc.h)
        set(PANTHEON_HEADLESS_LINK_FLAGS
            "-s USE_SDL=0"
            "-s USE_SDL_MIXER=0"
            "-s MODULARIZE=1"
            "-s EXPORT_ES6=1"
            "-s EXPORT_NAME=createAugustusHeadless"
            "-s ENVIRONMENT=web,worker,node"
            "-s INITIAL_MEMORY=33554432"
            "-s MAXIMUM_MEMORY=134217728"
            "-s ALLOW_MEMORY_GROWTH=1"
            "-s STACK_SIZE=1048576"
            "-s EXPORTED_FUNCTIONS=[\"_malloc\",\"_free\"]"
            "-s EXPORTED_RUNTIME_METHODS=[\"cwrap\",\"ccall\",\"FS\",\"HEAPU8\",\"HEAP32\",\"HEAPU16\",\"HEAPU32\",\"UTF8ToString\",\"stringToNewUTF8\",\"getValue\",\"setValue\"]"
            "--embed-file ${PANTHEON_HEADLESS_ASSET_DIR}@/assets"
        )
        if("${CMAKE_BUILD_TYPE}" MATCHES "Debug")
            list(APPEND PANTHEON_HEADLESS_LINK_FLAGS "-s ASSERTIONS=1")
        endif()
        string(JOIN " " PANTHEON_HEADLESS_LINK_FLAGS_STR ${PANTHEON_HEADLESS_LINK_FLAGS})
        set_target_properties(augustus-headless PROPERTIES
            SUFFIX ".mjs"
            LINK_FLAGS "${PANTHEON_HEADLESS_LINK_FLAGS_STR}"
        )
    else()
        add_executable(augustus-headless-native
            ${PANTHEON_HEADLESS_FILES}
            ${PROJECT_SOURCE_DIR}/src/platform/headless/main.c
        )
        target_compile_definitions(augustus-headless-native PRIVATE PANTHEON PANTHEON_HEADLESS)
        target_compile_options(augustus-headless-native PRIVATE -include ${PROJECT_SOURCE_DIR}/src/pantheon/zalloc.h)
        if(UNIX AND NOT APPLE)
            target_link_libraries(augustus-headless-native m)
        endif()
    endif()
endif()
