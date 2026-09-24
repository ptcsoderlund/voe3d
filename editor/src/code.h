// A project's code as the editor holds it: the library `Build/editor/` built,
// loaded from a copy of its own, and the one entry point resolved out of it
// (ADR-0242 point 5).
//
//     voe_editor_code code;
//     if (!voe_editor_code_open(built, folder, ++loads, arena, &code, &why))
//             return ...;                    // why names the failing step
//     ...                                    // project.h registers with it
//     voe_editor_code_close(&code);          // after every world it filled
//
// A LOAD OPENS A COPY, NEVER THE BUILT FILE: `project-<load>` in
// `<folder>/Build/editor/loaded/`, `load` counting loads in this run. Windows
// locks a loaded file, so the next build could not write over it; glibc hands
// back the handle it already has for a path that is open, so a rebuilt file
// opened by the same name would be the old code.
//
// ONE LOOKUP IS THE WHOLE SEAM (ADR-0008): voe_game_project_register, cast
// back from voe_platform_symbol. The editor never runs the project's systems.
//
// A CODE IS CLOSED ONLY AFTER EVERY WORLD IT REGISTERED INTO IS DESTROYED.
// Those worlds hold its keys, descriptions and defaults, which live in the
// library; a world outliving its code reads unmapped memory.
//
// Constraints: a zeroed voe_editor_code is no code, and closing it does
// nothing. The copy is a whole read then a whole write, so the library's
// bytes pass through memory once per load.
#pragma once

#include "notice.h"

#include <base/arena.h>

#include <ecs/world.h>

#include <platform/library.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	voe_platform_library *library;
	// The project's voe_game_project_register, out of library.
	void (*register_types)(voe_ecs_world *world);
	// The loaded copy's path, in the arena voe_editor_code_open was given.
	const char *path;
} voe_editor_code;

// built copied to project-<load> under folder's Build/editor/loaded/, which
// is made as needed, then opened and its entry point resolved. False with
// why naming the step, and nothing left open.
[[nodiscard]] bool voe_editor_code_open(const char *built, const char *folder,
					uint32_t load, voe_base_arena *arena,
					voe_editor_code *out,
					voe_editor_notice *why);

// True when built's bytes equal the loaded copy's; false when they differ or
// either will not read. Both files are read into arena.
bool voe_editor_code_same(const char *built, const voe_editor_code *code,
			  voe_base_arena *arena);

// code's library closed and code zeroed; a zeroed code is left as it is.
void voe_editor_code_close(voe_editor_code *code);
