// The states pushed once, the two lines swapped, the compare a settled edit
// makes against the state the world is, a settled reveal's amend of that
// state, and the step a take moves by. See the
// header for what a step
// is, what bounds it, where in the frame a take belongs and why the selection
// is re-found by authored id.
#include "undo.h"

#include <authoring/scene_write.h>

#include <base/assert.h>

#include <scene/identity_component.h>

#include <string.h>

// Writes `text` into the state at `index`, which is the caller's to have made
// room for.
static void voe_editor_undo_state_set(voe_editor_undo *undo, uint32_t index,
				      voe_authoring_text text)
{
	VOE_BASE_ASSERT(index < VOE_EDITOR_UNDO_STEPS,
			"writing a state past the end of the line");
	VOE_BASE_ASSERT(text.size <= VOE_EDITOR_UNDO_TEXT,
			"writing a scene text longer than a state");

	undo->states[index].size = text.size;
	memcpy(undo->states[index].text, text.text, text.size);
}

// Whether the state the world is says the same as `text`.
static bool voe_editor_undo_state_same(const voe_editor_undo *undo,
				       voe_authoring_text text)
{
	const voe_editor_undo_state *state = &undo->states[undo->at];

	VOE_BASE_ASSERT(undo->count > 0, "comparing against a line with no states");

	return state->size == text.size &&
	       memcmp(state->text, text.text, text.size) == 0;
}

// The entity carrying `id` among the world's authored rows, or a zeroed one
// when none does — which is the answer for an entity the step did not put
// back, and clears the selection.
static voe_ecs_entity voe_editor_undo_entity_of(const voe_ecs_world *world,
						uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);
	uint32_t count = voe_scene_identity_count(world);
	uint32_t i;

	VOE_BASE_ASSERT(world != NULL, "looking for an id in no world");

	for (i = 0; i < count; i++)
		if (rows[i].id == id)
			return entities[i];
	return (voe_ecs_entity){ 0 };
}

void voe_editor_undo_create(voe_editor_undo *undo, voe_base_arena *arena)
{
	VOE_BASE_ASSERT(undo != NULL, "creating no undo");
	VOE_BASE_ASSERT(arena != NULL, "creating an undo out of no arena");

	*undo = (voe_editor_undo){ 0 };
	undo->states = voe_base_arena_push(
		arena, VOE_EDITOR_UNDO_STEPS * sizeof(undo->states[0]));
	undo->aside_states = voe_base_arena_push(
		arena, VOE_EDITOR_UNDO_STEPS * sizeof(undo->states[0]));
	VOE_BASE_ASSERT(undo->states != NULL && undo->aside_states != NULL,
			"an undo created with no room for its lines");
}

// The line in force and the set-aside one change places.
static void voe_editor_undo_swap(voe_editor_undo *undo)
{
	voe_editor_undo_state *states = undo->states;
	uint32_t count = undo->count;
	uint32_t at = undo->at;

	undo->states = undo->aside_states;
	undo->count = undo->aside_count;
	undo->at = undo->aside_at;
	undo->aside_states = states;
	undo->aside_count = count;
	undo->aside_at = at;
}

void voe_editor_undo_aside(voe_editor_undo *undo)
{
	VOE_BASE_ASSERT(undo != NULL, "setting no undo aside");
	VOE_BASE_ASSERT(undo->states != NULL, "setting an uncreated undo aside");

	voe_editor_undo_swap(undo);
	undo->count = 0;
	undo->at = 0;
	undo->edited = false;
	undo->revealed = false;
	VOE_BASE_ASSERT(undo->states != undo->aside_states,
			"a line set aside over itself");
}

void voe_editor_undo_restore(voe_editor_undo *undo)
{
	VOE_BASE_ASSERT(undo != NULL, "restoring no undo");
	VOE_BASE_ASSERT(undo->states != NULL, "restoring an uncreated undo");

	voe_editor_undo_swap(undo);
	undo->aside_count = 0;
	undo->aside_at = 0;
	undo->edited = false;
	undo->revealed = false;
	VOE_BASE_ASSERT(undo->states != undo->aside_states,
			"a line restored over itself");
}

void voe_editor_undo_edited(voe_editor_undo *undo)
{
	VOE_BASE_ASSERT(undo != NULL, "marking no undo edited");
	VOE_BASE_ASSERT(undo->states != NULL, "marking an uncreated undo edited");

	undo->edited = true;
}

void voe_editor_undo_revealed(voe_editor_undo *undo)
{
	VOE_BASE_ASSERT(undo != NULL, "marking no undo revealed");
	VOE_BASE_ASSERT(undo->states != NULL,
			"marking an uncreated undo revealed");

	undo->revealed = true;
}

