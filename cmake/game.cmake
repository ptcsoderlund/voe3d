# The game build — included by a game tree's CMakeLists.txt.
#
# A game tree is the build tree Play writes inside a project folder
# (<project>/Build/game/), with the engine compiled from its source (0235). Its
# CMakeLists.txt sets VOE_ENGINE to the engine source root and includes this
# file; its main.c calls voe_game_run(), its scene.c is the cooked world,
# defining voe_game_scene_build() (0237), and its prefabs.c the cooked prefabs,
# defining voe_game_prefabs_cooked of game/prefabs.h (0283).
#
# VOE_PROJECT_CODE, set before the include, is the project's Code/ folder
# (0242): its *.c are globbed with CONFIGURE_DEPENDS, so a file added later is
# picked up by the next build, and the folder is on the include path. Unset,
# missing or holding no .c, the code is no_code.c instead: a file written into
# CMAKE_BINARY_DIR with file(CONFIGURE), rewritten only when its bytes change,
# defining the four entry points of game/project.h: three empty, and the
# interface ending the ui frame and returning true (0259).
# The variable may be a native path (backslashes on Windows); it is rewritten
# in CMake's form first, because a CONFIGURE_DEPENDS glob is copied unescaped
# into VerifyGlobs.cmake, where a backslash is an escape (bug 03).
#
# Two modes, chosen by VOE_GAME_LIBRARY:
# - OFF (the default): the executable `game` from main.c, scene.c, prefabs.c
#   and the code, linking voe::game. VOE_BASE_DESCRIPTIONS is off because a
#   shipped game compiles the field descriptions out (ADR-0145).
# - ON: the editor's second configure of the same tree, into Build/editor/.
#   Descriptions are on, there is no `game`, and the SHARED target `project` is
#   the code alone (no prefabs.c: its spawns go to the editor's
#   voe_game_project_spawn), built into CMAKE_BINARY_DIR as libproject.so. It compiles
#   against voe::game but links no engine code; its engine symbols stay
#   undefined and bind to the editor's own when loaded. The editor builds the
#   target `project` and nothing else of the engine need be built.
#   On WIN32 it is project.dll, because an MSVC-ABI DLL binds only through an
#   import library (0245): it links VOE_EDITOR_IMPORTS, the editor's import
#   library (unset or missing is a FATAL_ERROR); it defines VOE_BASE_IMPORTING
#   so engine data is declared dllimport; and WINDOWS_EXPORT_ALL_SYMBOLS exports
#   its entry points for the editor to find.
#
# VOE_FOLDER_DATABASES is off because this tree builds the engine's folders from
# the engine's source and must not write compile_commands.json into them. Both
# are set as normal variables before voe.cmake is included, and CMP0077 (NEW
# under cmake_minimum_required 3.28) lets them win over voe.cmake's options.
#
# An empty CMAKE_BUILD_TYPE becomes Debug, so Play has debug info (0235). The
# engine's game folder builds into voe_game, not game, because `game` is the
# program's file in the binary folder.
#
# Release is what Ship builds (0264); outside WIN32 it links with -s, so the
# program carries no symbols. `cmake --install` puts in the install root the
# program, renamed VOE_GAME_NAME (default `game`, set by the tree's
# CMakeLists.txt), and its licences, voe3d-LICENSE.txt and Oxanium-OFL.txt.
# Shaders and the font are embedded; only the sounds, models and pictures are read.
#
# Sounds, models and pictures (0266, 0277, 0298): every .wav, .glb, .png and
# .jpg under the project folder,
# the tree's grandparent, is copied by the target `game_files` (which `game`
# depends on) to the same relative path under CMAKE_BINARY_DIR, and installed
# the same way. Relative, because the game resolves a file's project-relative
# path against its own program's folder, in Play and shipped alike. Build/ and
# Cache/ are skipped: they hold build output, not the project's files. The glob
# is per top-level folder, skipping those two, so the copies under Build/ never
# count as a change and a build re-runs CMake only when the project's own files
# change.
#
# The splash (0346, 0356): the project's Assets/splashscreen.png, else the
# engine's game/src/splashscreen.png, is copied by `game_files` to
# splashscreen.png in CMAKE_BINARY_DIR and installed to the root under that
# name, because the game reads it beside its program. The engine's copy is a
# configure dependency, so removing it reconfigures; with neither, a stale copy
# is removed at configure and the game shows its plain start screen.

include_guard(GLOBAL)

if(NOT VOE_ENGINE)
    message(FATAL_ERROR "game.cmake: set VOE_ENGINE to the engine source root before including it")
endif()

option(VOE_GAME_LIBRARY "Build the project's code as the library `project`" OFF)

if(VOE_GAME_LIBRARY)
    if(WIN32 AND (NOT VOE_EDITOR_IMPORTS OR NOT EXISTS "${VOE_EDITOR_IMPORTS}"))
        message(FATAL_ERROR "game.cmake: on WIN32 set VOE_EDITOR_IMPORTS to the editor's import library (got '${VOE_EDITOR_IMPORTS}')")
    endif()
    set(VOE_BASE_DESCRIPTIONS ON)
else()
    set(VOE_BASE_DESCRIPTIONS OFF)
