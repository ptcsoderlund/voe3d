// The states pushed once, the two lines swapped, the compare a settled edit
// makes against the state the world is, a settled reveal's amend of that
// state, a stroke, material, file or landscape step pushed beside a copy of
// the text, every dropped state's freed, and the step a take moves by,
// applying whichever of those it steps over.
// See the header for what a step is, what bounds it, where in the frame a take
// belongs and why the selection is re-found by authored id.
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

// Leaves the state at `index` carrying nothing, without freeing what it did.
static void voe_editor_undo_bare(voe_editor_undo_state *states, uint32_t index)
{
	VOE_BASE_ASSERT(states != NULL && index < VOE_EDITOR_UNDO_STEPS,
			"baring a state of no line or past it");
	states[index].stroke = NULL;
	states[index].material = NULL;
	states[index].file = NULL;
	states[index].landscape = NULL;
}

// Destroys the steps `states` carry from `from` up to `to`, leaving NULL:
// every state outside a line's count carries none, so a state reused starts
// bare.
static void voe_editor_undo_drop(voe_editor_undo_state *states, uint32_t from,
				 uint32_t to)
{
	VOE_BASE_ASSERT(states != NULL, "dropping steps of no line");
	VOE_BASE_ASSERT(to <= VOE_EDITOR_UNDO_STEPS, "dropping past the line");

	for (uint32_t i = from; i < to; i++) {
		voe_editor_stroke_destroy(states[i].stroke);
		voe_editor_material_step_destroy(states[i].material);
		voe_editor_file_step_destroy(states[i].file);
		voe_editor_landscape_step_destroy(states[i].landscape);
		voe_editor_undo_bare(states, i);
	}
}

// Empties the line in force, freeing its steps.
static void voe_editor_undo_empty(voe_editor_undo *undo)
{
	voe_editor_undo_drop(undo->states, 0, undo->count);
	undo->count = 0;
	undo->at = 0;
	VOE_BASE_ASSERT(undo->count == 0, "an emptied line still counts states");
}

// Pushes `text`, carrying nothing, as the state after `at`: what could have
// been redone is thrown away, because the world has gone somewhere else from
// here, and count follows. The line full means the oldest state is dropped and
// the rest shift down one, which the header's memmove is. `text` must not
// point into the line.
static void voe_editor_undo_push(voe_editor_undo *undo, voe_authoring_text text)
{
	VOE_BASE_ASSERT(undo->count > 0, "pushing onto a line with no states");

	voe_editor_undo_drop(undo->states, undo->at + 1, undo->count);
	if (undo->at + 1 == VOE_EDITOR_UNDO_STEPS) {
		voe_editor_undo_drop(undo->states, 0, 1);
		memmove(&undo->states[0], &undo->states[1],
			(VOE_EDITOR_UNDO_STEPS - 1) * sizeof(undo->states[0]));
		undo->at--;
	}
	undo->at++;
	voe_editor_undo_state_set(undo, undo->at, text);
	voe_editor_undo_bare(undo->states, undo->at);
	undo->count = undo->at + 1;
	VOE_BASE_ASSERT(undo->count <= VOE_EDITOR_UNDO_STEPS,
			"a push past the end of the line");
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
	for (uint32_t i = 0; i < VOE_EDITOR_UNDO_STEPS; i++) {
		voe_editor_undo_bare(undo->states, i);
		voe_editor_undo_bare(undo->aside_states, i);
	}
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
	voe_editor_undo_empty(undo);
	undo->edited = false;
	undo->revealed = false;
	VOE_BASE_ASSERT(undo->states != undo->aside_states,
			"a line set aside over itself");
}

void voe_editor_undo_restore(voe_editor_undo *undo)
{
	VOE_BASE_ASSERT(undo != NULL, "restoring no undo");
	VOE_BASE_ASSERT(undo->states != NULL, "restoring an uncreated undo");

	voe_editor_undo_empty(undo);
	voe_editor_undo_swap(undo);
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

	voe_editor_undo_empty(undo);
	voe_editor_undo_drop(undo->aside_states, 0, undo->aside_count);
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
		voe_editor_undo_empty(undo);
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
		voe_editor_undo_push(undo, text);
	}

	voe_base_arena_rewind(scratch, mark);
	undo->edited = false;
	undo->revealed = false;
}

// Pushes a bare state with the text at `at`, recording the scene first when
// the line is empty, and returns it for the caller to hang its step on; NULL
// when the scene will not be a state, the caller then freeing its step.
// `scratch` is rewound.
static voe_editor_undo_state *
voe_editor_undo_beside(voe_editor_undo *undo, voe_editor_project *project,
		       voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark;
	voe_authoring_text text;
	char *copy;

	VOE_BASE_ASSERT(undo != NULL && undo->states != NULL,
			"recording a step beside the text in no undo");
	VOE_BASE_ASSERT(project != NULL && scratch != NULL,
			"recording a step with no project or scratch");

	mark = voe_base_arena_mark(scratch);
	if (undo->count == 0) {
		// The line empty, the scene is its first state, as a settle's
		// first record is; one that will not be a state takes no step.
		if (!voe_editor_project_scene_text(project, scratch, &text) ||
		    text.size > VOE_EDITOR_UNDO_TEXT) {
			voe_base_arena_rewind(scratch, mark);
			return NULL;
		}
		voe_editor_undo_state_set(undo, 0, text);
		undo->count = 1;
		undo->at = 0;
	}
	// A copy, because a full line's shift moves the state it was read from.
	copy = voe_base_arena_push(scratch, undo->states[undo->at].size + 1);
	VOE_BASE_ASSERT(copy != NULL, "no room to copy a state's text");
	memcpy(copy, undo->states[undo->at].text, undo->states[undo->at].size);
	voe_editor_undo_push(undo,
			     (voe_authoring_text){
				     .text = copy,
				     .size = undo->states[undo->at].size });
	voe_base_arena_rewind(scratch, mark);
	VOE_BASE_ASSERT(undo->count == undo->at + 1,
			"a step recorded off the top of the line");
	return &undo->states[undo->at];
}

