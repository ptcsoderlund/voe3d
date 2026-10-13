// The Assets panel's file commands — see assets_manage.h for the checks, the
// order a move goes in and why the undo line is forgotten.
//
// Every call clears the notice, runs the shared checks, then acts. Paths are
// built in scratch, which is rewound before each call returns.
#include "assets_manage.h"

#include "assets_panel.h"
#include "assets_walk.h"

#include <assets/landscape.h>
#include <assets/material.h>
#include <authoring/paths.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>
#include <platform/trash.h>
#include <scene/identity_component.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// The highest number a duplicate's name is given.
#define DUPLICATE_LAST 99
#define GLB_ENDING ".glb"
#define LANDSCAPE_ENDING ".landscape"
#define MATERIAL_ENDING ".material"
#define PARTS_ENDING ".parts"

// A path relative to `Assets/` cut at its last `/`: the folder ("" at
// `Assets/`), in scratch, and the name, pointing into the path.
struct split {
	const char *folder;
	const char *name;
};

static struct split split_path(voe_base_arena *scratch, const char *path)
{
	const char *slash = strrchr(path, '/');
	char *folder;

	VOE_BASE_ASSERT(scratch != NULL && path != NULL, "splitting no path");
	if (slash == NULL)
		return (struct split){ .folder = "", .name = path };
	folder = voe_base_arena_push(scratch, (size_t)(slash - path) + 1);
	VOE_BASE_ASSERT(folder != NULL, "no room to split a path");
	memcpy(folder, path, (size_t)(slash - path));
	folder[slash - path] = '\0';
	return (struct split){ .folder = folder, .name = slash + 1 };
}

// "Assets/<path>", or "Assets" for "", into scratch: the project-relative form
// a scene holds.
static const char *project_relative(voe_base_arena *scratch, const char *path)
{
	size_t size = strlen(path) + sizeof "Assets/";
	char *out = voe_base_arena_push(scratch, size);

	VOE_BASE_ASSERT(out != NULL, "no room for a project-relative path");
	snprintf(out, size, path[0] == '\0' ? "Assets%s" : "Assets/%s", path);
	VOE_BASE_ASSERT(strncmp(out, "Assets", 6) == 0, "a path outside Assets/");
	return out;
}

// `<project>/Assets/<path>` on disk, into scratch.
static const char *on_disk(const voe_editor_session *session, voe_base_arena *scratch,
			   const char *path)
{
	VOE_BASE_ASSERT(session->project->folder != NULL, "a disk path in an untitled project");
	return voe_platform_path_join(scratch, session->project->folder,
				      project_relative(scratch, path));
}

// The notice cleared, then the checks every call shares: a project with a
// folder and no prefab open. False with the refusal in the notice.
static bool command_allowed(voe_editor_session *session, voe_editor_scene *scene,
			    voe_editor_undo *undo, voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(session != NULL && session->project != NULL, "a file command with no project");
	VOE_BASE_ASSERT(scene != NULL && undo != NULL && scratch != NULL,
			"a file command with no scene, undo or scratch");
	voe_editor_notice_clear(&session->notice);
	if (session->project->folder == NULL) {
		voe_editor_notice_set(&session->notice, "Save the project first: it has no Assets folder yet");
		return false;
	}
	if (session->project->prefab[0] != '\0') {
		voe_editor_notice_set(&session->notice,
				      "Go Back to the level first: assets are not changed while a prefab is open");
		return false;
	}
	return true;
}

// A name that is not empty, does not begin with `.` and holds no `/`, `\` or
// `"`. False with the refusal in the notice.
static bool name_allowed(voe_editor_session *session, const char *name)
{
	VOE_BASE_ASSERT(session != NULL && name != NULL, "checking no name");
	if (name[0] == '\0') {
		voe_editor_notice_set(&session->notice, "A name cannot be empty");
		return false;
	}
	if (name[0] == '.') {
		voe_editor_notice_set(&session->notice, "%s: a name cannot begin with .", name);
		return false;
	}
	if (strpbrk(name, "/\\\"") != NULL) {
		voe_editor_notice_set(&session->notice, "%s: a name cannot hold /, \\ or \"", name);
		return false;
	}
	return true;
}

