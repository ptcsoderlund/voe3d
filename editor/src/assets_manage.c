// The Assets panel's file commands — see assets_manage.h for the checks, the
// order a move goes in and why the undo line is forgotten.
//
// Every call clears the notice, runs the shared checks, then acts. Paths are
// built in scratch, which is rewound before each call returns.
#include "assets_manage.h"

#include "assets_walk.h"

#include <authoring/paths.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>
#include <platform/trash.h>
#include <scene/identity_component.h>

#include <stdio.h>
#include <string.h>

// The highest number a duplicate's name is given.
#define DUPLICATE_LAST 99

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
	path = folder[0] == '\0' ? name : voe_platform_path_join(scratch, folder, name);
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
// listing, a folder into itself, and every followed path on disk and in the
// open scene, which is left followed in `followed`.
static bool move_allowed(voe_editor_session *session, voe_base_arena *scratch, const char *from,
			 const char *to, voe_authoring_paths_followed *followed)
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
	return voe_editor_assets_follow_check(session->project->folder, project_from, project_to,
					      scratch, &session->notice) &&
	       scene_followed(session, scratch, project_from, project_to, followed);
}

bool voe_editor_assets_move(voe_editor_session *session, voe_editor_scene *scene,
			    voe_editor_undo *undo, voe_base_arena *scratch, const char *from,
			    const char *to)
{
	struct voe_base_arena_mark mark;
	voe_authoring_paths_followed followed;
	voe_editor_notice why = { 0 };
	bool ok = false;

	VOE_BASE_ASSERT(from != NULL && to != NULL, "moving no path");
	if (!command_allowed(session, scene, undo, scratch))
		return false;
	if (strcmp(from, to) == 0)
		return true;
	mark = voe_base_arena_mark(scratch);
	if (!move_allowed(session, scratch, from, to, &followed))
		goto rewind;
	voe_base_report_error_clear();
	if (!voe_platform_file_move(on_disk(session, scratch, from), on_disk(session, scratch, to),
				    NULL)) {
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, from));
		goto rewind;
	}
	ok = voe_editor_assets_follow_write(session->project->folder,
					    project_relative(scratch, from),
					    project_relative(scratch, to), scratch, &session->notice);
	// The first failure is the one said: a write's, else the scene's.
	if (!scene_set_followed(session, scene, &followed, &why)) {
		if (ok)
			session->notice = why;
		ok = false;
	}
	voe_editor_undo_forget(undo);
	voe_editor_assets_list_due(&scene->assets);
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

bool voe_editor_assets_duplicate(voe_editor_session *session, voe_editor_scene *scene,
				 voe_editor_undo *undo, voe_base_arena *scratch, const char *path)
{
	struct voe_base_arena_mark mark;
	voe_platform_folder_listing listing;
	const voe_platform_folder_entry *entry;
	struct split source;
	const char *copy;
	const uint8_t *bytes;
	size_t count = 0;
	bool ok = false;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "duplicating no path");
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
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(on_disk(session, scratch, path), scratch, &count, NULL);
	if (bytes == NULL) {
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
		goto rewind;
	}
	// A write refuses no bytes (platform/file.h), so an empty file is said here.
	if (count == 0) {
		voe_editor_notice_set(&session->notice, "%s is empty", project_relative(scratch, path));
		goto rewind;
	}
	copy = source.folder[0] == '\0' ? copy : voe_platform_path_join(scratch, source.folder, copy);
	voe_base_report_error_clear();
	if (!voe_platform_file_write(on_disk(session, scratch, copy), bytes, count, NULL)) {
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, copy));
		goto rewind;
	}
	voe_editor_assets_list_due(&scene->assets);
	ok = true;
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

bool voe_editor_assets_trash(voe_editor_session *session, voe_editor_scene *scene,
			     voe_editor_undo *undo, voe_base_arena *scratch, const char *path)
{
	struct voe_base_arena_mark mark;
	voe_base_error error = VOE_BASE_ERROR_UNAVAILABLE;
	bool ok;

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "trashing no path");
	if (!command_allowed(session, scene, undo, scratch))
		return false;
	mark = voe_base_arena_mark(scratch);
	voe_base_report_error_clear();
	ok = voe_platform_trash(on_disk(session, scratch, path), scratch, &error);
	if (ok)
		voe_editor_assets_list_due(&scene->assets);
	else if (error == VOE_BASE_ERROR_UNSUPPORTED)
		voe_editor_notice_set(&session->notice,
				      "%s is on another drive than the trash; nothing deleted",
				      project_relative(scratch, path));
	else
		voe_editor_notice_from_report(&session->notice, project_relative(scratch, path));
	voe_base_arena_rewind(scratch, mark);
	return ok;
}
