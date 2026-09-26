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
// `hidden` is left out here as in _run.
//
// Each caster's record is draw_group.c's, the object's matrix about the frame's
// eye at its lag, exactly as the view draws it; only the world matrix is read.
// The mesh table is walked once per cascade: four linear walks, the same cost
// as the view's own walk, and a culled list per cascade is a later card.
#include "draw_group.h"

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/shadow_cascades.h>
#include <base/assert.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

// Every caster in the world, drawn into the shadow pass that is open. False
// when render refuses a draw, which it has already said on stderr.
static bool draw_casters(voe_ecs_world *world, voe_render_device *device,
			 const voe_3d_frame *frame)
{
	const voe_3d_mesh *meshes = voe_3d_mesh_rows(world);
	const voe_ecs_entity *owners = voe_3d_mesh_entities(world);
	uint32_t count = voe_3d_mesh_count(world);

	VOE_BASE_ASSERT(voe_render_pass_is_open(device),
			"drawing casters with no shadow pass open");
	for (uint32_t row = 0; row < count; row++) {
		const voe_3d_material *material;
		voe_scene_transform drawn;

		if (meshes[row].layer != VOE_3D_LAYER_WORLD ||
		    voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		material = voe_3d_material_get(world, owners[row]);
		if (material == NULL || material->unlit ||
		    material->alpha_mode == VOE_RENDER_ALPHA_BLENDED ||
		    voe_scene_transform_get(world, owners[row]) == NULL)
			continue;
		// Where it was `lag` of a step ago, as the view draws it (0254).
		drawn = voe_scene_transform_between(world, owners[row], frame->lag);
		if (!voe_render_frame_draw(device, meshes[row].geometry,
					   voe_3d_draw_group_object_of(&drawn, material,
								       NULL, frame->eye)))
			return false;
	}
	return true;
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
	// No sun casts nothing (ADR-0238), and a blind camera draws no world.
	if (frame->light.unshaded || frame->blind)
		return true;

	cascades = voe_3d_shadow_cascades_fit(frame->view, frame->eye,
					      frame->light.direction,
					      VOE_3D_SHADOW_TEXELS);
	for (uint32_t cascade = 0; cascade < VOE_RENDER_SHADOW_CASCADES; cascade++) {
		bool drawn;

		if (!voe_render_shadow_pass_begin(device, cascade,
						  &cascades.light[cascade]))
			return false;
		drawn = draw_casters(world, device, frame);
		voe_render_pass_end(device);
		if (!drawn)
			return false;
	}
	frame->shadow = cascades.shadow;
	VOE_BASE_ASSERT(frame->shadow.count <= VOE_RENDER_SHADOW_CASCADES,
			"more cascades than render has");
	return true;
}
