// `project.voe3d`, read and written (ADR-0164). A project is a folder holding
// this file, marking it as one, naming the one scene it holds and the window
// its game opens (ADR-0291).
//
//     [project]
//     scene = "main.scene"
//
//     [window]
//     width = 1280
//     height = 720
//     fullscreen = false
//
// THE PROJECT'S NAME IS ITS FOLDER'S, NEVER STORED HERE — a name kept in the
// file would disagree with the folder the first time someone renamed it
// outside the editor. `scene` is `[project]`'s only key: the scene file's
// path, relative to the project folder, `/`-separated on every platform, and
// never absolute or climbing out of the folder with `..`.
//
// `[window]` IS OPTIONAL, AND SO IS EACH OF ITS KEYS: `width` and `height`,
// whole decimal digits from VOE_AUTHORING_PROJECT_WINDOW_MIN to _MAX, and
// `fullscreen`, `true` or `false`. A missing key or section is its default,
// 1280, 720 and false, so a project written before the section existed still
// opens. The writer always writes the section. The game never reads this
// text (ADR-0236): the editor hands it the numbers (ADR-0291 point 3).
//
// NOTHING HERE OPENS A FILE. `platform` owns files; this reads text a caller
// already has and writes text into an arena a caller already holds, exactly
// as `authoring/scene_read.h` and `scene_write.h` do.
//
// THE READER GOES THROUGH `assets/sectioned.h` and refuses, reported through
// base/report.h with the line where one exists: a malformed file or a repeated
// section (the sectioned reader's own report), a section other than
// `[project]` or `[window]`, no `[project]` (no line to name), no `scene` key
// (named by the `[project]` line), a `scene` that is not a project-relative
// path, an unknown key in `[window]`, and a window value out of range or not
// in its form. An unknown key in `[project]` is a warning, and the file loads.
//
// THE WRITER CANNOT FAIL. It only ever writes a scene path and a window this
// reader accepts — an editor never hands it anything else — so anything the
// reader would refuse is the caller's bug, and asserts rather than reported.
#pragma once

#include <base/arena.h>

#include <stddef.h>

// The project file's fixed name, inside the project folder.
#define VOE_AUTHORING_PROJECT_FILE "project.voe3d"

// The window a missing `[window]` key stands for, and the range a width or
// height must fall in.
#define VOE_AUTHORING_PROJECT_WINDOW_DEFAULT_WIDTH 1280
#define VOE_AUTHORING_PROJECT_WINDOW_DEFAULT_HEIGHT 720
#define VOE_AUTHORING_PROJECT_WINDOW_MIN 160
#define VOE_AUTHORING_PROJECT_WINDOW_MAX 16384

// The game's window. With `fullscreen` the screen's size is used and
// `width` and `height` are only kept for going back to a window.
typedef struct {
	int width;
	int height;
	bool fullscreen;
} voe_authoring_project_window;

typedef struct {
	// The scene file's path, relative to the project folder, `/`-separated.
	// NUL-terminated, and never NULL on a successful read.
	const char *scene;
	voe_authoring_project_window window;
} voe_authoring_project;

// Reads `size` bytes of project text into `*out`. True with `out->scene` set,
// in `arena`, and `out->window` read or defaulted. False, reported, with
// `*out` untouched.
[[nodiscard]] bool voe_authoring_project_read(const char *text, size_t size,
					      voe_base_arena *arena,
					      voe_authoring_project *out);

// Writes `project` as project text into `arena`. Always succeeds: the text is
// `[project]` with `scene = "<scene>"`, `"` and `\` escaped, then a blank line
// and `[window]` with all three keys, NUL-terminated for convenience,
// `*out_size` its length without that NUL. Asserts that `project->scene` is a
// path, and the width and height are in the range, that
// voe_authoring_project_read would accept.
const char *voe_authoring_project_write(const voe_authoring_project *project,
					voe_base_arena *arena,
					size_t *out_size);
