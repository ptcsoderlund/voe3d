// What Play and Ship write and run: a project's game tree and the argument
// lists that configure it, build it, start the game and install it (ADR-0235,
// ADR-0237 point 3, ADR-0264). Nothing here starts a process; the caller
// hands these lists to platform/process.h.
//
// THE LAYOUT. `<project>/Build/game/` holds CMakeLists.txt, main.c, scene.c,
// prefabs.c, landscapes.c and materials.c; `<project>/Code/` is the project's own code (0242).
// Every configure and build step writes to `<project>/Build/build.log`.
// Everything generated lives in whole top-level folders git ignores — `Build/`
// here, `Cache/` for any other generated data — so a project's .gitignore lists
// only folders and nothing generated is ever written beside a developer's own
// files (0235). A project with no .gitignore is given one; an existing one is
// never read or changed.
//
// THREE KINDS, ONE TREE (0242 point 4). GAME builds `Build/game/` into
// `Build/debug/`, target `game`, the program `Build/debug/game`. LIBRARY
// configures the same `Build/game/` into `Build/editor/` with
// -DVOE_GAME_LIBRARY=ON and builds the target `project`, the code alone as
// `Build/editor/libproject.so`, or `project.dll` on Windows, where it links the
// editor's import library (0245). RELEASE builds `game` into `Build/release/`,
// which Ship installs into the shipped folder `Build/ship/<name>/`, `<name>`
// the project folder's name. The old shipped folder is removed only after a
// build that succeeded, so a failed Ship leaves the last one as it was (0264).
//
// main.c CARRIES THE PROJECT'S GAME WINDOW (0291 point 3): voe_game_run gets
// the name and a voe_game_window literal of width, height and fullscreen. They
// are numbers in the source, not a file the game reads, because the game never
// reads project text (0236); a changed setting changes main.c and rebuilds.
//
// CMakeLists.txt sets VOE_PROJECT_CODE to `Code/` and VOE_GAME_NAME to the
// project's name, the installed program's, before including game.cmake;
// scene.c includes every `.h` in `Code/`, sorted by name, before the cooked
// text, which names the project's own structs and keys.
//
// EACH FILE IS WRITTEN ONLY WHEN IT IS MISSING OR ITS BYTES DIFFER, because
// Ninja rebuilds by timestamp: an unchanged CMakeLists.txt or main.c rewritten
// on every Play would reconfigure or recompile for nothing, so a second Play
// rebuilds only the scene.
//
// scene.c IS THE WORLD AS IT IS NOW, unsaved edits included (ADR-0188), cooked
// by authoring/scene_cook.h; project->unsaved is left as it was.
//
// prefabs.c IS EVERY `.prefab` UNDER Assets/ AS SAVED (0283 point 9), as
// game_tree_find.h finds them: each read into a fresh project world, cooked as
// prefab_<n> after Code/'s includes and named by its path under Assets/ less
// .prefab; more than VOE_GAME_PREFAB_ENTITIES entities is refused. landscapes.c
// and materials.c ARE EVERY `.landscape` AND `.material` AS SAVED (0379, 0399
// point 7): landscape_<n> with its `Assets/…` path, size and cells; a
// material's path and values, floats in hex. A bad file is refused.
//
// THE TOOLS COME FROM THE EDITOR'S OWN BUILD (0237): toolchain.h names the
// engine source, CMake, the compiler, Ninja, pkg-config, slangc and
// wayland-scanner, and an empty value is a -D left out of the configure.
//
// Play's build is Debug, so a developer can attach any debugger to the running
// game or open Build/ in an IDE and start it there (0235); Ship's is Release.
//
// Every call takes the project's absolute folder (asserted non-NULL) and an
// arena that holds its paths, its argument lists and its working memory; what
// is pushed is the caller's to rewind.
#pragma once

#include "notice.h"
#include "project.h"

#include <base/arena.h>

#include <stdbool.h>