void voe_editor_undo_stroke(voe_editor_undo *undo, voe_editor_project *project,
			    voe_base_arena *scratch, voe_editor_stroke *stroke)
{
	voe_editor_undo_state *state;

	VOE_BASE_ASSERT(stroke != NULL, "recording no stroke");
	state = voe_editor_undo_beside(undo, project, scratch);
	if (state == NULL)
		voe_editor_stroke_destroy(stroke);
	else
		state->stroke = stroke;
}

void voe_editor_undo_material(voe_editor_undo *undo,
			      voe_editor_project *project,
			      voe_base_arena *scratch,
			      voe_editor_material_step *step)
{
	voe_editor_undo_state *state;

	VOE_BASE_ASSERT(step != NULL, "recording no material step");
	state = voe_editor_undo_beside(undo, project, scratch);
	if (state == NULL)
		voe_editor_material_step_destroy(step);
	else
		state->material = step;
}

void voe_editor_undo_file(voe_editor_undo *undo, voe_editor_project *project,
			  voe_base_arena *scratch, voe_editor_file_step *step)
{
	voe_editor_undo_state *state;

	VOE_BASE_ASSERT(step != NULL, "recording no file step");
	state = voe_editor_undo_beside(undo, project, scratch);
	if (state == NULL)
		voe_editor_file_step_destroy(step);
	else
		state->file = step;
}

void voe_editor_undo_landscape(voe_editor_undo *undo,
			       voe_editor_project *project,
			       voe_base_arena *scratch,
			       voe_editor_landscape_step *step)
{
	voe_editor_undo_state *state;

	VOE_BASE_ASSERT(step != NULL, "recording no landscape step");
	state = voe_editor_undo_beside(undo, project, scratch);
	if (state == NULL)
		voe_editor_landscape_step_destroy(step);
	else
		state->landscape = step;
}

// Applies the steps `carried` holds onto the world just set: their after when
// `forward`, their before otherwise. A file step refused says so in `why`.
static void voe_editor_undo_apply(const voe_editor_undo_state *carried,
				  voe_editor_project *project,
				  voe_editor_scene *scene,
				  voe_editor_models *models,
				  voe_base_arena *scratch,
				  voe_editor_disk_watch *watch,
				  voe_editor_notice *why, bool forward)
{
	VOE_BASE_ASSERT(carried != NULL && project != NULL,
			"applying the steps of no state, or to no project");
	VOE_BASE_ASSERT(why != NULL, "applying steps with nowhere to say why");
	if (carried->stroke != NULL)
		voe_editor_stroke_apply(carried->stroke, models, forward);
	if (carried->material != NULL)
		voe_editor_material_step_apply(carried->material, models, scene,
					       project->folder, scratch, watch,
					       forward);
	// A refusal leaves the world at the state stepped to: its text is
	// already read, and the file's notice in `why` is what is left to say.
	if (carried->file != NULL)
		(void)voe_editor_file_step_apply(carried->file, project, models,
						 watch, scratch, why, forward);
	if (carried->landscape != NULL)
		voe_editor_landscape_step_apply(carried->landscape, models,
						forward);
}

bool voe_editor_undo_take(voe_editor_undo *undo, voe_editor_project *project,
			  voe_editor_scene *scene, voe_editor_models *models,
			  voe_base_arena *scratch, voe_editor_disk_watch *watch,
			  voe_editor_notice *why, bool forward)
{
	const voe_scene_identity *identity;
	const voe_editor_undo_state *state;
	const voe_editor_undo_state *carried;
	voe_ecs_entity selected = { 0 };
	uint64_t id = 0;
	bool was_selected;
	uint32_t to;

	VOE_BASE_ASSERT(undo != NULL, "taking a step in no undo");
	VOE_BASE_ASSERT(project != NULL, "taking a step in no project");
	VOE_BASE_ASSERT(scene != NULL && models != NULL && scratch != NULL,
			"taking a step with no scene, store or scratch");
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

	// The steps stepped over: back from the state the world is, or forward
	// onto the next.
	carried = &undo->states[forward ? to : undo->at];
	state = &undo->states[to];
	if (!voe_editor_project_scene_set(project, state->text, state->size,
					  why)) {
		voe_editor_undo_empty(undo);
		return false;
	}
	undo->at = to;
	voe_editor_undo_apply(carried, project, scene, models, scratch, watch,
			      why, forward);

	if (was_selected)
		selected = voe_editor_undo_entity_of(project->world, id);
	voe_editor_scene_select(scene, selected);
	return true;
}
