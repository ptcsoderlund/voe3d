// The open project's sectioned files watched on disk (0506 point 1, 0509
// point 1): what changed since the editor last knew each file, found once a
// second, held until the editor has dealt with each change.
//
//     voe_editor_disk_watch *watch = voe_editor_disk_watch_create();
//     voe_editor_disk_watch_look(watch, project->folder, now);  // each frame
//     const voe_editor_disk_change *changes =
//             voe_editor_disk_watch_changes(watch, &count);
//     ... take changes[i] in, or keep mine ...
//     voe_editor_disk_watch_settle(watch, i, taken);
//     voe_editor_disk_watch_wrote(watch, path);  // after every own write
//     voe_editor_disk_watch_destroy(watch);
//
// WHAT IS WATCHED: every file assets_walk.h walks (`.scene`, `.prefab`,
// `.material`, `.landscape`, `.parts`, any case; hidden entries and the
// top-level Build, Cache and Code skipped) and the top-level project.voe3d.
// Each known path keeps its stamp and, but for a landscape, its bytes: a file
// step's before (0509 point 2). The first look in a folder notes every file
// and reports none; a new path is noted and a gone one let go, neither a change.
//
// A BURST, ONCE A SECOND (0386, 0504): stamping every file and reading the
// changed ones scales with the project, not the view, so one task on the
// standing workers does it and the frame only polls. The look works on its own
// copy of the known stamps, so the frame may write meanwhile.
//
// A LANDSCAPE IS READ ON THE WORKER: its file is megabytes of heights, and the
// strict read (0509 point 3) on the frame would hitch. A refused one carries
// the worker's first error (0490), its subject the path.
//
// OWN WRITES ARE TOLD, NOT GUESSED (0509 point 1): by time an outside write in
// the same second would be lost, and by content a closed prefab or a landscape
// has nothing cheap to compare. `_wrote` takes the stamp at once, so nothing
// the editor writes is a change, and a path told during a look or while
// changes are held is skipped by that look and left alone by their settle.
//
// HELD, AND LOOKS PAUSED: the changes a look found are held until each is
// settled, and no look starts meanwhile, so a change waiting on Play, a splash
// or a question is not found twice. Settling takes the stamp either way, so a
// refused file is said once; a taken change's bytes become the known copy.
//
// ITS MEMORY IS ITS OWN, malloced: rule 11's exception, as strokes.h explains.
// A project's files are one size today and another tomorrow, and a change's
// bytes and landscape live from the look to the settle.
//
// PATHS: project-relative ones are as the walk makes them, `/` between. `_wrote`
// and `_known` take a full path too, the folder and one separator before it; a
// path starting at a root (`/`, `\` or a drive) elsewhere is outside.
//
// Constraints: one thread, the frame's; running out of memory is an assert. A
// folder deeper than 16 levels is not looked in, said once. A folder that will
// not list is said each look and lets nothing go. A known path is found by a
// binary search of the sorted table. A different folder, NULL included, waits
// for a look in flight and drops every change held.
#pragma once

#include <assets/landscape.h>

#include <base/arena.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// One file changed on disk: its project-relative path and the stamp seen, its
// bytes, NUL past the last, which `size` does not count. For a `.landscape`,
// the strict-read landscape in `memory`, or NULL with `error` the worker's
// first error text. `settled` once dealt with; `written` when the editor wrote
// the path since, so settling leaves its known copy alone. All its own.
typedef struct {
	char *path;
	uint64_t stamp;
	uint8_t *bytes;
	size_t size;
	voe_assets_landscape *landscape;
	voe_base_arena *memory;
	char *error;
	bool settled;
	bool written;
} voe_editor_disk_change;

typedef struct voe_editor_disk_watch voe_editor_disk_watch;

// A watch knowing nothing and looking at no folder. Never NULL.
voe_editor_disk_watch *voe_editor_disk_watch_create(void);

// Finishes a look in flight and frees the watch with all it holds. NULL is
// nothing.
void voe_editor_disk_watch_destroy(voe_editor_disk_watch *watch);

// At most once a second, with no look in flight and no change held, starts a
// look in `folder`. NULL looks at nothing; a folder unlike the last one first
// forgets everything known. `now` in seconds on one clock.
void voe_editor_disk_watch_look(voe_editor_disk_watch *watch, const char *folder,
				double now);

// The changes held, `*count` of them, settled ones among them. A finished
// look's are taken first; NULL with 0 while a look runs or none are held.
// Valid until the last is settled or the folder changes.
const voe_editor_disk_change *voe_editor_disk_watch_changes(voe_editor_disk_watch *watch,
							    uint32_t *count);

// Change `index`, not settled, has been dealt with: its stamp becomes known,
// and when `taken` its bytes the known copy, never a landscape's. Once every
// change is settled all are freed.
void voe_editor_disk_watch_settle(voe_editor_disk_watch *watch, uint32_t index,
				  bool taken);

// The editor wrote `path`: stamped now and, but for a landscape, read again as
// the known copy. A path outside the folder or of a kind not watched is
// nothing.
void voe_editor_disk_watch_wrote(voe_editor_disk_watch *watch, const char *path);

// The bytes last known for `path`, `*size` of them, NUL past the last, or NULL
// when there are none. Valid until the path is next written, settled or let go.
const uint8_t *voe_editor_disk_watch_known(voe_editor_disk_watch *watch, const char *path,
					   size_t *size);
