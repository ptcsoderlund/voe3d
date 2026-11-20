// The held changes dealt with by kind, the safety net's compares, the selection
// re-found around a take that sets the world, and the question's one path. See
// disk_take.h for each kind's rule and why.
#include "disk_take.h"

#include "assets_panel.h"
#include "file_steps.h"

#include <assets/material.h>

#include <authoring/project.h>

#include <base/assert.h>
#include <base/report.h>

#include <scene/identity_component.h>

#include <string.h>

// What one change came to: left unsettled, queued to ask, or settled taken or
// untaken.
enum outcome { WAITS, ASKS, TAKEN, UNTAKEN };

// What dealing with changes works on, and the notice of the change at hand.
struct context {
	voe_editor_session *session;
	voe_editor_scene *scene;
	voe_editor_undo *undo;
	voe_editor_models *models;
	voe_base_arena *scratch;
	voe_editor_notice why;
};

// The selection as an authored id, since a take that sets the world leaves no
// handle alive.
struct noted {
	bool held;
	uint64_t id;
};

static struct noted selection_note(const struct context *at)
{
	const voe_scene_identity *identity;

	VOE_BASE_ASSERT(at->session->project != NULL && at->scene != NULL,
			"noting a selection in no project or scene");
	identity = voe_scene_identity_get(at->session->project->world,
					  voe_editor_scene_selected(at->scene));
	VOE_BASE_ASSERT(at->session->project->world != NULL,
			"noting a selection in no world");
	return (struct noted){ identity != NULL,
			       identity != NULL ? identity->id : 0 };
}

// Whichever entity carries the noted id selected, none when none does.
static void selection_find(const struct context *at, struct noted note)
{
	const voe_ecs_world *world = at->session->project->world;
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);
	uint32_t count = voe_scene_identity_count(world);
	voe_ecs_entity found = { 0 };

	VOE_BASE_ASSERT(world != NULL, "re-finding a selection in no world");
	for (uint32_t i = 0; note.held && i < count; i++)
		if (rows[i].id == note.id)
			found = entities[i];
	voe_editor_scene_select(at->scene, found);
	VOE_BASE_ASSERT(rows != NULL || count == 0,
			"identity entities with no rows");
}

// Whether the change's bytes are the scene text the world writes now.
static bool scene_same(const struct context *at,
		       const voe_editor_disk_change *change)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(at->scratch);
	voe_authoring_text text;
	bool same;

	VOE_BASE_ASSERT(change->bytes != NULL, "comparing a change of no bytes");
	voe_base_report_error_clear();
	same = voe_editor_project_scene_text(at->session->project, at->scratch,
					     &text) &&
	       text.size == change->size &&
	       memcmp(text.text, change->bytes, text.size) == 0;
	voe_base_arena_rewind(at->scratch, mark);
	return same;
}

// The open scene or prefab: the same text settled, asked while unsaved unless
// `load`, else taken and recorded as an edit. A line still empty records the
// world first, so the step has a before to go back to.
static enum outcome take_scene(struct context *at,
			       const voe_editor_disk_change *change, bool load)
{
	voe_editor_project *project = at->session->project;
	struct noted note;
	bool taken;

	VOE_BASE_ASSERT(project->folder != NULL, "taking a scene into no folder");
	if (scene_same(at, change))
		return TAKEN;
	if (project->unsaved && !load)
		return ASKS;
	note = selection_note(at);
	voe_editor_undo_settle(at->undo, project, at->scratch, false);
	taken = voe_editor_project_scene_take(project,
					      (const char *)change->bytes,
					      change->size, at->scratch,
					      &at->why);
	selection_find(at, note);
	if (!taken)
		return UNTAKEN;
	voe_editor_undo_edited(at->undo);
	VOE_BASE_ASSERT(!project->unsaved, "a scene taken in left unsaved");
	return TAKEN;
}

// A landscape: refused by the watch, not held, asked while edited unless
// `load`, else a landscape step recorded and the change taken unedited.
static enum outcome take_landscape(struct context *at,
				   const voe_editor_disk_change *change,
				   bool load)
{
	const voe_assets_landscape *held;

	VOE_BASE_ASSERT(at->models != NULL, "taking a landscape into no store");
	if (change->landscape == NULL) {
		if (change->error != NULL)
			voe_editor_notice_set(&at->why, "%s", change->error);
		else
			voe_editor_notice_set(&at->why, "%s: will not read",
					      change->path);
		return UNTAKEN;
	}
	held = voe_editor_models_landscape_held(at->models, change->path);
	if (held == NULL)
		return UNTAKEN;
	if (!load && voe_editor_models_landscape_edited(at->models, change->path))
		return ASKS;
	voe_editor_undo_landscape(at->undo, at->session->project, at->scratch,
				  voe_editor_landscape_step_new(change->path, held,
								change->landscape));
	voe_editor_models_landscape_take(at->models, change->path,
					 change->landscape, false);
	return TAKEN;
}

