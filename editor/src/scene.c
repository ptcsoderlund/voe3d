// The untitled scene's two entities, the selection, and the rows the Scene
// panel drew. See the header for why the scene is code, why `Light` has no
// transform, and why the rows outlive the call that drew them.
//
// NOTHING IN HERE DRAWS AND NOTHING IN HERE LAYS ANYTHING OUT. It holds `ui`
// nodes because that is what a widget answers through, and it asks `ui` exactly
// one question — what did the pointer do to this button — after the frame has
// ended.
#include "scene.h"

#include <3d/shape_component.h>

#include <base/assert.h>

#include <math/float3.h>
#include <math/quat.h>

#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <ui/widgets.h>

#include <string.h>

// THE UNTITLED LIGHT: where its light goes, not where it is — down, and from
// the front-right, so the cube's three visible faces are three different
// brightnesses. Not unit length here; voe_scene_light_add normalizes it once.
// The same numbers view.c's fixed sun used before the light moved into the
// scene.
#define LIGHT_X (-0.4f)
#define LIGHT_Y (-1.0f)
#define LIGHT_Z (-0.6f)
#define LIGHT_INTENSITY 3.14159265f

// An entity a person authored: just the identity, whose presence is what says
// so (ADR-0125). A transform, a shape or a light is added by the caller once
// this returns — some authored entities have all three, `Light` has none of the
// first two.
static voe_ecs_entity identified(voe_ecs_world *world, uint64_t id,
				 const char *name)
{
	voe_ecs_entity entity;
	voe_scene_identity identity = { .id = id };

	VOE_BASE_ASSERT(voe_ecs_entity_create(world, &entity),
			"the editor's world is too small for its own scene");

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

void voe_editor_scene_untitled(voe_editor_scene *scene, voe_ecs_world *world)
{
	voe_ecs_entity cube;
	voe_ecs_entity light;

	VOE_BASE_ASSERT(scene != NULL, "building a scene into nothing");
	VOE_BASE_ASSERT(world != NULL, "building a scene in no world");

	*scene = (voe_editor_scene){ .world = world };

	cube = identified(world, 1, "Cube");
	VOE_BASE_ASSERT(
		voe_scene_transform_add(
			world, cube,
			(voe_scene_transform){
				.position = { 0.0f, 0.0f, 0.0f },
				.rotation = voe_math_quat_from_axis_angle(
					(voe_math_float3){ 0.0f, 1.0f, 0.0f },
					0.0f),
				.scale = { 1.0f, 1.0f, 1.0f } }),
		"the editor's transform table is too small for its own scene");
	VOE_BASE_ASSERT(
		voe_3d_shape_add(world, cube,
				 (voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE }),
		"the editor's shape table is too small for its own scene");

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
		"the editor's light table is too small for its own scene");

	VOE_BASE_ASSERT(voe_scene_identity_count(world) == 2,
			"the editor's untitled scene is not the two identities it is written to be");
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
