// Project-relative paths in scene text, followed through a rename or a move and
// looked for before a delete (ADR-0378 point 1). Text in, text out: nothing here
// opens a file, and the same calls serve a `.scene` and a `.prefab`.
//
//     follow(text, size, "Assets/Rocks", "Assets/Stone", arena, &out)
//     position = [0, 0, 0]            stays
//     model = "Assets/Rocks/a.glb"    becomes "Assets/Stone/a.glb"
//     tags = ["Assets/Rocks", "x"]    becomes ["Assets/Stone", "x"]
//
// A QUOTED STRING IS ONE AS authoring/scene_write.h WRITES IT: a value that is a
// quoted string alone after `=`, or the quoted strings inside an array. Its `\"`
// and `\\` escapes are stepped over and the string is matched unescaped. Keys,
// section names, comment lines and unquoted values are never touched.
//
// A MATCH IS WHOLE PATH SEGMENTS: the string is exactly `from`, or begins with
// `from` and then `/`. So `Assets/Rock` never matches `Assets/Rocks.glb`, and a
// folder's `from` reaches everything beneath it.
//
// IT IS A TEXT REWRITE, so kept sections and a project's own components follow
// with no knowledge here of which component holds a path.
//
// `to` HOLDS NO QUOTE OR BACKSLASH — the caller refused such a name already, so
// one here is the caller's bug and asserted. The rewrite does not know how much
// room a field has: the caller checks `longest` against it.
#pragma once

#include <authoring/scene_write.h>
#include <base/arena.h>

#include <stddef.h>

// What a follow hands back: the rewritten text, how many string values changed,
// and the longest rewritten value's length in bytes, unescaped — 0 when none
// changed.
typedef struct voe_authoring_paths_followed {
	voe_authoring_text text;
	size_t changed;
	size_t longest;
} voe_authoring_paths_followed;

// Rewrites `text` into `arena` with every matching string's `from` start
// replaced by `to`, every other byte copied as it was. On success `*out` holds
// the new text, NUL-terminated. On a string never closed before its line ends
// it returns false, reports the line, and leaves `*out` untouched; what was
// pushed is the caller's to rewind.
[[nodiscard]] bool voe_authoring_paths_follow(const char *text, size_t size,
					      const char *from, const char *to,
					      voe_base_arena *arena,
					      voe_authoring_paths_followed *out);

// Whether any quoted string value in `text` matches `path` as above. A string
// never closed ends the search; a match before it still counts.
bool voe_authoring_paths_named(const char *text, size_t size, const char *path);
