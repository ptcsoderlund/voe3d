# The game build — included by a game tree's CMakeLists.txt.
#
# A game tree is the build tree Play writes inside a project folder
# (<project>/Build/game/), with the engine compiled from its source (0235). Its
# CMakeLists.txt sets VOE_ENGINE to the engine source root and includes this
# file; its main.c calls voe_game_run() and its scene.c is the cooked world,
# defining voe_game_scene_build() (0237). This file builds the executable `game`
# from those two files, linking voe::game.
#
# The game is built the way a shipped game is. VOE_BASE_DESCRIPTIONS is off
# because a shipped game compiles the field descriptions out (ADR-0145), and
# VOE_FOLDER_DATABASES is off because this tree builds the engine's folders from
# the engine's source and must not write compile_commands.json into them. Both
# are set as normal variables before voe.cmake is included, and CMP0077 (NEW
# under cmake_minimum_required 3.28) lets them win over voe.cmake's options.
#
# An empty CMAKE_BUILD_TYPE becomes Debug, so Play has debug info (0235). The
# engine's game folder builds into voe_game, not game, because `game` is the
# program's file in the binary folder.
#
# Milestone 3 adds the project's own C here, beside main.c and scene.c.

include_guard(GLOBAL)

if(NOT VOE_ENGINE)
    message(FATAL_ERROR "game.cmake: set VOE_ENGINE to the engine source root before including it")
endif()

set(VOE_BASE_DESCRIPTIONS OFF)
set(VOE_FOLDER_DATABASES OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

include(${VOE_ENGINE}/cmake/voe.cmake)

add_subdirectory(${VOE_ENGINE}/game ${CMAKE_BINARY_DIR}/voe_game)

add_executable(game ${CMAKE_CURRENT_SOURCE_DIR}/main.c ${CMAKE_CURRENT_SOURCE_DIR}/scene.c)
target_link_libraries(game PRIVATE voe::game)
voe_target_settings(game)
