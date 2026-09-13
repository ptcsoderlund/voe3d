// The four entities, the selection, and the rows the Scene panel drew. See the
// header for why the scene is code, why the fourth entity has no name, and why
// the rows outlive the call that drew them.
//
// NOTHING IN HERE DRAWS AND NOTHING IN HERE LAYS ANYTHING OUT. It holds `ui`
// nodes because that is what a widget answers through, and it asks `ui` exactly
// one question — what did the pointer do to this button — after the frame has
// ended.
#include "scene.h"

#include "cube.h"

#include <3d/material_component.h>
#include <3d/mesh_component.h>

#include <base/assert.h>

#include <math/float3.h>
#include <math/quat.h>

#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <ui/widgets.h>

#include <string.h>

// How far apart the three named things stand, and how big the marker is. Metres,
// as every length in this engine is.
//
// THE CUBES STAND ON A DIAGONAL, so that neither hides the other from the front
// or from the side — the two directions the scene views start from (view.c).
// Each is CUBE_X either side of the origin across and CUBE_Z along, which leaves
// more than a metre clear between them seen from either.
#define SPACING 2.0f
#define CUBE_X 1.25f
#define CUBE_Z 1.25f
#define MARKER_SCALE 0.5f

// The cubes' one material: an opaque, lit, light grey with nothing on it, so the
// shading is the sun and nothing else. Linear.
#define CUBE_GREY 0.7f
#define CUBE_ROUGHNESS 0.6f

// How far `Cube_2` is turned, so that "turned" is visible the day something
// draws it and so that the transform the inspector shows is not three tidy
// zeros. Radians: an eighth of a turn about +Y.
#define TURN 0.7853981633974483f

// A scale of one and a rotation of nothing, which is what every entity here
// starts from. There is no identity constant in `math` on purpose (math/quat.h)
// — an angle of zero about any axis is it.
static voe_scene_transform placed(voe_math_float3 position,
				  voe_math_quat rotation, float scale)
{
	return (voe_scene_transform){ .position = position,
				      .rotation = rotation,
				      .scale = { scale, scale, scale } };
}

static voe_math_quat unturned(void)
{
	return voe_math_quat_from_axis_angle((voe_math_float3){ 0, 1, 0 }, 0.0f);
}

// An entity with a transform and nothing else — the engine's own kind, and the
// one the list must not show.
static voe_ecs_entity unnamed(voe_ecs_world *world,
			      voe_scene_transform transform)
{
	voe_ecs_entity entity;

	VOE_BASE_ASSERT(voe_ecs_entity_create(world, &entity),
			"the editor's world is too small for its own scene");
	VOE_BASE_ASSERT(voe_scene_transform_add(world, entity, transform),
			"the editor's transform table is too small for its own scene");

	return entity;
}

// An entity a person authored: a transform, and the identity whose presence is
// what says so.
static voe_ecs_entity authored(voe_ecs_world *world, uint64_t id,
			       const char *name, voe_scene_transform transform)
{
	voe_ecs_entity entity = unnamed(world, transform);
	voe_scene_identity identity = { .id = id };

	// The name is copied into the row's fixed 64 bytes rather than pointed
	// at, because a component holds no pointers (scene/identity_component.h)
	// — so a literal here and a loaded name later are the same row.
	VOE_BASE_ASSERT(strlen(name) < VOE_SCENE_IDENTITY_NAME,
			"an editor scene name that does not fit an identity");
	memcpy(identity.name, name, strlen(name));

	VOE_BASE_ASSERT(voe_scene_identity_add(world, entity, identity),
			"the editor's identity table is too small for its own scene");

	return entity;
}

// What makes an entity drawn: the cube's geometry in the world layer, and the
// one material every cube shares.
static void drawn(voe_ecs_world *world, voe_ecs_entity entity,
		  voe_render_geometry cube, voe_3d_material material)
{
	VOE_BASE_ASSERT(voe_3d_mesh_add(world, entity,
					(voe_3d_mesh){
						.geometry = cube,
						.layer = VOE_3D_LAYER_WORLD }),
			"the editor's mesh table is too small for its own scene");
	VOE_BASE_ASSERT(voe_3d_material_add(world, entity, material),
			"the editor's material table is too small for its own scene");
}

