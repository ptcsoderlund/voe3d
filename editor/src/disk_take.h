// What the disk watch found changed, dealt with once a frame between frames:
// each held change taken in as one undo step, refused, or queued to ask the
// person, load from disk or keep mine (0506 point 1, 0509).
//
//     voe_editor_disk_take take = { 0 };
//     voe_editor_disk_take_step(&take, session, scene, undo, models, scratch,
//                               now);                    // once a frame
//     voe_editor_disk_take_question(&take, session, scene, undo, models,
//                                   scratch);  // the session's question kept
//
// EACH KIND BY ITS PATH, as its unsaved state is held (0509 point 2):
// - the open scene or open prefab is the scene text state: taken by
//   voe_editor_project_scene_take, the selection re-found by authored id, and
//   recorded as an edit is (voe_editor_undo_edited); asked while `unsaved`;
// - a `.landscape` the store holds is a landscape step from the held one,
//   taken unedited; asked while the store marks it edited; one the watch
//   refused is the notice; one the store does not hold is settled;
// - a `.material`, `.parts`, closed `.prefab` or project.voe3d, never unsaved
//   since the editor writes them at once, is never asked: voe_editor_file_take,
//   and a file step from the watch's known copy when it has one;
// - any other `.scene` is settled, nothing taken: only the open one is held.
// A refusal (0506 point 2) is said in the session's notice, file, line, key
// and why, and settled untaken, the last good state kept.
//
// WHAT WAITS (0509 point 4). The level's scene while a prefab is open is left
// unsettled until Back, when it is the open scene again; the watch looks for
// nothing new meanwhile. One change is asked at a time: another that would ask
// waits unsettled, the rest carry on. Play and a splash wait are the caller's:
// it does not call _step then.
//
// THE SAFETY NET under the told writes (0509 point 1): before taking, a change
// whose content says what the editor already holds, the scene's text, the
// materials table's row or the project file's values, is settled without a
// step. A change the editor wrote over since it was found is settled untaken:
// the disk holds mine.
//
// AN ANSWER. Load takes the change as a clean one is taken, one undo step so
// Ctrl+Z gives mine back, `unsaved` or `edited` false. Keep settles it
// untaken: mine stays unsaved and the next Save writes it.
//
// Constraints: one thread, the frame's; a session with no watch does nothing.
// A change whose path will not fit VOE_EDITOR_DISK_TAKE_PATH is kept, not
// asked, with a notice; a larger constant lifts it.
#pragma once

#include "models.h"
#include "scene.h"
#include "session.h"
#include "undo.h"

#include <base/arena.h>

#include <stdbool.h>

// The longest path asked about, its NUL included.
#define VOE_EDITOR_DISK_TAKE_PATH 1024

// The held change being asked, by its project-relative path, "" for none.
// Zeroed is nothing asked.
typedef struct {
	char asking[VOE_EDITOR_DISK_TAKE_PATH];
} voe_editor_disk_take;

// Once a frame, between frames: the watch asked to look in the project's
// folder, then every held, unsettled change but the one asked taken, refused,
// left waiting or queued to ask, as the header says. `scratch` is rewound;
// `now` is the frame clock's, in seconds.
void voe_editor_disk_take_step(voe_editor_disk_take *take,
			       voe_editor_session *session,
			       voe_editor_scene *scene, voe_editor_undo *undo,
			       voe_editor_models *models,
			       voe_base_arena *scratch, double now);

// The project-relative path being asked about, NULL when none.
const char *voe_editor_disk_take_asking(const voe_editor_disk_take *take);

// The question answered: `load` takes the asked change in as one undo step,
// else it is kept; settled either way and nothing asked after. Nothing when
// none is asked or the change is no longer held. `scratch` is rewound.
void voe_editor_disk_take_answer(voe_editor_disk_take *take,
				 voe_editor_session *session,
				 voe_editor_scene *scene, voe_editor_undo *undo,
				 voe_editor_models *models,
				 voe_base_arena *scratch, bool load);

// Once a frame after _step: an answer `session->disk_answer` holds handed to
// _answer and set back to NONE, then the session's question opened once on the
// path _asking names, or closed when it names none. `scratch` is rewound.
void voe_editor_disk_take_question(voe_editor_disk_take *take,
				   voe_editor_session *session,
				   voe_editor_scene *scene,
				   voe_editor_undo *undo,
				   voe_editor_models *models,
				   voe_base_arena *scratch);