// `folder`'s listing into scratch. False with the notice naming it.
static bool folder_listed(voe_editor_session *session, voe_base_arena *scratch,
			  const char *folder, voe_platform_folder_listing *out)
{
	VOE_BASE_ASSERT(folder != NULL && out != NULL, "listing no folder");
	voe_base_report_error_clear();
	if (voe_platform_folder_list(on_disk(session, scratch, folder), scratch, out, NULL))
		return true;
	voe_editor_notice_from_report(&session->notice, project_relative(scratch, folder));
	return false;
}

// `path` with its name's last extension cut and `.parts` appended, into
// scratch: the parts file a `.glb` at `path` has (0420 point 1).
static const char *parts_named(voe_base_arena *scratch, const char *path)
{
	const char *slash = strrchr(path, '/');
	const char *name = slash != NULL ? slash + 1 : path;
	const char *dot = strrchr(name, '.');
	int stem = dot != NULL && dot != name ? (int)(dot - path) : (int)strlen(path);
	size_t size = (size_t)stem + sizeof PARTS_ENDING;
	char *out = voe_base_arena_push(scratch, size);

	VOE_BASE_ASSERT(out != NULL, "no room for a parts file's name");
	snprintf(out, size, "%.*s%s", stem, path, PARTS_ENDING);
	VOE_BASE_ASSERT(strlen(out) == size - 1, "a parts file's name cut short");
	return out;
}

// The `.parts` beside `path` when `path` is a file ending in `.glb`, any case,
// and that `.parts` is a file too, into scratch; else NULL.
static const char *parts_beside(const voe_editor_session *session, voe_base_arena *scratch,
				const char *path)
{
	size_t length = strlen(path);
	size_t tail = sizeof GLB_ENDING - 1;
	const char *parts;

	VOE_BASE_ASSERT(session != NULL && scratch != NULL, "looking for parts in no project");
	if (length <= tail)
		return NULL;
	for (size_t i = 0; i < tail; i++)
		if (tolower((unsigned char)path[length - tail + i]) != GLB_ENDING[i])
			return NULL;
	if (!voe_platform_file_exists(on_disk(session, scratch, path)))
		return NULL;
	parts = parts_named(scratch, path);
	return voe_platform_file_exists(on_disk(session, scratch, parts)) ? parts : NULL;
}

// The listed entry called `name`, file or folder, hidden or not, or NULL.
static const voe_platform_folder_entry *entry_named(const voe_platform_folder_listing *listing,
						    const char *name)
{
	VOE_BASE_ASSERT(listing != NULL && name != NULL, "looking in no listing");
	for (uint32_t i = 0; i < listing->count; i++)
		if (strcmp(listing->entries[i].name, name) == 0)
			return &listing->entries[i];
	return NULL;
}

// `name` free in `folder`: listed, and taken by neither a file nor a folder.
// False with the refusal in the notice.
static bool name_free(voe_editor_session *session, voe_base_arena *scratch, const char *folder,
		      const char *name)
{
	voe_platform_folder_listing listing;

	VOE_BASE_ASSERT(folder != NULL && name != NULL, "checking no name in no folder");
	if (!folder_listed(session, scratch, folder, &listing))
		return false;
	if (entry_named(&listing, name) == NULL)
		return true;
	voe_editor_notice_set(&session->notice, "%s is taken in %s", name,
			      project_relative(scratch, folder));
	return false;
}

