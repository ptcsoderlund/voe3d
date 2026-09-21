// The line of scene texts a step is taken from: what Ctrl+Z goes back to and
// Ctrl+Y comes forward to (ADR-0204).
//
//     voe_editor_undo_create(&undo, arena);
//     voe_editor_undo_edited(&undo);              // an edit reached the project
//     voe_editor_undo_settle(&undo, project, scratch, at_rest);
//     if (voe_editor_undo_take(&undo, project, scene, &why, false))
//             ...                                 // the world is a step back
//
// A STEP IS THE WHOLE SCENE, AS THE TEXT A SAVE WRITES, and it is bounded by
// rest. Nothing here records what changed: an edit that reaches the project
// says so with voe_editor_undo_edited, and the world is written out and
// compared only on a later frame the caller calls at rest — the pointer's
// primary button up, nothing holding the keyboard (voe_ui_typing), the colour
// picker and the dropdown closed and the browser hidden. So one drag, one
// typed commit and one visit to the colour picker are one step each. THE COST
// OF THAT RULE: Tab from one field to the next never lets the keyboard go, so
// a row of numbers filled that way undoes as one step.
//
// A STEP IS TAKEN AT THE TOP OF A FRAME, before the world's structural queue is
// applied and the systems run, so the rows put back are given their meshes
// before anything draws them. voe_editor_project_scene_set says the same from
// the other side; this is the file that calls it.
//
// SELECTING IS NOT AN EDIT, and no entity handle survives a step: the whole
// scene is destroyed and read back. So a take notes the selected entity's
// authored voe_scene_identity id before it moves and selects whichever entity
// carries that id afterwards, clearing the selection when the scene put back
// has no such row.
//
// AN UNDO DOES NOT CLEAR THE PROJECT'S UNSAVED FLAG. Going back to the text
// that was last saved is not a save, and this file never writes project.unsaved
// either way; marking the project edited after a step is the caller's, as is
// calling voe_editor_undo_edited for the next step.
//
// THE LINE BELONGS TO THE PROJECT BEING WORKED ON. voe_editor_undo_forget
// empties it when a different project is opened or made, and the first settle
// after that records the fresh project as the line's one state.
//
// CONSTRAINTS. The line is VOE_EDITOR_UNDO_STEPS states of VOE_EDITOR_UNDO_TEXT
// bytes, pushed once out of the arena voe_editor_undo_create is handed — two
// megabytes of it — and a project's world holds at most VOE_EDITOR_SCENE_ROWS
// authored entities, which is a fraction of one state. A scene text too long
// for a state empties the line rather than recording half of it; lifting that
// means a text a state does not hold, which is a different structure and not a
// larger number. Once the line is full, dropping the oldest state shifts the
// whole array down one — one memmove of those two megabytes per step, which is
// the price of an array that a take can index straight into; a base index
// turning it into a ring would lift it.
#pragma once

#include "notice.h"
#include "project.h"
#include "scene.h"

#include <base/arena.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// How many scene texts the line holds, and how long one of them may be.
#define VOE_EDITOR_UNDO_STEPS 64
#define VOE_EDITOR_UNDO_TEXT (32u * 1024u)

// One state of the line: a scene text, `size` bytes of `text` long.
typedef struct {
	size_t size;
	char text[VOE_EDITOR_UNDO_TEXT];
} voe_editor_undo_state;

// The line and where in it the world is. Zeroed is a line with no states,
// which voe_editor_undo_create makes room for.
typedef struct {
	// VOE_EDITOR_UNDO_STEPS states, pushed once by _create.
	voe_editor_undo_state *states;
	// How many states are in the line, 0 for none recorded yet.
	uint32_t count;
	// Which of them the world is, meaningless while count is 0.
	uint32_t at;
	// Whether an edit has reached the project since the last settle.
	bool edited;
} voe_editor_undo;

// Pushes the line's states out of arena, once, and leaves it empty: the first
// settle records the project as its one state.
void voe_editor_undo_create(voe_editor_undo *undo, voe_base_arena *arena);

// Says an edit has reached the project. The step it belongs to is taken by
// whichever later settle finds the editor at rest.
void voe_editor_undo_edited(voe_editor_undo *undo);

// Empties the line and forgets any edit: a different project is being worked
// on.
void voe_editor_undo_forget(voe_editor_undo *undo);

// Records the project's scene as a state when there is none yet, or when an
// edit has reached it and `at_rest` — see the header for what rest is. A
// recorded state that differs from the one the world is throws away everything
// that could have been redone. Working memory comes out of `scratch`, which is
// rewound to where it was. Does nothing at all on any other frame.
void voe_editor_undo_settle(voe_editor_undo *undo, voe_editor_project *project,
			    voe_base_arena *scratch, bool at_rest);

// Takes a step back, or forward when `forward`: the project's world is made
// the neighbouring state and the selection re-found by its authored id. False
// and nothing done when there is no state that way, and false with `why`
// filled and the line emptied when the text was refused, which leaves the
// world half-loaded (voe_editor_project_scene_set).
[[nodiscard]] bool voe_editor_undo_take(voe_editor_undo *undo,
					voe_editor_project *project,
					voe_editor_scene *scene,
					voe_editor_notice *why, bool forward);
