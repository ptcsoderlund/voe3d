// The project being worked on. See the header for what owns what and why
// every failure destroys the arena it was building.
//
// EVERY WORLD IS BUILT THE SAME WAY, WHETHER UNTITLED, READ OFF DISK OR
// SWAPPED: world_make, so no two call sites can drift apart on what a
// project's world holds.
//
// THE UNTITLED SCENE'S THREE ENTITIES ARE BUILT HERE, NOT IN scene.c. scene.c is
// the Scene panel's selection and the rows it drew; what a fresh project
// starts holding is this file's decision, the same as what an opened one
// holds is authoring/scene_read.h's. Every world ends up with exactly one
// camera (0218): an untitled one is built with it, and an opened scene with
// none is given one and marked unsaved.
#include "project.h"

#include "prefabs.h"
#include "scene.h"

#include <3d/shape_component.h>

#include <authoring/project.h>
#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/component.h>
#include <ecs/structure.h>
#include <ecs/world.h>

#include <game/world.h>

#include <math/float3.h>
#include <math/quat.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/parent_component.h>
#include <scene/transform_system.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

// The project's own arena, the world's, the arena a scene read owns (project.h) and the
// scratch a save's text is built in. Block sizes, not limits.
#define PROJECT_ARENA (1u * 1024u * 1024u)
#define PROJECT_WORLD_ARENA (4u * 1024u * 1024u)
#define PROJECT_SCENE_ARENA (1u * 1024u * 1024u)
#define PROJECT_SAVE_SCRATCH (1u * 1024u * 1024u)

// Every row the Scene panel lists is an authored entity, so the world must
// have room for an identity on each of them (scene.h).
static_assert(VOE_EDITOR_SCENE_ROWS <= VOE_GAME_WORLD_AUTHORED);

// The scene file's name inside every project this editor writes (ADR-0164).
// voe_editor_project_new_opened reads whatever project.voe3d names instead —
// see the header on why the two are not the same call.
#define SCENE_FILE "main.scene"

// The untitled light: where its light goes — down, and from the front-right,
// so the cube's three visible faces are three different brightnesses — turned
// into the sun's rotation by voe_scene_light_facing, which takes any length.
// LIGHT_HEIGHT is where the sun stands, which is only where its marker is seen.
#define LIGHT_X (-0.4f)
#define LIGHT_Y (-1.0f)
#define LIGHT_Z (-0.6f)
#define LIGHT_INTENSITY 3.14159265f
#define LIGHT_HEIGHT 4.0

// Where the scene camera is put: up and back from the origin, looking at it.
#define CAMERA_Y 2.0
#define CAMERA_Z 6.0

// A project's world in arena: game/world.h's engine types, then code's own
// when it has a library.
static voe_ecs_world *world_make(voe_base_arena *arena,
				 const voe_editor_code *code)
{
	voe_ecs_world *world = voe_game_world_new(arena);

	VOE_BASE_ASSERT(world != NULL, "no world made");
	VOE_BASE_ASSERT(code != NULL, "making a world with no code to ask");
	if (code->library != NULL)
		code->register_types(world);
	return world;
}

// An entity a person authored: just the identity, whose presence is what says
// so (ADR-0125). A transform, a shape or a light is added by the caller once
// this returns — some authored entities have all three, `Light` has no shape.
static voe_ecs_entity identified(voe_ecs_world *world, uint64_t id,
				 const char *name)
{
	voe_ecs_entity entity;
	voe_scene_identity identity_row = { .id = id };

	VOE_BASE_ASSERT(voe_ecs_entity_create(world, &entity),
			"a project's world is too small for its own untitled scene");

	VOE_BASE_ASSERT(strlen(name) < VOE_SCENE_IDENTITY_NAME,
			"an untitled scene name that does not fit an identity");
	memcpy(identity_row.name, name, strlen(name));

	VOE_BASE_ASSERT(
		voe_scene_identity_add(world, entity, identity_row),
		"a project's identity table is too small for its own untitled scene");

	return entity;
}