bool voe_editor_assets_folder_make(voe_editor_session *session, voe_editor_scene *scene,
				   voe_editor_undo *undo, voe_base_arena *scratch,
				   const char *folder, const char *name)
{
	struct voe_base_arena_mark mark;
	const char *path;
	bool ok = false;

	VOE_BASE_ASSERT(folder != NULL && name != NULL, "making no folder");
	if (!command_allowed(session, scene, undo, scratch) || !name_allowed(session, name))
		return false;
	mark = voe_base_arena_mark(scratch);
	if (!name_free(session, scratch, folder, name))
		goto rewind;
	path = voe_editor_assets_join(scratch, folder, name);
	voe_base_report_error_clear();
	if (!voe_platform_folder_create(on_disk(session, scratch, path), NULL)) {
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
		goto rewind;
	}
	voe_editor_assets_list_due(&scene->assets);
	ok = true;
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

// `name` as a made file's name in scratch: `ending` (lower case) appended
// unless it already ends so, in any case.
static const char *file_named(voe_base_arena *scratch, const char *name, const char *ending)
{
	size_t length = strlen(name);
	size_t tail = strlen(ending);
	bool ends = length >= tail;
	char *out;

	VOE_BASE_ASSERT(scratch != NULL && ending != NULL, "naming a file in no scratch");
	for (size_t i = 0; ends && i < tail; i++)
		ends = tolower((unsigned char)name[length - tail + i]) == ending[i];
	out = voe_base_arena_push(scratch, length + tail + 1);
	snprintf(out, length + tail + 1, "%s%s", name, ends ? "" : ending);
	VOE_BASE_ASSERT(strlen(out) >= length, "a made file's name cut short");
	return out;
}

// The checks a made file shares: a project with a folder and an allowed name.
static bool make_allowed(voe_editor_session *session, const char *name)
{
	voe_editor_notice_clear(&session->notice);
	if (session->project->folder == NULL) {
		voe_editor_notice_set(&session->notice, "Save the project first: it has no Assets folder yet");
		return false;
	}
	return name_allowed(session, name);
}

// `<folder>/<name>` free and written with `size` bytes of `text`. False with
// the notice naming why.
static bool file_made(voe_editor_session *session, voe_base_arena *scratch, const char *folder,
		      const char *name, const char *text, size_t size)
{
	const char *path;

	VOE_BASE_ASSERT(text != NULL && size > 0, "making a file of no text");
	if (!name_free(session, scratch, folder, name))
		return false;
	path = voe_editor_assets_join(scratch, folder, name);
	voe_base_report_error_clear();
	if (voe_platform_file_write(on_disk(session, scratch, path), (const uint8_t *)text, size, NULL))
		return true;
	voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
	return false;
}

bool voe_editor_assets_landscape_make(voe_editor_session *session, voe_base_arena *scratch,
				      const char *folder, const char *name)
{
	struct voe_base_arena_mark mark;
	voe_assets_landscape flat;
	voe_assets_landscape_text text;
	bool ok;

	VOE_BASE_ASSERT(session != NULL && session->project != NULL, "a landscape with no project");
	VOE_BASE_ASSERT(scratch != NULL && folder != NULL && name != NULL, "making no landscape");
	if (!make_allowed(session, name))
		return false;
	mark = voe_base_arena_mark(scratch);
	flat = voe_assets_landscape_flat(VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT, VOE_ASSETS_LANDSCAPE_CELLS,
					 scratch);
	text = voe_assets_landscape_write(&flat, scratch);
	ok = file_made(session, scratch, folder, file_named(scratch, name, LANDSCAPE_ENDING),
		       text.text, text.size);
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

// The materials' and parts' tables read again, then the open material
// refreshed from its row (scene.h).
static void materials_read(voe_editor_session *session, voe_editor_scene *scene,
			   voe_editor_models *models, voe_base_arena *scratch)
{
	VOE_BASE_ASSERT(session != NULL && scene != NULL, "reading materials for no scene");
	VOE_BASE_ASSERT(models != NULL && scratch != NULL, "reading materials into no store");
	voe_editor_models_materials_read(models, session->project->folder, scratch);
	voe_editor_scene_material_refresh(scene, voe_editor_models_materials(models));
}

bool voe_editor_assets_material_make(voe_editor_session *session, voe_editor_scene *scene,
				     voe_editor_models *models, voe_base_arena *scratch,
				     const char *folder, const char *name)
{
	struct voe_base_arena_mark mark;
	voe_assets_material_file defaults = voe_assets_material_default();
	voe_assets_material_text text;
	bool ok;

	VOE_BASE_ASSERT(session != NULL && session->project != NULL, "a material with no project");
	VOE_BASE_ASSERT(scene != NULL && models != NULL && scratch != NULL && folder != NULL &&
				name != NULL,
			"making no material");
	if (!make_allowed(session, name))
		return false;
	mark = voe_base_arena_mark(scratch);
	text = voe_assets_material_write(&defaults, scratch);
	ok = file_made(session, scratch, folder, file_named(scratch, name, MATERIAL_ENDING),
		       text.text, text.size);
	voe_base_arena_rewind(scratch, mark);
	if (ok)
		materials_read(session, scene, models, scratch);
	return ok;
}

// The entity carrying authored `id`, or a zeroed one when none does.
static voe_ecs_entity entity_of(const voe_ecs_world *world, uint64_t id)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(world);
	const voe_ecs_entity *entities = voe_scene_identity_entities(world);
	uint32_t count = voe_scene_identity_count(world);

	VOE_BASE_ASSERT(world != NULL, "looking for an id in no world");
	for (uint32_t i = 0; i < count; i++)
		if (rows[i].id == id)
			return entities[i];
	return (voe_ecs_entity){ 0 };
}

// The open scene's text followed from `from` to `to` in scratch, nothing set.
// False with the notice saying why: a text that will not write or follow, or a
// followed path too long for its field.
static bool scene_followed(voe_editor_session *session, voe_base_arena *scratch, const char *from,
			   const char *to, voe_authoring_paths_followed *out)
{
	voe_authoring_text text;

	VOE_BASE_ASSERT(from != NULL && to != NULL && out != NULL, "following the scene into nothing");
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(session->project, scratch, &text) ||
	    !voe_authoring_paths_follow(text.text, text.size, from, to, scratch, out)) {
		voe_editor_notice_from_report(&session->notice, "the open scene");
		return false;
	}
	if (out->longest < VOE_EDITOR_ASSETS_PATH_ROOM)
		return true;
	voe_editor_notice_set(&session->notice,
			      "the open scene: a followed path would be %zu bytes, %d or more",
			      out->longest, VOE_EDITOR_ASSETS_PATH_ROOM);
	return false;
}

