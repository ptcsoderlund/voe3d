// `project.voe3d`, read and written (ADR-0164). A project is a folder holding
// this file, marking it as one, and naming the one scene it holds.
//
//     [project]
//     scene = "main.scene"
//
// THE PROJECT'S NAME IS ITS FOLDER'S, NEVER STORED HERE — a name kept in the
// file would disagree with the folder the first time someone renamed it
// outside the editor. `scene` is the only key: the scene file's path, relative
// to the project folder, `/`-separated on every platform, and never absolute
// or climbing out of the folder with `..`.
//
// NOTHING HERE OPENS A FILE. `platform` owns files; this reads text a caller
// already has and writes text into an arena a caller already holds, exactly
// as `authoring/scene_read.h` and `scene_write.h` do.
//
// THE READER GOES THROUGH `assets/sectioned.h` and refuses everything but a
// well-formed `[project]` section holding one valid `scene` key: a malformed
// file (the sectioned reader's own report), a section other than `[project]`,
// no `[project]`, no `scene` key, and a `scene` value that is not a project-
// relative path — see voe_authoring_project_read(). Every refusal is reported
// through base/report.h, with the line where one exists: a malformed file, an
// unwanted section, a missing `scene` key (named by the `[project]` line) and a
// bad scene value all sit on a line the file has, but a file with no `[project]`
// at all names none. An unknown key in `[project]` is a warning, not a refusal,
// and the file still loads.
//
// THE WRITER CANNOT FAIL. It only ever writes a scene path this reader
// accepts — an editor never hands it anything else — so a path the reader
// would refuse is the caller's bug, and asserts rather than being reported.
#pragma once

#include <base/arena.h>

#include <stddef.h>

// The project file's fixed name, inside the project folder.
#define VOE_AUTHORING_PROJECT_FILE "project.voe3d"

typedef struct {
	// The scene file's path, relative to the project folder, `/`-separated.
	// NUL-terminated, and never NULL on a successful read.
	const char *scene;
} voe_authoring_project;

// Reads `size` bytes of project text into `*out`. True with `out->scene` set,
// in `arena`. False, reported, with `*out` untouched.
[[nodiscard]] bool voe_authoring_project_read(const char *text, size_t size,
					      voe_base_arena *arena,
					      voe_authoring_project *out);

// Writes `project` as project text into `arena`. Always succeeds: the text is
// `[project]\nscene = "<scene>"\n`, `"` and `\` escaped, NUL-terminated for
// convenience, `*out_size` its length without that NUL. Asserts that
// `project->scene` is a path voe_authoring_project_read would accept.
const char *voe_authoring_project_write(const voe_authoring_project *project,
					voe_base_arena *arena,
					size_t *out_size);