// Adds the scene's one camera (0218) as a `Camera` with this id: at
// (0, CAMERA_Y, CAMERA_Z), yaw 0 and pitched down at the origin, scale one, and
// the lens voe_scene_camera_register made the default rather than a second copy
// of its numbers. With yaw 0 the pose is the pitch about X alone.
static void add_camera(voe_ecs_world *world, uint64_t id)
{
	voe_ecs_entity camera = identified(world, id, "Camera");
	const voe_scene_camera *lens = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));

	VOE_BASE_ASSERT(lens != NULL, "the camera has no default lens");
	VOE_BASE_ASSERT(
		voe_scene_transform_add(
			world, camera,
			(voe_scene_transform){
				.position = { 0.0, CAMERA_Y, CAMERA_Z },
				.rotation = voe_math_quat_from_axis_angle(
					(voe_math_float3){ 1.0f, 0.0f, 0.0f },
					-atanf(CAMERA_Y / CAMERA_Z)),
				.scale = { 1.0f, 1.0f, 1.0f } }),
		"a project's transform table is too small for its camera");
	VOE_BASE_ASSERT(voe_scene_camera_add(world, camera, *lens),
			"a project's camera table is too small for its camera");
}

// Builds the untitled scene's three entities — `Cube`, the `Light` that
// shows it and the `Camera` — into world. See project.h on why this is here and not scene.c.
static void build_untitled(voe_ecs_world *world)
{
	voe_ecs_entity cube = identified(world, 1, "Cube");
	voe_ecs_entity light;

	VOE_BASE_ASSERT(
		voe_scene_transform_add(
			world, cube,
			(voe_scene_transform){
				.position = { 0.0, 0.0, 0.0 },
				.rotation = voe_math_quat_from_axis_angle(
					(voe_math_float3){ 0.0f, 1.0f, 0.0f },
					0.0f),
				.scale = { 1.0f, 1.0f, 1.0f } }),
		"a project's transform table is too small for its own untitled scene");
	VOE_BASE_ASSERT(
		voe_3d_shape_add(world, cube,
				 (voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
						 .colour = VOE_3D_SHAPE_GREY }),
		"a project's shape table is too small for its own untitled scene");

	// THE LIGHT'S TRANSFORM TURNS IT (ADR-0273): its rotation is the
	// direction it shines. A directional light lights the same from
	// anywhere, so its position only places the marker a person sees.
	light = identified(world, 2, "Light");
	VOE_BASE_ASSERT(
		voe_scene_transform_add(
			world, light,
			(voe_scene_transform){
				.position = { 0.0, LIGHT_HEIGHT, 0.0 },
				.rotation = voe_scene_light_facing(
					(voe_math_float3){ LIGHT_X, LIGHT_Y,
							   LIGHT_Z }),
				.scale = { 1.0f, 1.0f, 1.0f } }),
		"a project's transform table is too small for its own untitled scene");
	VOE_BASE_ASSERT(
		voe_scene_light_add(
			world, light,
			(voe_scene_light){
				.colour = { 1.0f, 1.0f, 1.0f },
				.intensity = LIGHT_INTENSITY }),
		"a project's light table is too small for its own untitled scene");

	add_camera(world, 3);

	VOE_BASE_ASSERT(voe_scene_identity_count(world) == 3,
			"an untitled scene is not the three identities it is written to be");
}

voe_editor_project *voe_editor_project_new_untitled(void)
{
	voe_base_arena *arena = voe_base_arena_new(PROJECT_ARENA);
	voe_editor_project *project = voe_base_arena_push(arena, sizeof *project);

	*project = (voe_editor_project){
		.arena = arena,
		.world_arena = voe_base_arena_new(PROJECT_WORLD_ARENA),
		.scene_arena = voe_base_arena_new(PROJECT_SCENE_ARENA)
	};
	project->world = world_make(project->world_arena, &project->code);
	build_untitled(project->world);

	return project;
}