// The open scene made to hold its followed text, the selection re-found by
// authored id; scene_set leaves the unsaved flag as it was. Nothing to read
// back when nothing changed. False with why.
static bool scene_set_followed(voe_editor_session *session, voe_editor_scene *scene,
			       const voe_authoring_paths_followed *followed, voe_editor_notice *why)
{
	const voe_scene_identity *identity;
	bool unsaved = session->project->unsaved;
	uint64_t id = 0;

	VOE_BASE_ASSERT(followed != NULL && why != NULL, "setting no followed scene");
	if (followed->changed == 0)
		return true;
	identity = voe_scene_identity_get(session->project->world, voe_editor_scene_selected(scene));
	if (identity != NULL)
		id = identity->id;
	if (!voe_editor_project_scene_set(session->project, followed->text.text,
					  followed->text.size, why))
		return false;
	voe_editor_scene_select(scene, identity != NULL ? entity_of(session->project->world, id)
						       : (voe_ecs_entity){ 0 });
	VOE_BASE_ASSERT(session->project->unsaved == unsaved, "a follow changed the unsaved flag");
	return true;
}

// Every check a move makes before it moves: the name, the target folder's
// listing, a folder into itself, the `.parts`'s new name (`parts_to`, NULL for
// none), and every followed path on disk and in the open scene, which is left
// followed in `followed`.
static bool move_allowed(voe_editor_session *session, voe_base_arena *scratch, const char *from,
			 const char *to, const char *parts_to,
			 voe_authoring_paths_followed *followed)
{
	struct split target = split_path(scratch, to);
	size_t length = strlen(from);
	const char *project_from = project_relative(scratch, from);
	const char *project_to = project_relative(scratch, to);

	VOE_BASE_ASSERT(from[0] != '\0', "moving Assets/ itself");
	if (!name_allowed(session, target.name))
		return false;
	if (strncmp(to, from, length) == 0 && to[length] == '/') {
		voe_editor_notice_set(&session->notice, "%s cannot move into itself", project_from);
		return false;
	}
	if (!name_free(session, scratch, target.folder, target.name))
		return false;
	if (parts_to != NULL) {
		struct split parts = split_path(scratch, parts_to);

		if (!name_free(session, scratch, parts.folder, parts.name))
			return false;
	}
	return voe_editor_assets_follow_check(session->project->folder, project_from, project_to,
					      scratch, &session->notice) &&
	       scene_followed(session, scratch, project_from, project_to, followed);
}

