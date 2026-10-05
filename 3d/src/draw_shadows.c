// The shadow passes: each casting directional light's cascades fitted to the
// frame's view, every caster drawn once into each of its four (ADR-0258,
// 0357), and the point lights'
// one pass, then the probe bounce when a light bounces. The contract is
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
// nothing (0324 point 5); a mesh with no shape casts by the rules above. A
// model row with `fade` at or above 1 casts nothing, and a fading one casts
// as ever (0336 point 3); the bounce shares this walk.
//
// EVERY CASTING LIGHT CASTS ITS OWN CASCADES (0324 point 4, 0357 point 3): the
// first light and each of `more_lights`, whose light row has `cast_shadows`
// true. They take slots in table order up to what
// voe_render_shadow_lights_ready holds, slot s writing layers 4s to 4s + 3 and
// its record's `slot`; a light past what is ready draws unshadowed that frame,
// its record zeroed, and the array grows for the next. No light row casts
// nothing.
//
// Each caster's record is draw_group.c's, the object's matrix about the frame's
// eye at its lag, exactly as the view draws it; a cascade reads only the world
// matrix. The record carries the shape's colour as the view's does, because a
// probe's picture is the base colour times it: without it every shape bounced
// white, and a red box tinted nothing (bug 01).
// The mesh table is walked once per cascade: four linear walks, the same cost
// as the view's own walk, and a culled list per cascade is a later card.
//
// THEN ONE POINT-SHADOW PASS (0325 point 6), whether or not the sun casts: when
// the device's maps are ready and a light in the frame's points has a slot, the
// same casters are drawn into it through the same walk, one instanced draw
// each over the faces render finds. A lamp's shadow is of what the sun's is of
// (0324 point 5), so a shape or model with `cast_shadows` false casts for no
// lamp either; sharing the walk is what keeps the two from disagreeing.
//
// THEN THE PROBE BOUNCE (0326 point 8), whether or not the sun casts:
// draw_bounce.c begins the frame's target's bounce, draws the same casters into
// each capture pass render opens and, for a casting sun, into the relight's own
// sun map (0329), and relights. Who bounces: the world's light
// row, the one voe_3d_draw_system_light reads, with `bounces` of 1 or more while
// the frame's light has intensity above nought and is not `unshaded`; or any
// light in the frame's points with `bounces` of 1 or more. A blind frame bounces
// nothing. When nothing bounces nothing is called, so it costs nothing (0316):
// no begin, no volume, no pass. A failure there is the call's.
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

		// A gone row casts nothing; a fading one casts as ever (0336).
		if (!rows[row].cast_shadows || rows[row].fade >= 1.0f ||
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

// Every caster in the world, drawn into the pass that is open, a cascade, the
// point-shadow pass or a capture pass. False when render refuses a draw, which it has already
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

// Whether anything bounces this frame (0326 point 8): the world's light, row
// zero as voe_3d_draw_system_light reads it, with bounces and a frame light of
// some strength that is shaded, or a frame point light with bounces. A blind
// frame never does.
static bool anything_bounces(const voe_ecs_world *world,
			     const voe_3d_frame *frame)
{
	VOE_BASE_ASSERT(world != NULL, "bouncing no world");
	if (frame->blind)
		return false;
	if (voe_scene_light_count(world) >= 1 &&
	    voe_scene_light_rows(world)[0].bounces >= 1 &&
	    frame->light.intensity > 0.0f && !frame->light.unshaded)
		return true;
	for (uint32_t i = 0; i < frame->points.count; i++)
		if (frame->points.lights[i].bounces >= 1)
			return true;
	return false;
}

// Whether light row `light`, in table order, casts (0324 point 4). Past the
// count never does.
bool voe_3d_draw_light_casts(const voe_ecs_world *world, uint32_t light)
{
	VOE_BASE_ASSERT(world != NULL, "casting from no world");
	return light < voe_scene_light_count(world) &&
	       voe_scene_light_rows(world)[light].cast_shadows;
}

// Directional light `light` of the frame: 0 is `light` and `shadow`, i + 1 is
// `more_lights[i]`.
static voe_render_light *frame_light(voe_3d_frame *frame, uint32_t light)
{
	VOE_BASE_ASSERT(light <= frame->more_count, "a light past the frame's");
	return light == 0 ? &frame->light : &frame->more_lights[light - 1].light;
}

static voe_render_shadow *frame_shadow(voe_3d_frame *frame, uint32_t light)
{
	VOE_BASE_ASSERT(light <= frame->more_count, "a light past the frame's");
	return light == 0 ? &frame->shadow : &frame->more_lights[light - 1].shadow;
}

// Every directional light's shadow record zeroed: none reads a map.
static void zero_shadows(voe_3d_frame *frame)
{
	for (uint32_t light = 0; light <= frame->more_count; light++)
		*frame_shadow(frame, light) = (voe_render_shadow){ 0 };
}

// Whether the frame's light `light` casts. A light of no strength casts
// nothing, and a zeroed one's direction cannot orient cascades (0290 point 2);
// nor does an unshaded one, and a blind camera draws no world, nor a light
// whose row does not cast (0324).
static bool light_casts_now(const voe_ecs_world *world, voe_3d_frame *frame,
			    uint32_t light)
{
	const voe_render_light *drawn = frame_light(frame, light);

	return !drawn->unshaded && drawn->intensity > 0.0f && !frame->blind &&
	       voe_3d_draw_light_casts(world, light);
}

// One light's four cascades, fitted to its direction, into layers slot × 4
// onward (0357 point 3), and its record with `slot`. False when render refuses.
static bool draw_light_cascades(voe_ecs_world *world, voe_render_device *device,
				voe_3d_frame *frame, uint32_t light,
				uint32_t slot)
{
	voe_3d_shadow_cascades cascades = voe_3d_shadow_cascades_fit(
		frame->view, frame->eye, frame_light(frame, light)->direction,
		VOE_3D_SHADOW_TEXELS);

	for (uint32_t cascade = 0; cascade < VOE_RENDER_SHADOW_CASCADES; cascade++) {
		bool drawn;

		if (!voe_render_shadow_pass_begin(
			    device, slot * VOE_RENDER_SHADOW_CASCADES + cascade,
			    &cascades.light[cascade]))
			return false;
		drawn = voe_3d_draw_casters(world, device, frame);
		voe_render_pass_end(device);
		if (!drawn)
			return false;
	}
	VOE_BASE_ASSERT(cascades.shadow.count <= VOE_RENDER_SHADOW_CASCADES,
			"more cascades than render has");
	cascades.shadow.slot = slot;
	*frame_shadow(frame, light) = cascades.shadow;
	return true;
}

// Every casting directional light's cascades, a slot each in table order up
// to what the device's array holds (0357 point 3); the rest keep zeroed
// records. False when render refuses.
static bool draw_sun_shadows(voe_ecs_world *world, voe_render_device *device,
			     voe_3d_frame *frame)
{
	uint32_t casting = 0;
	uint32_t ready;
	uint32_t slot = 0;

	VOE_BASE_ASSERT(frame->more_count < VOE_RENDER_DIRECTIONAL_LIGHTS &&
				(frame->more_count == 0 || frame->more_lights != NULL),
			"more lights than a pass holds, or none to hold them");
	for (uint32_t light = 0; light <= frame->more_count; light++)
		if (light_casts_now(world, frame, light))
			casting++;
	if (casting == 0)
		return true;
	ready = voe_render_shadow_lights_ready(device, casting);
	for (uint32_t light = 0; light <= frame->more_count && slot < ready;
	     light++) {
		if (!light_casts_now(world, frame, light))
			continue;
		if (!draw_light_cascades(world, device, frame, light, slot))
			return false;
		slot++;
	}
	return true;
}

// Whether any of the frame's point lights has a shadow slot (0325 point 5).
static bool any_point_slotted(const voe_render_point_lights *points)
{
	for (uint32_t i = 0; i < points->count; i++)
		if (points->lights[i].shadow != 0)
			return true;
	return false;
}

// The one point-shadow pass, the casters drawn into it, when the device has
// the maps and a light has a slot (0325 point 6); nothing otherwise.
static bool draw_point_shadows(voe_ecs_world *world, voe_render_device *device,
			       const voe_3d_frame *frame)
{
	bool drawn;

	if (!voe_render_point_shadows_ready(device) ||
	    !any_point_slotted(&frame->points))
		return true;
	if (!voe_render_point_shadow_pass_begin(device, &frame->points))
		return false;
	drawn = voe_3d_draw_casters(world, device, frame);
	voe_render_pass_end(device);
	return drawn;
}

bool voe_3d_draw_system_shadows(voe_ecs_world *world, voe_render_device *device,
				voe_3d_frame *frame)
{
	VOE_BASE_ASSERT(world != NULL && device != NULL && frame != NULL,
			"shadowing with no world, device or frame");
	VOE_BASE_ASSERT(!voe_render_pass_is_open(device),
			"the shadow passes go before the view's pass — see 3d/draw_system.h");
	zero_shadows(frame);
	if (!draw_sun_shadows(world, device, frame) ||
	    !draw_point_shadows(world, device, frame) ||
	    (anything_bounces(world, frame) &&
	     !voe_3d_draw_bounce(world, device, frame))) {
		zero_shadows(frame);
		return false;
	}
	return true;
}
