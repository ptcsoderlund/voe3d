// The project being worked on. See the header for what owns what and why
// every failure destroys the arena it was building.
//
// EVERY WORLD IS BUILT THE SAME WAY, WHETHER UNTITLED OR READ OFF DISK:
// world_new() registers the seven component types once, so an untitled
// project and an opened one can never end up with different room for the
// same thing by two call sites drifting apart.
//
// THE UNTITLED SCENE'S TWO ENTITIES ARE BUILT HERE, NOT IN scene.c. scene.c is
// the Scene panel's selection and the rows it drew; what a fresh project
// starts holding is this file's decision, the same as what an opened one
// holds is authoring/scene_read.h's.
#include "project.h"

#include "scene.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>

#include <authoring/project.h>
#include <authoring/scene_read.h>
#include <authoring/scene_write.h>

#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <ecs/structure.h>
#include <ecs/world.h>

#include <math/float3.h>
#include <math/quat.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/transform_system.h>

#include <stdint.h>
#include <string.h>

// The project's own arena, the arena a scene read owns (project.h) and the
// scratch a save's text is built in. Block sizes, not limits.
#define PROJECT_ARENA (4u * 1024u * 1024u)
#define PROJECT_SCENE_ARENA (1u * 1024u * 1024u)
#define PROJECT_SAVE_SCRATCH (1u * 1024u * 1024u)

// What a project's world may hold. Seven component types, four of them with
// an intent queue, and the entities are a number to author into rather than a
// measurement of anything.
#define MAX_ENTITIES 1024
#define MAX_COMPONENT_TYPES 8
#define MAX_INTENT_TYPES 8

// The structural queue (ecs/structure.h): room for a frame's Add, Delete,
// Duplicate or component change many times over, and for the rows those
// requests carry.
#define STRUCTURE_REQUESTS 256
#define STRUCTURE_BYTES 32768

// Transforms and identities. The identities are VOE_EDITOR_SCENE_ROWS because
// that is how many the Scene panel can list, and a world that could hold an
// identity the list could not show would be a disagreement between two
// numbers in one program (scene.h). Transforms are wider: an entity the
// engine makes for itself has one and no identity.
#define MAX_TRANSFORMS 256
#define MAX_IDENTITIES VOE_EDITOR_SCENE_ROWS

// A light is only ever on an authored entity, so the room for it is the same
// as the room for an identity.
#define MAX_LIGHTS MAX_IDENTITIES

// A mesh and a material each, one per drawn entity, and the room for a shape
// is the same number: every shape the shape system finds becomes one. The
// panel table is walked by the draw system whether anything has one or not
// (3d/draw_system.h), so it is registered with room for one and nothing ever
// adds a row.
#define MAX_SHAPES VOE_EDITOR_PROJECT_MAX_DRAWN
#define MAX_PANELS 1

// The scene file's name inside every project this editor writes (ADR-0164).
// voe_editor_project_new_opened reads whatever project.voe3d names instead —
// see the header on why the two are not the same call.
#define SCENE_FILE "main.scene"

// The untitled light: where its light goes, not where it is — down, and from
// the front-right, so the cube's three visible faces are three different
// brightnesses. Not unit length here; voe_scene_light_add normalizes it once.
#define LIGHT_X (-0.4f)
#define LIGHT_Y (-1.0f)
#define LIGHT_Z (-0.6f)
#define LIGHT_INTENSITY 3.14159265f

// A fresh world with every component type a project may hold registered, and
// nothing in it yet.
static voe_ecs_world *world_new(voe_base_arena *arena)
{
	voe_ecs_world *world = voe_ecs_world_new(
		arena, (voe_ecs_limits){ .entities = MAX_ENTITIES,
					 .component_types = MAX_COMPONENT_TYPES,
					 .intent_types = MAX_INTENT_TYPES,
					 .structure_requests = STRUCTURE_REQUESTS,
					 .structure_bytes = STRUCTURE_BYTES });

	voe_scene_transform_register(world, MAX_TRANSFORMS);
	voe_scene_identity_register(world, MAX_IDENTITIES);
	voe_scene_light_register(world, MAX_LIGHTS);
	voe_3d_mesh_register(world, VOE_EDITOR_PROJECT_MAX_DRAWN);
	voe_3d_material_register(world, VOE_EDITOR_PROJECT_MAX_DRAWN);
	voe_3d_panel_register(world, MAX_PANELS);
	voe_3d_shape_register(world, MAX_SHAPES);

	return world;
}

// An entity a person authored: just the identity, whose presence is what says
// so (ADR-0125). A transform, a shape or a light is added by the caller once
// this returns — some authored entities have all three, `Light` has none of
// the first two.
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

// Builds the untitled scene's two entities — `Cube` and the `Light` that
// shows it — into world. See project.h on why this is here and not scene.c.
static void build_untitled(voe_ecs_world *world)
{
	voe_ecs_entity cube = identified(world, 1, "Cube");
	voe_ecs_entity light;

	VOE_BASE_ASSERT(
		voe_scene_transform_add(
			world, cube,
			(voe_scene_transform){
				.position = { 0.0f, 0.0f, 0.0f },
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

	// LIGHT HAS NO TRANSFORM. A directional light has no position — see
	// scene/light_component.h — so there is nothing to place it at.
	light = identified(world, 2, "Light");
	VOE_BASE_ASSERT(
		voe_scene_light_add(
			world, light,
			(voe_scene_light){
				.direction = { LIGHT_X, LIGHT_Y, LIGHT_Z },
				.colour = { 1.0f, 1.0f, 1.0f },
				.intensity = LIGHT_INTENSITY }),
		"a project's light table is too small for its own untitled scene");

	VOE_BASE_ASSERT(voe_scene_identity_count(world) == 2,
			"an untitled scene is not the two identities it is written to be");
}

voe_editor_project *voe_editor_project_new_untitled(void)
{
	voe_base_arena *arena = voe_base_arena_new(PROJECT_ARENA);
	voe_editor_project *project = voe_base_arena_push(arena, sizeof *project);

	*project = (voe_editor_project){
		.arena = arena, .scene_arena = voe_base_arena_new(PROJECT_SCENE_ARENA)
	};
	project->world = world_new(arena);
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
		// REASON — a person choosing a folder in Open, or one named on
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
		.arena = arena, .scene_arena = voe_base_arena_new(PROJECT_SCENE_ARENA)
	};
	project->world = world_new(arena);

	voe_base_report_error_clear();
	if (!voe_authoring_scene_read((const char *)scene_bytes, scene_size,
				      project->world, project->scene_arena,
				      &kept)) {
		// A world too small to hold the file is the same failure, and
		// reported the same way (authoring/scene_read.h) — there is
		// nothing more to add here.
		voe_editor_notice_from_report(why, scene_path);
		voe_base_arena_destroy(project->scene_arena);
		voe_base_arena_destroy(arena);
		return NULL;
	}

	project->kept = kept;
	project->folder = absolute;
	project->unsaved = false;

	return project;
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
	VOE_BASE_ASSERT(project != NULL, "destroying no project");

	voe_base_arena_destroy(project->scene_arena);
	voe_base_arena_destroy(project->arena);
}