endif()
set(VOE_FOLDER_DATABASES OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

include(${VOE_ENGINE}/cmake/voe.cmake)

add_subdirectory(${VOE_ENGINE}/game ${CMAKE_BINARY_DIR}/voe_game)

set(voe_project_sources "")
if(VOE_PROJECT_CODE)
    cmake_path(SET VOE_PROJECT_CODE NORMALIZE "${VOE_PROJECT_CODE}")
endif()
if(VOE_PROJECT_CODE AND IS_DIRECTORY "${VOE_PROJECT_CODE}")
    file(GLOB voe_project_sources CONFIGURE_DEPENDS "${VOE_PROJECT_CODE}/*.c")
endif()
if(voe_project_sources)
    set(voe_project_includes "${VOE_PROJECT_CODE}")
else()
    set(voe_project_includes "")
    set(voe_no_code "${CMAKE_BINARY_DIR}/no_code.c")
    file(CONFIGURE OUTPUT "${voe_no_code}" CONTENT [[
// Written by cmake/game.cmake: a project with no code still gets empty entry points.
#include <game/project.h>
#include <ui/layout.h>

void voe_game_project_register(voe_ecs_world *world)
{
	(void)world;
}

void voe_game_project_systems_run(const voe_game_project_step *step)
{
	(void)step;
}

void voe_game_project_systems_after_move(const voe_game_project_step *step)
{
	(void)step;
}

bool voe_game_project_interface(const voe_game_project_frame *frame)
{
	(void)voe_ui_frame_end(frame->ui);
	return true;
}
]])
    set(voe_project_sources "${voe_no_code}")
endif()

if(VOE_GAME_LIBRARY)
    add_library(project SHARED ${voe_project_sources})
    target_include_directories(project PRIVATE ${voe_project_includes})
    target_link_libraries(project PRIVATE $<COMPILE_ONLY:voe::game>)
    voe_target_settings(project)
    set_target_properties(project PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    if(WIN32)
        target_link_libraries(project PRIVATE "${VOE_EDITOR_IMPORTS}")
        target_compile_definitions(project PRIVATE VOE_BASE_IMPORTING)
        set_target_properties(project PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
else()
    add_executable(game ${CMAKE_CURRENT_SOURCE_DIR}/main.c ${CMAKE_CURRENT_SOURCE_DIR}/scene.c
        ${CMAKE_CURRENT_SOURCE_DIR}/prefabs.c ${voe_project_sources})
    target_include_directories(game PRIVATE ${voe_project_includes})
    target_link_libraries(game PRIVATE voe::game)
    voe_target_settings(game)
    if(NOT VOE_GAME_NAME)
        set(VOE_GAME_NAME game)
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT WIN32)
        target_link_options(game PRIVATE -s)
    endif()
    install(PROGRAMS $<TARGET_FILE:game> DESTINATION .
        RENAME ${VOE_GAME_NAME}${CMAKE_EXECUTABLE_SUFFIX})
    install(FILES ${VOE_ENGINE}/LICENSE DESTINATION . RENAME voe3d-LICENSE.txt)
    install(FILES ${VOE_ENGINE}/text/fonts/OFL.txt DESTINATION . RENAME Oxanium-OFL.txt)

    cmake_path(SET voe_project_root NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
    file(GLOB voe_top LIST_DIRECTORIES true CONFIGURE_DEPENDS "${voe_project_root}/*")
    set(voe_files "")
    foreach(voe_entry IN LISTS voe_top)
        cmake_path(GET voe_entry FILENAME voe_entry_name)
        if(IS_DIRECTORY "${voe_entry}")
            if(NOT voe_entry_name MATCHES "^(Build|Cache)$")
                file(GLOB_RECURSE voe_dir_files CONFIGURE_DEPENDS
                    "${voe_entry}/*.wav" "${voe_entry}/*.glb"
                    "${voe_entry}/*.png" "${voe_entry}/*.jpg")
                list(APPEND voe_files ${voe_dir_files})
            endif()
        elseif(voe_entry_name MATCHES "\\.(wav|glb|png|jpg)$")
            list(APPEND voe_files "${voe_entry}")
        endif()
    endforeach()
    set(voe_file_copies "")
    foreach(voe_file IN LISTS voe_files)
        file(RELATIVE_PATH voe_file_rel "${voe_project_root}" "${voe_file}")
        set(voe_file_copy "${CMAKE_BINARY_DIR}/${voe_file_rel}")
        add_custom_command(OUTPUT "${voe_file_copy}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${voe_file}" "${voe_file_copy}"
            DEPENDS "${voe_file}" VERBATIM)
        list(APPEND voe_file_copies "${voe_file_copy}")
        cmake_path(GET voe_file_rel PARENT_PATH voe_file_dir)
        if(NOT voe_file_dir)
            set(voe_file_dir .)
        endif()
        install(FILES "${voe_file}" DESTINATION "${voe_file_dir}")
    endforeach()
    set(voe_splash "${voe_project_root}/Assets/splashscreen.png")
    if(NOT EXISTS "${voe_splash}")
        set(voe_splash "${VOE_ENGINE}/game/src/splashscreen.png")
        if(EXISTS "${voe_splash}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${voe_splash}")
        endif()
    endif()
    if(EXISTS "${voe_splash}")
        add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/splashscreen.png"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${voe_splash}"
                "${CMAKE_BINARY_DIR}/splashscreen.png"
            DEPENDS "${voe_splash}" VERBATIM)
        list(APPEND voe_file_copies "${CMAKE_BINARY_DIR}/splashscreen.png")
        install(FILES "${voe_splash}" DESTINATION . RENAME splashscreen.png)
    else()
        file(REMOVE "${CMAKE_BINARY_DIR}/splashscreen.png")
    endif()
    add_custom_target(game_files DEPENDS ${voe_file_copies})
    add_dependencies(game game_files)
endif()