bool voe_editor_scene_build(voe_editor_scene *scene, voe_ecs_world *world,
			    voe_render_device *gpu, voe_base_error *error)
{
	voe_render_geometry cube;
	voe_3d_material material = {
		.base_colour = { CUBE_GREY, CUBE_GREY, CUBE_GREY, 1.0f },
		.metallic = 0.0f,
		.roughness = CUBE_ROUGHNESS,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
	};
	voe_ecs_entity entity;

	VOE_BASE_ASSERT(scene != NULL, "building a scene into nothing");
	VOE_BASE_ASSERT(world != NULL, "building a scene in no world");
	VOE_BASE_ASSERT(gpu != NULL, "building a scene with no device");

	*scene = (voe_editor_scene){ .world = world };

	// Uploaded once and shared: one geometry and one record, however many
	// entities wear them (3d/material_component.h).
	if (!voe_render_geometry_create(gpu, voe_editor_cube_vertices,
					VOE_EDITOR_CUBE_VERTEX_COUNT,
					voe_editor_cube_indices,
					VOE_EDITOR_CUBE_INDEX_COUNT, &cube,
					error))
		return false;
	if (!voe_3d_material_upload(gpu, &material, error))
		return false;

	entity = authored(world, 1, "Cube",
			  placed((voe_math_float3){ -CUBE_X, 0, CUBE_Z },
				 unturned(), 1.0f));
	drawn(world, entity, cube, material);
	entity = authored(world, 2, "Cube_2",
			  placed((voe_math_float3){ CUBE_X, 0, -CUBE_Z },
				 voe_math_quat_from_axis_angle(
					 (voe_math_float3){ 0, 1, 0 }, TURN),
				 1.0f));
	drawn(world, entity, cube, material);
	(void)authored(world, 3, "Marker",
		 placed((voe_math_float3){ 0, SPACING, 0 }, unturned(),
			MARKER_SCALE));

	// THE FOURTH, AND THE WHOLE REASON THIS FUNCTION ASSERTS ANYTHING. It is
	// in the world with a transform and no identity, so the Scene panel must
	// show three names and not four. Counting names on a screen is a weak
	// check and this is the strong one: four transforms, three identities,
	// here, where both numbers are known.
	(void)unnamed(world, placed((voe_math_float3){ 0, -SPACING, 0 },
				    unturned(), 1.0f));

	VOE_BASE_ASSERT(voe_scene_transform_count(world) == 4,
			"the editor's scene is not the four transforms it is written to be");
	VOE_BASE_ASSERT(voe_scene_identity_count(world) == 3,
			"the editor's scene is not the three identities it is written to be");

	return true;
}

voe_ecs_entity voe_editor_scene_selected(const voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "asking what no scene has selected");
	VOE_BASE_ASSERT(scene->world != NULL,
			"asking what a scene with no world has selected");

	if (!voe_ecs_entity_alive(scene->world, scene->selected))
		return (voe_ecs_entity){ 0 };

	return scene->selected;
}

bool voe_editor_scene_is_selected(const voe_editor_scene *scene,
				  voe_ecs_entity entity)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);

	return selected.generation == entity.generation &&
	       selected.index == entity.index &&
	       selected.generation != 0;
}

void voe_editor_scene_rows_clear(voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "clearing the rows of no scene");

	scene->listed_count = 0;
}

void voe_editor_scene_row_add(voe_editor_scene *scene, voe_ui_node node,
			      voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(scene != NULL, "recording a row on no scene");

	if (scene->listed_count == VOE_EDITOR_SCENE_ROWS)
		return;

	scene->listed[scene->listed_count++] =
		(voe_editor_scene_row){ .node = node, .entity = entity };
}

void voe_editor_scene_clicks_read(voe_editor_scene *scene,
				  const voe_ui_context *ui)
{
	VOE_BASE_ASSERT(scene != NULL, "reading the clicks of no scene");
	VOE_BASE_ASSERT(ui != NULL, "reading clicks out of no interface");

	// A refused frame hands back VOE_UI_NODE_NONE for every widget past the
	// node budget, and asking one of those what the pointer did is the
	// caller's bug — so they are skipped rather than asserted on, because a
	// full frame is `ui`'s to report and not this file's to fail on.
	for (uint32_t i = 0; i < scene->listed_count; i++) {
		if (scene->listed[i].node == VOE_UI_NODE_NONE)
			continue;
		if (voe_ui_button_action(ui, scene->listed[i].node).fired)
			scene->selected = scene->listed[i].entity;
	}
}
