// The particles' draws: each live particle's emitter's picture found in the
// frame's store, a camera-facing matrix about the eye scaled by its size, and
// its colour and alpha at its age, held in the world's blended group.
//
// THE QUAD FACES THE CAMERA THROUGH THE VIEW'S OWN AXES. Rows 0, 1 and 2 of the
// view matrix are the camera's right, up and back in world space; normalised,
// they are the matrix's first three columns, so the picture's quad, wound
// counter-clockwise seen from +Z, is turned to the eye whatever the camera's
// scale. The particle's position less the eye, in double, is the last column,
// as voe_scene_transform_matrix does for every other drawable (ADR-0250).
//
// `t` IS AGE OVER LIFE, clamped to [0, 1], and size, colour and alpha each lerp
// from start to end along it. Colour and alpha are the object's colour, which
// the shader multiplies into the picture (render/device.h's voe_render_object).
//
// Constraints: every particles row is walked every frame, one lookup of its
// emitter each; a picture entry is required, so a `.glb` named as a texture
// draws nothing.
#include "draw_particles.h"

#include <3d/emitter_component.h>
#include <3d/normal_matrix.h>
#include <base/assert.h>
#include <ecs/component.h>
#include <math/double3.h>
#include <math/float3.h>

// Whether the world registered the particles table: a walk of the types, as
// voe_3d_draw_group_shape_type's, because asking for an unregistered key asserts.
static bool has_particles(const voe_ecs_world *world)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_3d_particles_key)
			return true;
	}
	return false;
}

uint32_t voe_3d_draw_particles_count(const voe_ecs_world *world,
				     const voe_3d_models *models)
{
	uint32_t live = 0;

	VOE_BASE_ASSERT(world != NULL, "counting particles in no world");
	if (models == NULL || !has_particles(world))
		return 0;
	for (uint32_t row = 0; row < voe_3d_particles_count(world); row++)
		live += voe_3d_particles_rows(world)[row].count;
	return live;
}

// The camera-facing world matrix of a particle of `size` at `position`, about
// `eye`: the view's right, up and back as columns, each scaled by the size.
static voe_math_float4x4 facing(voe_math_float4x4 view, voe_math_double3 position,
				voe_math_double3 eye, float size)
{
	voe_math_float4x4 world = { 0 };
	voe_math_float3 at = voe_math_double3_to_float3(
		voe_math_double3_sub(position, eye));
	const float offset[3] = { at.x, at.y, at.z };

	for (uint32_t axis = 0; axis < 3; axis++) {
		voe_math_float3 column = voe_math_float3_scale(
			voe_math_float3_normalize((voe_math_float3){
				view.m[axis][0], view.m[axis][1],
				view.m[axis][2] }),
			size);

		world.m[0][axis] = column.x;
		world.m[1][axis] = column.y;
		world.m[2][axis] = column.z;
		world.m[axis][3] = offset[axis];
	}
	world.m[3][3] = 1.0f;
	return world;
}

// The picture an emitter draws with: its texture's entry, the dot for an empty
// one, NULL when the store has no loaded picture there.
static const voe_3d_model_entry *picture_of(const voe_3d_models *models,
					    const voe_3d_emitter *emitter)
{
	const voe_3d_model_entry *entry =
		voe_3d_models_find(models, emitter->texture);

	if (entry == NULL || !entry->loaded || !entry->picture ||
	    entry->part_count < 2)
		return NULL;
	return entry;
}

// One particle's record: the part's shading, the facing matrix and its normal
// matrix, and the colour and alpha at its age.
static voe_render_object object_of(const voe_3d_frame *frame,
				   const voe_3d_emitter *emitter,
				   const voe_3d_model_part *part,
				   const voe_3d_particle *particle)
{
	float t = particle->life > 0.0f ? particle->age / particle->life : 1.0f;
	voe_math_float3 colour;
	voe_render_object object = { 0 };

	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	colour = voe_math_float3_lerp(emitter->colour_start, emitter->colour_end, t);
	object.world = facing(frame->view.view, particle->position, frame->eye,
			      emitter->size_start +
				      (emitter->size_end - emitter->size_start) * t);
	object.normal = voe_3d_normal_matrix(object.world);
	object.shading = part->material.shading.index;
	object.colour = (voe_math_float4){
		colour.x, colour.y, colour.z,
		emitter->alpha_start + (emitter->alpha_end - emitter->alpha_start) * t
	};
	return object;
}

void voe_3d_draw_particles_hold(const voe_ecs_world *world,
				const voe_3d_frame *frame,
				struct voe_3d_draw_group *world_blended)
{
	const voe_3d_particles *rows;
	const voe_ecs_entity *owners;

	VOE_BASE_ASSERT(world != NULL && frame != NULL && world_blended != NULL,
			"drawing particles with no world, frame or group");
	if (frame->models == NULL || !has_particles(world))
		return;
	rows = voe_3d_particles_rows(world);
	owners = voe_3d_particles_entities(world);
	for (uint32_t row = 0; row < voe_3d_particles_count(world); row++) {
		const voe_3d_emitter *emitter;
		const voe_3d_model_entry *picture;
		const voe_3d_model_part *part;

		if (voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		emitter = voe_3d_emitter_get(world, owners[row]);
		if (emitter == NULL)
			continue;
		picture = picture_of(frame->models, emitter);
		if (picture == NULL)
			continue;
		part = &picture->parts[emitter->glow > 0.0f ? 1 : 0];
		VOE_BASE_ASSERT(rows[row].count <= VOE_3D_EMITTER_PARTICLES,
				"more live particles than a row holds");
		for (uint32_t i = 0; i < rows[row].count; i++) {
			struct voe_3d_deferred entry = {
				.panel = false,
				.mesh = { .geometry = part->geometry,
					  .object = object_of(frame, emitter, part,
							      &rows[row].particles[i]) },
			};

			voe_3d_draw_group_hold(world_blended, entry,
					       entry.mesh.object.world,
					       frame->view.view);
		}
	}
}