// Whether a `.material`'s strict read says what its table row holds, the two
// compared as the text each writes, since the struct has padding.
static bool material_same(const struct context *at,
			  const voe_editor_disk_change *change)
{
	const voe_game_material *row = voe_editor_materials_find(
		voe_editor_models_materials(at->models), change->path);
	struct voe_base_arena_mark mark;
	voe_assets_material_file read;
	voe_assets_material_text held;
	voe_assets_material_text found;
	bool same;

	VOE_BASE_ASSERT(change->bytes != NULL, "comparing a change of no bytes");
	if (row == NULL ||
	    !voe_assets_material_read_strict((const char *)change->bytes,
					     change->size, &read, NULL))
		return false;
	mark = voe_base_arena_mark(at->scratch);
	held = voe_assets_material_write(&row->values, at->scratch);
	found = voe_assets_material_write(&read, at->scratch);
	same = held.size == found.size &&
	       memcmp(held.text, found.text, held.size) == 0;
	voe_base_arena_rewind(at->scratch, mark);
	return same;
}

// Whether project.voe3d's strict read says the scene path and window the
// project holds.
static bool project_same(const struct context *at,
			 const voe_editor_disk_change *change)
{
	const voe_authoring_project *kept = &at->session->project->file;
	struct voe_base_arena_mark mark = voe_base_arena_mark(at->scratch);
	voe_authoring_project file;
	bool same;

	VOE_BASE_ASSERT(kept->scene != NULL, "a project file kept with no scene");
	same = voe_authoring_project_read_strict((const char *)change->bytes,
						 change->size, at->scratch,
						 &file) &&
	       strcmp(file.scene, kept->scene) == 0 &&
	       file.window.width == kept->window.width &&
	       file.window.height == kept->window.height &&
	       file.window.fullscreen == kept->window.fullscreen;
	voe_base_arena_rewind(at->scratch, mark);
	return same;
}

// A file the editor writes at once: the same values settled, else taken by
// file_steps.h, a closed prefab's world re-finding the selection, an open
// material refreshed, and a file step from the known copy when there is one.
static enum outcome take_file(struct context *at,
			      const voe_editor_disk_change *change)
{
	voe_editor_project *project = at->session->project;
	const char *path = change->path;
	bool material = voe_editor_assets_name_ends(path, ".material");
	bool prefab = voe_editor_assets_name_ends(path, ".prefab");
	struct noted note = { 0 };
	const uint8_t *known;
	size_t known_size = 0;
	bool taken;

	VOE_BASE_ASSERT(project->folder != NULL, "taking a file into no folder");
	if ((material && material_same(at, change)) ||
	    (strcmp(path, VOE_AUTHORING_PROJECT_FILE) == 0 &&
	     project_same(at, change)))
		return TAKEN;
	if (prefab)
		note = selection_note(at);
	taken = voe_editor_file_take(project, at->models, path,
				     (const char *)change->bytes, change->size,
				     at->scratch, &at->why);
	if (prefab)
		selection_find(at, note);
	if (!taken)
		return UNTAKEN;
	if (material)
		voe_editor_scene_material_refresh(
			at->scene, voe_editor_models_materials(at->models));
	known = voe_editor_disk_watch_known(at->session->watch, path,
					    &known_size);
	if (known != NULL)
		voe_editor_undo_file(at->undo, project, at->scratch,
				     voe_editor_file_step_new(path, known,
							      known_size,
							      change->bytes,
							      change->size));
	return TAKEN;
}

// One change by its path: the open scene or prefab, the level's scene waiting
// for Back, another scene, a landscape, or a file the editor writes at once.
static enum outcome take_change(struct context *at,
				const voe_editor_disk_change *change, bool load)
{
	const voe_editor_project *project = at->session->project;
	bool prefab_open = project->prefab[0] != '\0';
	const char *open = prefab_open ? project->prefab : project->file.scene;

	VOE_BASE_ASSERT(change != NULL && !change->settled,
			"taking in no change or a settled one");
	VOE_BASE_ASSERT(open != NULL, "a project with no open scene");
	if (strcmp(change->path, open) == 0)
		return take_scene(at, change, load);
	if (prefab_open && strcmp(change->path, project->file.scene) == 0)
		return WAITS;
	if (voe_editor_assets_name_ends(change->path, ".scene"))
		return UNTAKEN;
	if (voe_editor_assets_name_ends(change->path, ".landscape"))
		return take_landscape(at, change, load);
	return take_file(at, change);
}

// Change `index` settled as `outcome` says, a refusal's notice the session's.
static void settle(struct context *at, uint32_t index, enum outcome outcome)
{
	VOE_BASE_ASSERT(outcome == TAKEN || outcome == UNTAKEN,
			"settling a change that waits or asks");
	VOE_BASE_ASSERT(at->session->watch != NULL, "settling in no watch");
	if (at->why.text[0] != '\0')
		at->session->notice = at->why;
	voe_editor_disk_watch_settle(at->session->watch, index, outcome == TAKEN);
}