voe_editor_project *voe_editor_project_new_opened(const char *folder,
						  voe_editor_notice *why)
{
	voe_base_arena *arena;
	voe_base_error error;
	const char *absolute;
	const char *project_path;
	const uint8_t *project_bytes;
	size_t project_size;
	voe_authoring_project file;
	const char *scene_path;
	const uint8_t *scene_bytes;
	size_t scene_size;
	voe_authoring_kept kept;
	voe_editor_project *project;

	VOE_BASE_ASSERT(folder != NULL, "opening a project with no folder");
	VOE_BASE_ASSERT(why != NULL, "opening a project with nowhere to say why");

	arena = voe_base_arena_new(PROJECT_ARENA);

	voe_base_report_error_clear();
	absolute = voe_platform_path_absolute(folder, arena, &error);
	if (absolute == NULL) {
		voe_editor_notice_from_report(why, folder);
		voe_base_arena_destroy(arena);
		return NULL;
	}

	project_path =
		voe_platform_path_join(arena, absolute, VOE_AUTHORING_PROJECT_FILE);

	voe_base_report_error_clear();
	project_bytes = voe_platform_file_read(project_path, arena,
					       &project_size, &error);
	if (project_bytes == NULL) {
		// WORDED AS "IS NOT A PROJECT" RATHER THAN THE OPEN'S OWN
		// REASON — a person picking a folder in Open, or one named on
		// the command line, needs to hear what the folder is not, not
		// what the operating system called a missing file.
		voe_editor_notice_set(why, "%s is not a project", project_path);
		voe_base_arena_destroy(arena);
		return NULL;
	}

	voe_base_report_error_clear();
	if (!voe_authoring_project_read((const char *)project_bytes,
					project_size, arena, &file)) {
		voe_editor_notice_from_report(why, project_path);
		voe_base_arena_destroy(arena);
		return NULL;
	}

	scene_path = voe_platform_path_join(arena, absolute, file.scene);

	voe_base_report_error_clear();
	scene_bytes = voe_platform_file_read(scene_path, arena, &scene_size,
					     &error);
	if (scene_bytes == NULL) {
		voe_editor_notice_from_report(why, scene_path);
		voe_base_arena_destroy(arena);
		return NULL;
	}

	project = voe_base_arena_push(arena, sizeof *project);
	*project = (voe_editor_project){
		.arena = arena,
		.world_arena = voe_base_arena_new(PROJECT_WORLD_ARENA),
		.scene_arena = voe_base_arena_new(PROJECT_SCENE_ARENA)
	};
	project->world = world_make(project->world_arena, &project->code);

	voe_base_report_error_clear();
	if (!voe_authoring_scene_read((const char *)scene_bytes, scene_size,
				      project->world, project->scene_arena,
				      &kept)) {
		// A world too small to hold the file is the same failure, and
		// reported the same way (authoring/scene_read.h) — there is
		// nothing more to add here.
		voe_editor_notice_from_report(why, scene_path);
		voe_base_arena_destroy(project->scene_arena);
		voe_base_arena_destroy(project->world_arena);
		voe_base_arena_destroy(arena);
		return NULL;
	}

	project->kept = kept;
	project->folder = absolute;
	project->unsaved = false;

	// A SCENE WRITTEN BEFORE 0218 HAS NO CAMERA, so it is given one above
	// every id it holds and shown as changed until it is saved.
	if (voe_scene_camera_count(project->world) == 0) {
		const voe_scene_identity *rows =
			voe_scene_identity_rows(project->world);
		uint32_t count = voe_scene_identity_count(project->world);
		uint64_t largest = 0;
		uint32_t i;

		for (i = 0; i < count; i++)
			if (rows[i].id > largest)
				largest = rows[i].id;
		add_camera(project->world, largest + 1);
		project->unsaved = true;
	}

	voe_editor_prefabs_expand(project->world, absolute, arena, why);
	return project;
}

