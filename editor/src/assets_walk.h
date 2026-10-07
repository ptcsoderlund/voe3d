// The project's text files that name an asset path: walked to follow a rename
// or a move, and asked who uses a path before a Delete (ADR-0378 point 1).
//
//     if (!voe_editor_assets_follow_check(project, from, to, scratch, &why))
//             return refuse(&why);           // nothing moved, nothing written
//     move from to to on disk;
//     if (!voe_editor_assets_follow_write(project, from, to, scratch, &why))
//             say(&why);                     // the move stands; why names the file
//
// WHAT IS WALKED: every file under the project folder whose name ends in one of
// the extensions in assets_walk.c's one list (`.scene`, `.prefab`; a later own
// kind adds its own), in any case. Hidden entries are skipped, and so are the
// top-level `Build`, `Cache` and `Code` folders. C code is never walked: a bare
// name in C, such as a spawner's prefab name, is the code's to change (ADR-0377
// point 3).
//
// A FILE IS READ AS TEXT, NEVER AS A WORLD. authoring/paths.h rewrites quoted
// strings, so kept sections and a project's own components follow with no world
// made per file and no knowledge of which component holds a path.
//
// CHECK BEFORE THE MOVE, WRITE AFTER IT. The check reads and follows every file
// and writes nothing, so a path that would outgrow its field or a file that will
// not read refuses the whole rename with nothing changed. The write comes after
// the move because a moved folder's own prefabs are read where they now are.
//
// PATHS: `project` is the project folder; `from`, `to` and `path` are project-
// relative, `Assets/...`, `/` between, as a scene holds them. A followed path
// must be shorter than VOE_EDITOR_ASSETS_PATH_ROOM.
//
// CONSTRAINTS: the walk is an explicit stack of folder listings, no recursion; a
// folder deeper than assets_walk.c's depth limit (a symlink loop included) is
// skipped and said once on stderr, and a larger limit lifts it. Every file is
// read whole; scratch is rewound before each call returns.
#pragma once

#include "notice.h"

#include <base/arena.h>

#include <stdint.h>

// The room of every path field a scene holds: a model's, a prefab's, a
// texture's, a sound's.
#define VOE_EDITOR_ASSETS_PATH_ROOM 128

// How many users' names are kept; `count` goes on past it.
#define VOE_EDITOR_ASSETS_USERS_KEPT 4

// The walked files naming a path: the first few project-relative names, pushed
// into the caller's arena, and how many in all.
typedef struct {
	const char *names[VOE_EDITOR_ASSETS_USERS_KEPT];
	uint32_t count;
} voe_editor_assets_used;

// Every walked file naming path, as authoring/paths.h matches it. A folder that
// will not list ends the walk and a file that will not read is skipped, each
// already said on stderr; what was found before counts.
void voe_editor_assets_users(const char *project, const char *path,
			     voe_base_arena *arena, voe_editor_assets_used *out);

// Every walked file followed from `from` to `to` in scratch, nothing written.
// False with why naming the file whose rewritten path would not be shorter than
// the room, or that will not read or follow, or a folder that will not list.
[[nodiscard]] bool voe_editor_assets_follow_check(const char *project, const char *from,
						  const char *to, voe_base_arena *scratch,
						  voe_editor_notice *why);

// Every walked file followed, and each that changed written back. False with why
// naming the first that would not read, follow or write; those before it stay
// written.
[[nodiscard]] bool voe_editor_assets_follow_write(const char *project, const char *from,
						  const char *to, voe_base_arena *scratch,
						  voe_editor_notice *why);