// Makes <folder>/Build/game/ as needed and writes its six files, then a
// .gitignore listing /Build/ and /Cache/ when the project has none. False with
// why naming the file or folder on a refused cook, folder or write; the report
// is cleared first. project->folder must be set.
[[nodiscard]] bool voe_editor_game_tree_write(const voe_editor_project *project,
					      voe_base_arena *arena,
					      voe_editor_notice *why);

// landscapes.c's text for the project at folder (game_tree_landscapes.c), in
// arena. NULL with why naming the file on one that will not find, read or
// parse.
const char *voe_editor_game_tree_landscapes_source(const char *folder,
						   voe_base_arena *arena,
						   voe_editor_notice *why);

// materials.c's text for the project at folder (game_tree_materials.c), in
// arena. NULL with why naming the file on one that will not find, read or
// parse.
const char *voe_editor_game_tree_materials_source(const char *folder,
						  voe_base_arena *arena,
						  voe_editor_notice *why);

// Formats into arena, measured first, for the game tree's own files.
[[gnu::format(printf, 2, 3)]]
const char *voe_editor_game_tree_format(voe_base_arena *arena, const char *pattern, ...);

// text escaped for inside a C string literal, in arena: `"` and `\`
// escaped, every byte outside printable ASCII as three octal digits.
const char *voe_editor_game_tree_c_string(voe_base_arena *arena, const char *text);

typedef enum {
	VOE_EDITOR_GAME_TREE_GAME,
	VOE_EDITOR_GAME_TREE_LIBRARY,
	VOE_EDITOR_GAME_TREE_RELEASE,
} voe_editor_game_tree_kind;

// Whether kind's binary folder holds CMakeCache.txt, so a configure is done.
bool voe_editor_game_tree_configured(const char *folder,
				     voe_editor_game_tree_kind kind,
				     voe_base_arena *arena);

// The NULL-terminated argument list that configures <folder>/Build/game into
// kind's binary folder with Ninja, Debug (Release for RELEASE), and each
// non-empty tool; LIBRARY
// adds -DVOE_GAME_LIBRARY=ON and, when non-empty, -DVOE_EDITOR_IMPORTS, the
// editor's import library (Windows only).
const char *const *voe_editor_game_tree_configure(const char *folder,
						  voe_editor_game_tree_kind kind,
						  voe_base_arena *arena);

// The NULL-terminated argument list that builds kind's target: project for
// LIBRARY, game otherwise.
const char *const *voe_editor_game_tree_build(const char *folder,
					      voe_editor_game_tree_kind kind,
					      voe_base_arena *arena);

// <folder>/Build/debug/game, with .exe on Windows.
const char *voe_editor_game_tree_program(const char *folder,
					 voe_base_arena *arena);

// <folder>/Build/editor/libproject.so, project.dll on Windows: what LIBRARY
// builds.
const char *voe_editor_game_tree_library(const char *folder,
					 voe_base_arena *arena);

// <folder>/Build/ship/<name>, <name> the folder's last component: where Ship
// installs the game.
const char *voe_editor_game_tree_shipped(const char *folder, voe_base_arena *arena);

// The NULL-terminated argument list that removes the shipped folder with
// CMake's `-E rm -rf`; run only after a release build succeeded.
const char *const *voe_editor_game_tree_ship_clear(const char *folder,
						   voe_base_arena *arena);

// The NULL-terminated argument list that runs `cmake --install` on
// <folder>/Build/release with the shipped folder as its prefix.
const char *const *voe_editor_game_tree_install(const char *folder,
						voe_base_arena *arena);

// <folder>/Build/build.log, where every configure and build step writes.
const char *voe_editor_game_tree_log(const char *folder, voe_base_arena *arena);

// Whether <folder>/Code/ is a folder that lists a .c file. Reports nothing.
bool voe_editor_game_tree_has_code(const char *folder, voe_base_arena *arena);