// The open prefab's world written to `<folder>/<prefab>`: refused unless it is
// one tree whose root voe_editor_prefab_refused lets through (0283 point 1).
static bool prefab_save(voe_editor_project *project, voe_editor_notice *why)
{
	const voe_ecs_entity *authored = voe_scene_identity_entities(project->world);
	uint32_t count = voe_scene_identity_count(project->world);
	voe_ecs_entity root = { 0 };
	uint32_t roots = 0;
	voe_base_arena *scratch;
	voe_base_error error;
	const char *path;
	voe_authoring_text text;
	bool saved = false;

	VOE_BASE_ASSERT(project->folder != NULL && project->prefab[0] != '\0',
			"saving a prefab with no prefab open");
	for (uint32_t i = 0; i < count; i++) {
		if (voe_scene_parent_get(project->world, authored[i]) == NULL) {
			root = authored[i];
			roots++;
		}
	}
	if (roots != 1) {
		voe_editor_notice_set(
			why, "A prefab holds exactly one entity with no parent.");
		return false;
	}
	if (voe_editor_prefab_refused(project->world, root, why))
		return false;

	scratch = voe_base_arena_new(PROJECT_SAVE_SCRATCH);
	path = voe_platform_path_join(scratch, project->folder, project->prefab);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, scratch, &text) ||
	    !voe_platform_file_write(path, (const uint8_t *)text.text,
				     text.size, &error)) {
		voe_editor_notice_from_report(why, path);
	} else {
		project->unsaved = false;
		saved = true;
	}
	voe_base_arena_destroy(scratch);
	return saved;
}

bool voe_editor_project_save(voe_editor_project *project, const char *folder,
			     voe_editor_notice *why)
{
	voe_base_arena *scratch;
	voe_base_error error;
	const char *target;
	const char *scene_path;
	voe_authoring_text scene;

	VOE_BASE_ASSERT(project != NULL, "saving no project");
	VOE_BASE_ASSERT(why != NULL, "saving a project with nowhere to say why");
	VOE_BASE_ASSERT((project->folder == NULL) == (folder != NULL),
			"folder is required for an untitled project and refused for an opened one");

	if (project->prefab[0] != '\0')
		return prefab_save(project, why);

	scratch = voe_base_arena_new(PROJECT_SAVE_SCRATCH);

	if (project->folder == NULL) {
		voe_platform_folder_listing listing;
		const char *absolute;

		voe_base_report_error_clear();
		if (!voe_platform_folder_list(folder, scratch, &listing,
					      &error)) {
			voe_editor_notice_from_report(why, folder);
			voe_base_arena_destroy(scratch);
			return false;
		}
		if (listing.count != 0) {
			voe_editor_notice_set(why, "%s is not empty", folder);
			voe_base_arena_destroy(scratch);
			return false;
		}

		voe_base_report_error_clear();
		absolute = voe_platform_path_absolute(folder, project->arena,
						      &error);
		if (absolute == NULL) {
			voe_editor_notice_from_report(why, folder);
			voe_base_arena_destroy(scratch);
			return false;
		}
		target = absolute;
	} else {
		target = project->folder;
	}

	scene_path = voe_platform_path_join(scratch, target, SCENE_FILE);

	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, scratch, &scene)) {
		voe_editor_notice_from_report(why, scene_path);
		voe_base_arena_destroy(scratch);
		return false;
	}

	voe_base_report_error_clear();
	if (!voe_platform_file_write(scene_path, (const uint8_t *)scene.text,
				     scene.size, &error)) {
		voe_editor_notice_from_report(why, scene_path);
		voe_base_arena_destroy(scratch);
		return false;
	}

	if (project->folder == NULL) {
		const char *project_path = voe_platform_path_join(
			scratch, target, VOE_AUTHORING_PROJECT_FILE);
		voe_authoring_project file = { .scene = SCENE_FILE };
		size_t project_size;
		const char *project_text = voe_authoring_project_write(
			&file, scratch, &project_size);

		voe_base_report_error_clear();
		if (!voe_platform_file_write(project_path,
					     (const uint8_t *)project_text,
					     project_size, &error)) {
			voe_editor_notice_from_report(why, project_path);
			voe_base_arena_destroy(scratch);
			return false;
		}

		project->folder = target;
	}

	project->unsaved = false;
	voe_base_arena_destroy(scratch);
	return true;
}

