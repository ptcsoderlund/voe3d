// String arithmetic on a path: joining a folder and a name, finding a path's
// parent and its last name, resolving a path to an absolute one, and the
// running program's own path. Nothing here touches disk except _absolute and
// _program, which have to ask the operating system; the other three are pure
// text.
//
//     const char *scene = voe_platform_path_join(arena, project, "main.scene");
//     const char *parent = voe_platform_path_parent(arena, scene);   // project
//     const char *name = voe_platform_path_name(scene);              // "main.scene"
//
// PATHS ARE THIS FOLDER'S FOR THE SAME REASON FOLDERS AND FILES ARE
// (platform/folder.h, platform/file.h): a root and a separator are what the
// operating system says they are, and platform is the only folder allowed to
// know (ADR-0157, ADR-0162). Everywhere else in the engine treats a path as an
// opaque string handed to platform/file.h or platform/folder.h.
//
// SEPARATORS. A path reads either `/` or `\` as a separator on Windows, and
// only `/` on Linux — src/path.c decides once, behind a compile-time
// constant, which one a function that builds a new path (join, and the
// separator restored between a parent and a root) writes; nothing in this
// header or its source picks a separator with a per-function #ifdef. A
// separator this folder writes is always `\` on Windows, never `/`, so a
// path built once and joined again stays one spelling.
//
// JOINING puts exactly one separator between folder and name. When folder
// already ends in one — most often because it is a root, `/` or `C:\` — no
// second separator is added.
//
// THE PARENT of a root (`/`, or a drive root such as `C:\`) is NULL: there is
// nothing above it this folder will name. A trailing separator on path is
// ignored before the parent is found, so a path and the same path with one
// separator added past its last name share a parent. A bare name with no
// separator in it — nothing above it that this call can name — is NULL too.
//
// THE NAME is whatever follows the last separator, as a pointer into path
// itself — no copy, so it lives exactly as long as path does. A path that is
// only a root, or one ending in a separator, has nothing following its last
// separator, and the answer is "".
//
// RESOLVING. voe_platform_path_absolute turns path into an absolute one with
// symlinks and `..` resolved against the current working directory —
// `realpath` on Linux, `GetFullPathNameA` plus a check that the result exists
// on Windows, since that call alone will happily invent a path for something
// not there. NULL with VOE_BASE_ERROR_UNAVAILABLE, and the path named in a
// report through base/report.h, is nothing being there to resolve.
//
// THE PROGRAM'S OWN PATH. voe_platform_path_program is the running
// executable's absolute path, from /proc/self/exe on Linux and
// GetModuleFileNameW on Windows — not argv[0], which is a bare name found on
// PATH or relative to a working directory that may since have moved. NULL,
// reported, when the system will not say.
//
// A NULL path or arena is the caller's bug and aborts (rule 13).
#pragma once

#include <base/arena.h>
#include <base/error.h>

// Joins folder and name with exactly one separator, into arena. No extra
// separator is added when folder already ends in one.
const char *voe_platform_path_join(voe_base_arena *arena, const char *folder,
				   const char *name);

// path's parent, into arena. NULL at a root and for a bare name with no
// separator in it. A trailing separator on path is ignored.
const char *voe_platform_path_parent(voe_base_arena *arena, const char *path);

// A pointer into path at its last name — no copy. "" for a root or a path
// ending in a separator.
const char *voe_platform_path_name(const char *path);

// path resolved to an absolute one, symlinks and ".." followed, into arena.
// NULL with VOE_BASE_ERROR_UNAVAILABLE, and the real reason already reported,
// when there is nothing at path to resolve.
[[nodiscard]] const char *voe_platform_path_absolute(const char *path,
						     voe_base_arena *arena,
						     voe_base_error *error);

// The running program's absolute path, UTF-8, into arena. NULL, and the
// reason reported, when the system will not say.
[[nodiscard]] const char *voe_platform_path_program(voe_base_arena *arena);