// The index of the unsettled change at `path` among `count`, `count` when
// none.
static uint32_t held_at(const voe_editor_disk_change *changes, uint32_t count,
			const char *path)
{
	uint32_t i;

	VOE_BASE_ASSERT(path != NULL, "finding a change at no path");
	VOE_BASE_ASSERT(changes != NULL || count == 0, "changes with no array");
	for (i = 0; i < count; i++)
		if (!changes[i].settled && strcmp(changes[i].path, path) == 0)
			break;
	return i;
}

// A change that asks becomes the question, or, its path too long to hold,
// is kept with a notice.
static enum outcome ask(voe_editor_disk_take *take, struct context *at,
			const voe_editor_disk_change *change)
{
	size_t length = strlen(change->path);

	VOE_BASE_ASSERT(take->asking[0] == '\0', "asking over a question");
	if (length >= sizeof(take->asking)) {
		voe_editor_notice_set(&at->why,
				      "%s: changed on disk, kept as it is: its "
				      "path is too long to ask about",
				      change->path);
		return UNTAKEN;
	}
	memcpy(take->asking, change->path, length + 1);
	VOE_BASE_ASSERT(take->asking[0] != '\0', "asking about no path");
	return ASKS;
}

void voe_editor_disk_take_step(voe_editor_disk_take *take,
			       voe_editor_session *session,
			       voe_editor_scene *scene, voe_editor_undo *undo,
			       voe_editor_models *models,
			       voe_base_arena *scratch, double now)
{
	struct context at = { session, scene, undo, models, scratch, { { 0 } } };
	const voe_editor_disk_change *changes;
	uint32_t count = 0;
	uint32_t left = 0;

	VOE_BASE_ASSERT(take != NULL && session != NULL &&
				session->project != NULL,
			"taking changes in with no take, session or project");
	VOE_BASE_ASSERT(scene != NULL && undo != NULL && models != NULL &&
				scratch != NULL,
			"taking changes in with no scene, undo, store or scratch");
	if (session->watch == NULL)
		return;
	voe_editor_disk_watch_look(session->watch, session->project->folder, now);
	changes = voe_editor_disk_watch_changes(session->watch, &count);
	if (held_at(changes, count, take->asking) == count)
		take->asking[0] = '\0';
	for (uint32_t i = 0; i < count; i++)
		left += changes[i].settled ? 0 : 1;
	// Settling the last unsettled change frees them all, so the walk stops
	// there.
	for (uint32_t i = 0; i < count && left > 0; i++) {
		enum outcome outcome;

		if (changes[i].settled || strcmp(changes[i].path, take->asking) == 0)
			continue;
		at.why.text[0] = '\0';
		outcome = changes[i].written ?
				  UNTAKEN :
				  take_change(&at, &changes[i], false);
		if (outcome == ASKS && take->asking[0] == '\0')
			outcome = ask(take, &at, &changes[i]);
		if (outcome != TAKEN && outcome != UNTAKEN)
			continue;
		settle(&at, i, outcome);
		left--;
	}
}

const char *voe_editor_disk_take_asking(const voe_editor_disk_take *take)
{
	VOE_BASE_ASSERT(take != NULL, "asking no take what it asks");
	VOE_BASE_ASSERT(memchr(take->asking, '\0', sizeof(take->asking)) != NULL,
			"an asked path with no end");
	return take->asking[0] != '\0' ? take->asking : NULL;
}

void voe_editor_disk_take_answer(voe_editor_disk_take *take,
				 voe_editor_session *session,
				 voe_editor_scene *scene, voe_editor_undo *undo,
				 voe_editor_models *models,
				 voe_base_arena *scratch, bool load)
{
	struct context at = { session, scene, undo, models, scratch, { { 0 } } };
	const voe_editor_disk_change *changes;
	enum outcome outcome = UNTAKEN;
	uint32_t count = 0;
	uint32_t index;

	VOE_BASE_ASSERT(take != NULL && session != NULL &&
				session->project != NULL,
			"answering with no take, session or project");
	VOE_BASE_ASSERT(scene != NULL && undo != NULL && models != NULL &&
				scratch != NULL,
			"answering with no scene, undo, store or scratch");
	if (take->asking[0] == '\0' || session->watch == NULL)
		return;
	changes = voe_editor_disk_watch_changes(session->watch, &count);
	index = held_at(changes, count, take->asking);
	take->asking[0] = '\0';
	if (index == count)
		return;
	if (load)
		outcome = take_change(&at, &changes[index], true);
	// Only an open scene or a landscape asks, and with `load` either is
	// taken or refused; anything else is kept.
	settle(&at, index, outcome == TAKEN ? TAKEN : UNTAKEN);
}