bool voe_editor_project_scene_text(const voe_editor_project *project,
				   voe_base_arena *arena,
				   voe_authoring_text *out)
{
	VOE_BASE_ASSERT(project != NULL, "asking no project for its scene text");
	VOE_BASE_ASSERT(arena != NULL, "writing a project's scene text nowhere");
	VOE_BASE_ASSERT(out != NULL, "writing a project's scene text with nowhere to put it");

	return voe_authoring_scene_write(project->world, &project->kept, arena,
					 out);
}

bool voe_editor_project_scene_set(voe_editor_project *project, const char *text,
				  size_t size, voe_editor_notice *why)
{
	const voe_ecs_entity *authored;
	uint32_t count;
	uint32_t i;
	voe_authoring_kept kept;

	VOE_BASE_ASSERT(project != NULL, "setting no project's scene");
	VOE_BASE_ASSERT(text != NULL, "setting a project's scene to no text");
	VOE_BASE_ASSERT(why != NULL,
			"setting a project's scene with nowhere to say why");

	// EVERY AUTHORED ENTITY GOES, AND THE QUEUE IS APPLIED BEFORE THE READ,
	// because authoring/scene_read.h reads into a world with nothing
	// authored in it. An entity the engine made for itself has no identity
	// and is not one of these.
	authored = voe_scene_identity_entities(project->world);
	count = voe_scene_identity_count(project->world);
	for (i = 0; i < count; i++)
		VOE_BASE_ASSERT(
			voe_ecs_structure_destroy(project->world, authored[i]),
			"a project's structural queue is too small to empty its own world");
	voe_ecs_structure_apply(project->world);

	voe_base_arena_clear(project->scene_arena);

	voe_base_report_error_clear();
	if (!voe_authoring_scene_read(text, size, project->world,
				      project->scene_arena, &kept)) {
		voe_editor_notice_from_report(why, "the scene");
		return false;
	}

	project->kept = kept;
	voe_editor_prefabs_expand(project->world, project->folder,
				  project->scene_arena, why);
	return true;
}

bool voe_editor_project_code_set(voe_editor_project *project,
				 voe_editor_code code, voe_editor_notice *why)
{
	voe_base_arena *scratch;
	voe_base_arena *world_arena;
	voe_base_arena *scene_arena;
	voe_ecs_world *world;
	voe_authoring_text text;
	voe_authoring_kept kept;

	VOE_BASE_ASSERT(project != NULL, "setting no project's code");
	VOE_BASE_ASSERT(why != NULL, "setting code with nowhere to say why");

	scratch = voe_base_arena_new(PROJECT_SAVE_SCRATCH);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, scratch, &text)) {
		voe_editor_notice_from_report(why, "the scene");
		voe_base_arena_destroy(scratch);
		voe_editor_code_close(&code);
		return false;
	}

	world_arena = voe_base_arena_new(PROJECT_WORLD_ARENA);
	scene_arena = voe_base_arena_new(PROJECT_SCENE_ARENA);
	world = world_make(world_arena, &code);

	voe_base_report_error_clear();
	if (!voe_authoring_scene_read(text.text, text.size, world, scene_arena,
				      &kept)) {
		voe_editor_notice_from_report(why, "the scene");
		voe_base_arena_destroy(scene_arena);
		voe_base_arena_destroy(world_arena);
		voe_base_arena_destroy(scratch);
		voe_editor_code_close(&code);
		return false;
	}
	voe_editor_prefabs_expand(world, project->folder, scratch, why);

	// THE OLD WORLD GOES BEFORE THE OLD CODE: it holds that code's keys.
	voe_base_arena_destroy(project->world_arena);
	voe_base_arena_destroy(project->scene_arena);
	voe_editor_code_close(&project->code);
	voe_base_arena_destroy(scratch);
	project->world_arena = world_arena;
	project->scene_arena = scene_arena;
	project->world = world;
	project->kept = kept;
	project->code = code;
	VOE_BASE_ASSERT(project->world == world, "the swap left the old world");
	return true;
}