void voe_editor_undo_forget(voe_editor_undo *undo)
{
	VOE_BASE_ASSERT(undo != NULL, "forgetting no undo");
	VOE_BASE_ASSERT(undo->states != NULL, "forgetting an uncreated undo");

	undo->count = 0;
	undo->at = 0;
	undo->aside_count = 0;
	undo->aside_at = 0;
	undo->edited = false;
	undo->revealed = false;
}

void voe_editor_undo_settle(voe_editor_undo *undo, voe_editor_project *project,
			    voe_base_arena *scratch, bool at_rest)
{
	struct voe_base_arena_mark mark;
	voe_authoring_text text;

	VOE_BASE_ASSERT(undo != NULL, "settling no undo");
	VOE_BASE_ASSERT(project != NULL, "settling an undo on no project");
	VOE_BASE_ASSERT(scratch != NULL, "settling an undo with no scratch arena");
	VOE_BASE_ASSERT(undo->states != NULL, "settling an uncreated undo");

	if (undo->count > 0 &&
	    !((undo->edited || undo->revealed) && at_rest))
		return;

	mark = voe_base_arena_mark(scratch);
	if (!voe_editor_project_scene_text(project, scratch, &text)) {
		// A write the authoring layer refuses leaves the line as it
		// was: half a scene is not a state to go back to.
		voe_base_arena_rewind(scratch, mark);
		undo->edited = false;
		undo->revealed = false;
		return;
	}

	if (text.size > VOE_EDITOR_UNDO_TEXT) {
		undo->count = 0;
		undo->at = 0;
	} else if (undo->count == 0) {
		voe_editor_undo_state_set(undo, 0, text);
		undo->count = 1;
		undo->at = 0;
	} else if (!undo->edited) {
		// A REVEAL ALONE AMENDS THE STATE THE WORLD IS AT (0366):
		// nothing pushed, and count kept, so what could be redone
		// still can.
		voe_editor_undo_state_set(undo, undo->at, text);
	} else if (!voe_editor_undo_state_same(undo, text)) {
		// THE STATE AFTER `at` IS WHERE IT LANDS, AND count FOLLOWS IT:
		// everything that could have been redone is thrown away,
		// because the world has gone somewhere else from here. The line
		// full means the oldest state is dropped and the rest shift
		// down one, which is the header's memmove.
		if (undo->at + 1 == VOE_EDITOR_UNDO_STEPS) {
			memmove(&undo->states[0], &undo->states[1],
				(VOE_EDITOR_UNDO_STEPS - 1) *
					sizeof(undo->states[0]));
			undo->at--;
		}
		undo->at++;
		voe_editor_undo_state_set(undo, undo->at, text);
		undo->count = undo->at + 1;
	}

	voe_base_arena_rewind(scratch, mark);
	undo->edited = false;
	undo->revealed = false;
}

bool voe_editor_undo_take(voe_editor_undo *undo, voe_editor_project *project,
			  voe_editor_scene *scene, voe_editor_notice *why,
			  bool forward)
{
	const voe_scene_identity *identity;
	const voe_editor_undo_state *state;
	voe_ecs_entity selected = { 0 };
	uint64_t id = 0;
	bool was_selected;
	uint32_t to;

	VOE_BASE_ASSERT(undo != NULL, "taking a step in no undo");
	VOE_BASE_ASSERT(project != NULL, "taking a step in no project");
	VOE_BASE_ASSERT(scene != NULL, "taking a step with no scene");
	VOE_BASE_ASSERT(why != NULL, "taking a step with nowhere to say why");
	VOE_BASE_ASSERT(undo->states != NULL, "taking a step in an uncreated undo");

	if (undo->count == 0)
		return false;
	if (forward ? undo->at + 1 == undo->count : undo->at == 0)
		return false;
	to = forward ? undo->at + 1 : undo->at - 1;

	// The selection is noted as an authored id and not as an entity,
	// because no handle survives what comes next.
	identity = voe_scene_identity_get(project->world,
					  voe_editor_scene_selected(scene));
	was_selected = identity != NULL;
	if (was_selected)
		id = identity->id;

	state = &undo->states[to];
	if (!voe_editor_project_scene_set(project, state->text, state->size,
					  why)) {
		undo->count = 0;
		undo->at = 0;
		return false;
	}
	undo->at = to;

	if (was_selected)
		selected = voe_editor_undo_entity_of(project->world, id);
	voe_editor_scene_select(scene, selected);
	return true;
}