bool voe_editor_assets_move(voe_editor_session *session, voe_editor_scene *scene,
			    voe_editor_undo *undo, voe_editor_models *models,
			    voe_base_arena *scratch, const char *from, const char *to)
{
	struct voe_base_arena_mark mark;
	voe_authoring_paths_followed followed;
	voe_editor_notice why = { 0 };
	const char *parts_from;
	const char *parts_to = NULL;
	bool ok = false;

	VOE_BASE_ASSERT(from != NULL && to != NULL, "moving no path");
	VOE_BASE_ASSERT(models != NULL, "moving with no model store to follow");
	if (!command_allowed(session, scene, undo, scratch))
		return false;
	if (strcmp(from, to) == 0)
		return true;
	mark = voe_base_arena_mark(scratch);
	parts_from = parts_beside(session, scratch, from);
	if (parts_from != NULL)
		parts_to = parts_named(scratch, to);
	// A rename of the `.glb`'s ending alone leaves its `.parts` where it is.
	if (parts_to != NULL && strcmp(parts_from, parts_to) == 0)
		parts_from = parts_to = NULL;
	if (!move_allowed(session, scratch, from, to, parts_to, &followed))
		goto rewind;
	voe_base_report_error_clear();
	if (!voe_platform_file_move(on_disk(session, scratch, from), on_disk(session, scratch, to),
				    NULL)) {
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, from));
		goto rewind;
	}
	voe_editor_models_rename(models, project_relative(scratch, from),
				 project_relative(scratch, to));
	// Before the read, which would close it at its old path.
	voe_editor_scene_material_follow(scene, from, to);
	ok = voe_editor_assets_follow_write(session->project->folder,
					    project_relative(scratch, from),
					    project_relative(scratch, to), scratch, &session->notice);
	// The first failure is the one said: a write's, else the scene's.
	if (!scene_set_followed(session, scene, &followed, &why)) {
		if (ok)
			session->notice = why;
		ok = false;
	}
	// The `.parts` last, its failure said only when nothing before failed.
	voe_base_report_error_clear();
	if (parts_from != NULL && !voe_platform_file_move(on_disk(session, scratch, parts_from),
							  on_disk(session, scratch, parts_to), NULL)) {
		if (ok)
			voe_editor_notice_from_report(&session->notice,
						      project_relative(scratch, parts_from));
		ok = false;
	}
	voe_editor_undo_forget(undo);
	voe_editor_assets_list_due(&scene->assets);
	materials_read(session, scene, models, scratch);
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

// `<stem> <n><ext>` for the first n from 2 not taken in listing, into scratch,
// or NULL past DUPLICATE_LAST.
static const char *duplicate_name(voe_base_arena *scratch,
				  const voe_platform_folder_listing *listing, const char *name)
{
	const char *dot = strrchr(name, '.');
	int stem = dot != NULL && dot != name ? (int)(dot - name) : (int)strlen(name);
	size_t size = strlen(name) + sizeof " 99";
	char *out = voe_base_arena_push(scratch, size);

	VOE_BASE_ASSERT(out != NULL, "no room for a duplicate's name");
	for (int n = 2; n <= DUPLICATE_LAST; n++) {
		snprintf(out, size, "%.*s %d%s", stem, name, n, name + stem);
		if (entry_named(listing, out) == NULL)
			return out;
	}
	return NULL;
}

// `path` read whole into scratch. NULL with the notice naming it, an empty
// file included: a write refuses no bytes (platform/file.h).
static const uint8_t *copy_read(voe_editor_session *session, voe_base_arena *scratch,
				const char *path, size_t *count)
{
	const uint8_t *bytes;

	VOE_BASE_ASSERT(path != NULL && count != NULL, "reading no copy");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(on_disk(session, scratch, path), scratch, count, NULL);
	if (bytes == NULL) {
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
		return NULL;
	}
	if (*count == 0) {
		voe_editor_notice_set(&session->notice, "%s is empty", project_relative(scratch, path));
		return NULL;
	}
	return bytes;
}

// `count` bytes written to `path`. False with the notice naming it.
static bool copy_written(voe_editor_session *session, voe_base_arena *scratch, const char *path,
			 const uint8_t *bytes, size_t count)
{
	VOE_BASE_ASSERT(path != NULL && bytes != NULL && count > 0, "writing no copy");
	voe_base_report_error_clear();
	if (voe_platform_file_write(on_disk(session, scratch, path), bytes, count, NULL))
		return true;
	voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
	return false;
}

