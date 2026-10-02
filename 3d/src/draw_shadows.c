// The sun's shadow passes: the cascades fitted to the frame's view, and every
// caster drawn once into each of the four (ADR-0258). The contract is
// voe_3d_draw_system_shadows's in 3d/draw_system.h.
//
// IT IS ITS OWN CALL AND NOT A STEP INSIDE _run, BECAUSE A PASS DOES NOT NEST.
// _run draws into the view's pass the loop has already opened, and a shadow
// pass has to be opened and closed before that one is (render/device.h): so the
// loop calls this between the frame's begin and the view's pass, and the frame
// it fills carries the record that pass reads.
//
// CASTERS ARE CHOSEN BY WHAT A SUN'S SHADOW IS OF (0258 point 5). World-layer,
// because the overlay is drawn above the world and nothing in the world stands
// under it; lit, because an unlit material is a mark or a readout and not a
// solid in the sun; opaque or cutout, because nothing see-through casts, and a
// cutout casts as solid since the depth-only pipeline has no fragment stage to
// discard with. Panels and the editor's marks cast nothing, and the frame's
// `hidden` is left out here as in _run. A model part in the frame's store casts
// by the same rule, its material the part's, in the world layer as every part is.
// A mesh whose shape, or a model whose row, has `cast_shadows` false casts
// nothing (0324 point 5); a mesh with no shape casts by the rules above.
//
// THE CASCADES NEED A LIGHT THAT CASTS (0324 point 4): the world's one light
// row with `cast_shadows` true. No light row casts nothing.
//
// Each caster's record is draw_group.c's, the object's matrix about the frame's
// eye at its lag, exactly as the view draws it; a cascade reads only the world
// matrix. The record carries the shape's colour as the view's does, because the
// bounce map's flux is the base colour times it: without it every shape bounced
// white, and a red box tinted nothing (bug 01).
// The mesh table is walked once per cascade: four linear walks, the same cost
// as the view's own walk, and a culled list per cascade is a later card.
//
// THE BOUNCE PASS FOLLOWS THE CASCADES (0308 point 1): draw_bounce.c opens it,
// draws the same casters through this file's walk, and updates the frame's
// target's grid. Only when cascades were drawn and the world's light, the row
// voe_3d_draw_system_light reads, has `bounces` of 1 or more (0319 point 3): at
// 0, or with no light, there is no bounce pass, draw or update; following the
// cascades, a light that casts nothing bounces nothing. A failure there is the
// call's.
#include "draw_bounce.h"
#include "draw_group.h"

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/shadow_cascades.h>
#include <3d/shape_component.h>
#include <base/assert.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

// Whether a material casts: lit, and not blended (0258 point 5).
bool voe_3d_draw_casts(const voe_3d_material *material)
{
	VOE_BASE_ASSERT(material != NULL, "casting with no material");
	return !material->unlit && material->alpha_mode != VOE_RENDER_ALPHA_BLENDED;
}

// Every loaded model part that casts, drawn into the shadow pass that is open,
// as draw_casters draws a mesh. Nothing with no store (0277 point 3).
static bool draw_model_casters(voe_ecs_world *world, voe_render_device *device,
			       const voe_3d_frame *frame)
{
	const voe_3d_model *rows = voe_3d_model_rows(world);
	const voe_ecs_entity *owners = voe_3d_model_entities(world);

	VOE_BASE_ASSERT(frame->models != NULL, "casting models from no store");
	VOE_BASE_ASSERT(voe_render_pass_is_open(device),
			"drawing casters with no shadow pass open");
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *model;
		voe_scene_transform drawn;

		if (!rows[row].cast_shadows ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden) ||
		    voe_scene_transform_get(world, owners[row]) == NULL)
			continue;
		model = voe_3d_models_find(frame->models, rows[row].path);
		if (model == NULL || !model->loaded)
			continue;
		drawn = voe_scene_transform_between(world, owners[row], frame->lag);
		for (uint32_t part = 0; part < model->part_count; part++) {
			const voe_3d_model_part *piece = &model->parts[part];

			if (voe_3d_draw_casts(&piece->material) &&
			    !voe_render_frame_draw(device, piece->geometry,
						   voe_3d_draw_group_object_of(
							   &drawn, &piece->material,
							   NULL, frame->eye)))
				return false;
		}
	}
	return true;
}