bool voe_editor_project_prefab_open(voe_editor_project *project,
				    const char *path, voe_editor_notice *why)
{
	voe_base_arena *arena;
	voe_base_error error;
	voe_authoring_text level;
	const char *absolute;
	const uint8_t *bytes;
	size_t size;
	voe_editor_notice ignored;

	VOE_BASE_ASSERT(project != NULL && path != NULL && why != NULL,
			"opening a prefab with no project, path or notice");
	VOE_BASE_ASSERT(project->folder != NULL, "opening a prefab untitled");
	VOE_BASE_ASSERT(project->prefab[0] == '\0' &&
				project->level_arena == NULL,
			"opening a prefab while one is open");

	if (strlen(path) >= sizeof project->prefab) {
		voe_editor_notice_set(why, "The prefab's path is too long.");
		return false;
	}
	arena = voe_base_arena_new(PROJECT_SAVE_SCRATCH);
	voe_base_report_error_clear();
	if (!voe_editor_project_scene_text(project, arena, &level)) {
		voe_editor_notice_from_report(why, "the scene");
		voe_base_arena_destroy(arena);
		return false;
	}
	absolute = voe_platform_path_join(arena, project->folder, path);
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(absolute, arena, &size, &error);
	if (bytes == NULL) {
		voe_editor_notice_from_report(why, path);
		voe_base_arena_destroy(arena);
		return false;
	}
	if (!voe_editor_project_scene_set(project, (const char *)bytes, size,
					  why)) {
		// The world is half-loaded; the level's own text always fits
		// back into the world it came from (scene_set).
		bool restored = voe_editor_project_scene_set(
			project, level.text, level.size, &ignored);

		VOE_BASE_ASSERT(restored,
				"the level did not read back after a refused prefab");
		voe_base_arena_destroy(arena);
		return false;
	}

	memcpy(project->prefab, path, strlen(path) + 1);
	project->level_arena = arena;
	project->level_text = level.text;
	project->level_size = level.size;
	project->level_unsaved = project->unsaved;
	project->unsaved = false;
	VOE_BASE_ASSERT(project->prefab[0] != '\0', "an open prefab with no path");
	return true;
}

bool voe_editor_project_prefab_back(voe_editor_project *project,
				    voe_editor_notice *why)
{
	VOE_BASE_ASSERT(project != NULL && why != NULL,
			"going back from no project or with nowhere to say why");
	VOE_BASE_ASSERT(project->prefab[0] != '\0' &&
				project->level_arena != NULL,
			"going back with no prefab open");

	if (!voe_editor_project_scene_set(project, project->level_text,
					  project->level_size, why))
		return false;
	project->unsaved = project->level_unsaved;
	voe_base_arena_destroy(project->level_arena);
	project->level_arena = NULL;
	project->level_text = NULL;
	project->level_size = 0;
	project->prefab[0] = '\0';
	VOE_BASE_ASSERT(project->level_arena == NULL, "a level left set aside");
	return true;
}

const char *voe_editor_project_name(const voe_editor_project *project)
{
	VOE_BASE_ASSERT(project != NULL, "asking the name of no project");

	if (project->folder == NULL)
		return NULL;
	return voe_platform_path_name(project->folder);
}

void voe_editor_project_destroy(voe_editor_project *project)
{
	voe_editor_code code;

	VOE_BASE_ASSERT(project != NULL, "destroying no project");

	// The struct is in arena, so the code is copied out before it goes and
	// closed after every arena, the world's included (code.h).
	code = project->code;
	if (project->level_arena != NULL)
		voe_base_arena_destroy(project->level_arena);
	voe_base_arena_destroy(project->scene_arena);
	voe_base_arena_destroy(project->world_arena);
	voe_base_arena_destroy(project->arena);
	voe_editor_code_close(&code);
	VOE_BASE_ASSERT(code.library == NULL, "a destroyed project's code is open");
}