bool voe_editor_assets_duplicate(voe_editor_session *session, voe_editor_scene *scene,
				 voe_editor_undo *undo, voe_editor_models *models,
				 voe_base_arena *scratch, const char *path)
{
	struct voe_base_arena_mark mark;
	voe_platform_folder_listing listing;
	const voe_platform_folder_entry *entry;
	struct split source;
	const char *copy;
	const char *parts;
	const char *parts_copy = NULL;
	const uint8_t *bytes;
	const uint8_t *parts_bytes = NULL;
	size_t count = 0;
	size_t parts_count = 0;
	bool ok = false;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "duplicating no path");
	VOE_BASE_ASSERT(models != NULL, "duplicating with no materials to read");
	if (!command_allowed(session, scene, undo, scratch))
		return false;
	mark = voe_base_arena_mark(scratch);
	source = split_path(scratch, path);
	if (!folder_listed(session, scratch, source.folder, &listing))
		goto rewind;
	entry = entry_named(&listing, source.name);
	if (entry == NULL || entry->folder) {
		voe_editor_notice_set(&session->notice, entry == NULL ? "%s is not there"
								      : "%s is a folder; only a file is duplicated",
				      project_relative(scratch, path));
		goto rewind;
	}
	copy = duplicate_name(scratch, &listing, source.name);
	if (copy == NULL) {
		voe_editor_notice_set(&session->notice, "%s has no free copy name up to %d",
				      project_relative(scratch, path), DUPLICATE_LAST);
		goto rewind;
	}
	copy = voe_editor_assets_join(scratch, source.folder, copy);
	parts = parts_beside(session, scratch, path);
	if (parts != NULL) {
		parts_copy = parts_named(scratch, copy);
		if (entry_named(&listing, split_path(scratch, parts_copy).name) != NULL) {
			voe_editor_notice_set(&session->notice, "%s is taken",
					      project_relative(scratch, parts_copy));
			goto rewind;
		}
		parts_bytes = copy_read(session, scratch, parts, &parts_count);
		if (parts_bytes == NULL)
			goto rewind;
	}
	bytes = copy_read(session, scratch, path, &count);
	if (bytes == NULL || !copy_written(session, scratch, copy, bytes, count))
		goto rewind;
	ok = parts == NULL || copy_written(session, scratch, parts_copy, parts_bytes, parts_count);
	voe_editor_assets_list_due(&scene->assets);
	materials_read(session, scene, models, scratch);
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

// `path` sent to the trash. False with the notice saying why.
static bool trash_sent(voe_editor_session *session, voe_base_arena *scratch, const char *path)
{
	voe_base_error error = VOE_BASE_ERROR_UNAVAILABLE;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "trashing no path");
	voe_base_report_error_clear();
	if (voe_platform_trash(on_disk(session, scratch, path), scratch, &error))
		return true;
	if (error == VOE_BASE_ERROR_UNSUPPORTED)
		voe_editor_notice_set(&session->notice,
				      "%s is on a drive the trash does not take from; nothing deleted",
				      project_relative(scratch, path));
	else
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
	return false;
}

bool voe_editor_assets_trash(voe_editor_session *session, voe_editor_scene *scene,
			     voe_editor_undo *undo, voe_editor_models *models,
			     voe_base_arena *scratch, const char *path)
{
	struct voe_base_arena_mark mark;
	const char *parts;
	bool sent;
	bool ok;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "trashing no path");
	VOE_BASE_ASSERT(models != NULL, "trashing with no materials to read");
	if (!command_allowed(session, scene, undo, scratch))
		return false;
	mark = voe_base_arena_mark(scratch);
	// Looked for before the `.glb` goes, which it must be beside.
	parts = parts_beside(session, scratch, path);
	sent = trash_sent(session, scratch, path);
	ok = sent && (parts == NULL || trash_sent(session, scratch, parts));
	if (sent)
		voe_editor_assets_list_due(&scene->assets);
	voe_base_arena_rewind(scratch, mark);
	if (sent)
		materials_read(session, scene, models, scratch);
	return ok;
}
