// The model component: its key, the registration, the reads, the intent's
// submit and the drain that is the whole of its system. Nothing here reads a
// file or draws; see 3d/model_component.h.
//
// THE INTENT'S KEY IS THIS FILE'S ALONE. The drain and the submit find the queue
// as the model's replace (ecs/component.h), so nothing else needs its name.
//
// THE CORRECTED RUN AND ITS COUNT ARE FILE-SCOPE STATICS, per process, as in
// shape_system.c: two worlds in one process share them.
#include <3d/model_component.h>

#include <base/assert.h>
#include <base/report.h>

#include <ecs/intent.h>

#include <scene/transform_component.h>

#include <inttypes.h>
#include <stddef.h>
#include <string.h>

const struct voe_ecs_key voe_3d_model_key = { "voe_3d_model" };

static const struct voe_ecs_key model_intent_key = { "voe_3d_model_intent" };

// Whether the last drain corrected any intent, and how many since that run began.
static bool in_corrected_run;
static uint32_t corrected_run_count;

static const voe_base_struct_description *model_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_3d_model_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

static voe_ecs_type model_type(const voe_ecs_world *world)
{
	return voe_ecs_component_type(world, &voe_3d_model_key);
}

void voe_3d_model_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering models in no world");

	type = voe_ecs_component_register(world, &voe_3d_model_key,
					  sizeof(voe_3d_model), capacity,
					  model_description());
	intent = voe_ecs_intent_register(world, &model_intent_key,
					 sizeof(voe_3d_model_intent), capacity);
	voe_ecs_component_replace_set(world, type, intent,
				      offsetof(voe_3d_model_intent, model));
	voe_ecs_component_default_set(world, type,
				      &(voe_3d_model){ .cast_shadows = true });
	voe_ecs_component_needs_set(
		world, type,
		voe_ecs_component_type(world, &voe_scene_transform_key));
	voe_ecs_component_menu_set(world, type, "Rendering / Model");
}

bool voe_3d_model_add(voe_ecs_world *world, voe_ecs_entity entity,
		      voe_3d_model model)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a model to no world");

	return voe_ecs_component_add(world, model_type(world), entity, &model);
}

const voe_3d_model *voe_3d_model_get(const voe_ecs_world *world,
				     voe_ecs_entity entity)
{
	return voe_ecs_component_get(world, model_type(world), entity);
}

uint32_t voe_3d_model_count(const voe_ecs_world *world)
{
	return voe_ecs_component_count(world, model_type(world));
}

const voe_3d_model *voe_3d_model_rows(const voe_ecs_world *world)
{
	return voe_ecs_component_rows(world, model_type(world));
}

const voe_ecs_entity *voe_3d_model_entities(const voe_ecs_world *world)
{
	return voe_ecs_component_entities(world, model_type(world));
}

bool voe_3d_model_submit(voe_ecs_world *world, voe_3d_model_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a model to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_component_replace(world, model_type(world)).intent,
		&intent);
}

void voe_3d_model_system_run(voe_ecs_world *world)
{
	voe_ecs_type type;
	voe_ecs_intent queue;
	const voe_3d_model_intent *intents;
	uint32_t count;
	uint32_t corrected = 0;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the model system on no world");

	type = model_type(world);
	queue = voe_ecs_component_replace(world, type).intent;
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_3d_model row = intents[i].model;

		if (voe_ecs_component_get(world, type, intents[i].entity) == NULL)
			continue;

		if (memchr(row.path, '\0', VOE_3D_MODEL_PATH) == NULL) {
			row.path[VOE_3D_MODEL_PATH - 1] = '\0';
			if (!in_corrected_run && corrected == 0)
				VOE_BASE_WARNING(
					"3d",
					"voe_3d_model: entity %" PRIu32 "v%" PRIu32
					": path of %d bytes with no end, cut to \"%s\"",
					intents[i].entity.index,
					intents[i].entity.generation,
					VOE_3D_MODEL_PATH, row.path);
			corrected++;
		}

		(void)voe_ecs_component_set(world, type, intents[i].entity, &row);
	}

	if (corrected > 0) {
		in_corrected_run = true;
		corrected_run_count += corrected;
	} else if (in_corrected_run) {
		VOE_BASE_WARNING("3d",
				 "voe_3d_model: %" PRIu32
				 " intents corrected in that run",
				 corrected_run_count);
		in_corrected_run = false;
		corrected_run_count = 0;
	}

	voe_ecs_intent_clear(world, queue);
}
