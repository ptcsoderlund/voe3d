// The model rows' walk: each row's parts from the frame's store, drawn into the
// open frame as the mesh walk draws a world-layer mesh. Internal to 3d;
// draw_system.c's run calls voe_3d_draw_models once a frame and sizes the
// world's blended group with voe_3d_draw_models_part_count (draw_models.h).
//
// A model row at fade 1 or more is not drawn, and one between 0 and 1 is held
// blended (0336 point 3); each at the level the camera chooses for it (0425
// point 5). A part whose worn material is blended is held too; the rest are
// drawn solid now, two-sided when that material is double-sided (0421 point
// 2). A landscape row is skipped here: draw_system.c draws it by
// draw_terrain.c's nodes.
//
// It holds nothing between frames: every row's entry is looked up in the
// store each frame, and a held part is the group's, given back by _run's
// rewind. The walk is linear in the model table, as every walk in
// draw_system.c is.
#include "draw_models.h"
#include "models_store.h"

#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <base/assert.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

const voe_3d_model_entry *voe_3d_draw_models_entry(const voe_ecs_world *world,
						   const voe_3d_models *models,
						   uint32_t row)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(row < voe_3d_model_count(world), "a model row past the table");
	if (models == NULL ||
	    voe_scene_transform_get(world, voe_3d_model_entities(world)[row]) == NULL)
		return NULL;
	entry = voe_3d_models_find(models, voe_3d_model_rows(world)[row].path);
	if (entry == NULL || !entry->loaded)
		return NULL;
	VOE_BASE_ASSERT(entry->part_count <= VOE_3D_MODEL_PARTS,
			"a model with more parts than the store holds");
	return entry;
}

bool voe_3d_draw_models_is_landscape(const voe_3d_models *models,
				     const voe_3d_model_entry *entry)
{
	voe_3d_models_terrain terrain;

	VOE_BASE_ASSERT(models != NULL && entry != NULL,
			"asking a landscape of no store or entry");
	VOE_BASE_ASSERT(entry->loaded, "asking a landscape of an unloaded entry");
	return voe_3d_models_terrain_of(models, entry, &terrain);
}

// Every loaded part of every model row, which the world's blended group is
// sized for as it is for every mesh and panel, a fading row's held parts
// among them, and VOE_3D_LANDSCAPE_NODES for a landscape row. Nought with no
// store.
uint32_t voe_3d_draw_models_part_count(const voe_ecs_world *world,
				       const voe_3d_models *models)
{
	uint32_t parts = 0;

	VOE_BASE_ASSERT(world != NULL, "counting parts in no world");
	if (models == NULL)
		return 0;
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *entry =
			voe_3d_draw_models_entry(world, models, row);

		if (entry != NULL)
			parts += voe_3d_draw_models_is_landscape(models, entry) ?
					 VOE_3D_LANDSCAPE_NODES :
					 entry->part_count;
	}
	VOE_BASE_ASSERT(parts <= voe_3d_model_count(world) * VOE_3D_LANDSCAPE_NODES,
			"more parts than the rows can wear");
	return parts;
}

// One part's held or drawn entry: white, since a model has no shape
// (ADR-0191), or with `fading` its `faded` record at alpha 1 − fade (0336).
static struct voe_3d_deferred model_part_entry(const voe_3d_model_part *piece,
					       const voe_scene_transform *drawn,
					       bool fading, float fade,
					       voe_math_double3 eye)
{
	voe_3d_material material = piece->material;
	struct voe_3d_deferred entry = { .panel = false };

	VOE_BASE_ASSERT(!fading || (fade > 0.0f && fade < 1.0f),
			"a fading part outside 0 and 1");
	if (fading)
		material.shading = piece->faded;
	entry.mesh.geometry = piece->geometry;
	entry.mesh.object = voe_3d_draw_group_object_of(drawn, &material, NULL, NULL,
							eye);
	entry.mesh.two_sided = material.double_sided;
	if (fading)
		entry.mesh.object.colour.w = 1.0f - fade;
	VOE_BASE_ASSERT(entry.mesh.object.colour.w > 0.0f,
			"a drawn part with no alpha");
	return entry;
}

// A row's fade is read once: at or above 1 nothing is drawn, between 0 and 1
// every part is held blended, at or below 0 or not a number as a solid row
// (0336 point 3).
bool voe_3d_draw_models(voe_ecs_world *world, voe_render_device *device,
			const voe_3d_frame *frame,
			struct voe_3d_draw_group *world_blended)
{
	const voe_ecs_entity *owners;
	const voe_3d_model *rows;

	VOE_BASE_ASSERT(world != NULL && frame != NULL && world_blended != NULL,
			"drawing models with no world, frame or group");
	if (frame->models == NULL)
		return true;
	owners = voe_3d_model_entities(world);
	rows = voe_3d_model_rows(world);
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *model;
		voe_scene_transform drawn;
		uint32_t level;
		float fade = rows[row].fade;
		// False for a NaN as for nought, so either draws as ever.
		bool fading = fade > 0.0f && fade < 1.0f;

		if (fade >= 1.0f ||
		    voe_3d_draw_group_left_out(world, owners[row], frame->hidden))
			continue;
		model = voe_3d_draw_models_entry(world, frame->models, row);
		if (model == NULL ||
		    voe_3d_draw_models_is_landscape(frame->models, model))
			continue;
		drawn = voe_scene_transform_between(world, owners[row], frame->lag);
		level = voe_3d_model_level(model, &drawn, frame->eye, &frame->view);
		for (uint32_t part = 0; part < model->part_count; part++) {
			// Its own record, or the one its parts file names.
			voe_3d_model_part piece = voe_3d_draw_group_part_worn(
				frame->models, model, part, level);
			struct voe_3d_deferred entry = model_part_entry(
				&piece, &drawn, fading, fade, frame->eye);

			if (piece.geometry.generation == 0)
				continue;
			if (fading ||
			    piece.material.alpha_mode == VOE_RENDER_ALPHA_BLENDED)
				voe_3d_draw_group_hold(world_blended, entry,
						       entry.mesh.object.world,
						       frame->view.view);
			else if (!voe_3d_draw_group_mesh_draw(device, &entry,
							      false))
				return false;
		}
	}
	return true;
}