// Every caster in the world, drawn into the sun's pass that is open, a cascade
// or the bounce map. False when render refuses a draw, which it has already
// said on stderr.
bool voe_3d_draw_casters(voe_ecs_world *world, voe_render_device *device,
			 const voe_3d_frame *frame)
{
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	uint32_t count = voe_3d_mesh_count(world);
	voe_ecs_type shapes;
	bool has_shapes = voe_3d_draw_group_shape_type(world, &shapes);

	VOE_BASE_ASSERT(voe_render_pass_is_open(device),
			"drawing casters with no shadow pass open");
	for (uint32_t row = 0; row < count; row++) {
		const voe_3d_material *material;
		const voe_3d_shape *shape;
		voe_scene_transform drawn;

		if (meshes[row].layer != VOE_3D_LAYER_WORLD ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		material = voe_3d_material_get(world, owners[row]);
		shape = has_shapes ? voe_ecs_component_get(world, shapes,
							   owners[row]) :
				     NULL;
		// A shape whose flag is off is no caster (0324 point 5).
		if (material == NULL || !voe_3d_draw_casts(material) ||
		    (shape != NULL && !shape->cast_shadows) ||
		    voe_scene_transform_get(world, owners[row]) == NULL)
			continue;
		// Where it was `lag` of a step ago, as the view draws it (0254).
		drawn = voe_scene_transform_between(world, owners[row], frame->lag);
		if (!voe_render_frame_draw(device, meshes[row].geometry,
					   voe_3d_draw_group_object_of(
						   &drawn, material, shape,
						   frame->eye)))
			return false;
	}
	return frame->models == NULL || draw_model_casters(world, device, frame);
}

// Whether the world's light, row zero as voe_3d_draw_system_light reads it,
// bounces (0319 point 3). No light never does.
static bool light_bounces(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "bouncing no world");
	VOE_BASE_ASSERT(voe_scene_light_count(world) <= 1,
			"a world to draw has at most one light — see 3d/draw_system.h");
	return voe_scene_light_count(world) == 1 &&
	       voe_scene_light_rows(world)[0].bounces >= 1;
}

// Whether the world's light, the same row, casts (0324 point 4). No light
// never does.
static bool light_casts(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(voe_scene_light_count(world) <= 1,
			"a world to draw has at most one light — see 3d/draw_system.h");
	return voe_scene_light_count(world) == 1 &&
	       voe_scene_light_rows(world)[0].cast_shadows;
}

bool voe_3d_draw_system_shadows(voe_ecs_world *world, voe_render_device *device,
				voe_3d_frame *frame)
{
	voe_3d_shadow_cascades cascades;

	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL,
			"shadowing with no world, device or frame");
	VOE_BASE_ASSERT(!voe_render_pass_is_open(device),
			"the sun's shadow passes go before the view's pass — see 3d/draw_system.h");
	frame->shadow = (voe_render_shadow){ 0 };
	// A light of no strength casts nothing, and a zeroed one's direction
	// cannot orient cascades (0290 point 2); nor does an unshaded one, and
	// a blind camera draws no world, nor a light that does not cast (0324).
	if (frame->light.unshaded || frame->light.intensity <= 0.0f ||
	    frame->blind || !light_casts(world))
		return true;

	cascades = voe_3d_shadow_cascades_fit(frame->view, frame->eye,
					      frame->light.direction,
					      VOE_3D_SHADOW_TEXELS);
	for (uint32_t cascade = 0; cascade < VOE_RENDER_SHADOW_CASCADES; cascade++) {
		bool drawn;

		if (!voe_render_shadow_pass_begin(device, cascade,
						  &cascades.light[cascade]))
			return false;
		drawn = voe_3d_draw_casters(world, device, frame);
		voe_render_pass_end(device);
		if (!drawn)
			return false;
	}
	// The bounce pass after the cascades, only when there were cascades
	// (0308 point 1; no sun, 0287: neither) and the light bounces (0319).
	if (light_bounces(world) && !voe_3d_draw_bounce(world, device, frame))
		return false;
	frame->shadow = cascades.shadow;
	VOE_BASE_ASSERT(frame->shadow.count <= VOE_RENDER_SHADOW_CASCADES,
			"more cascades than render has");
	return true;
}
