// The transform system: registration, the one direct creation call, the drain
// that settles what it applies, and the report that settling wrote.
//
// THE DRAIN IS THE WHOLE SYSTEM. It reads the queue in order, settles each row,
// writes it, and clears. There is nothing per-frame to compute, because a
// transform's matrix is built on demand by whoever wants it
// (transform_component.h) rather than cached here.
//
// AN INTENT FOR AN ENTITY WITH NO TRANSFORM IS DROPPED, and the drain asks for
// the current row before it settles rather than letting voe_ecs_component_set
// refuse it afterwards — it needs to know the row is there anyway, because
// keeping the last valid transform is one of the two things settling can decide
// and there is nothing to keep when there is no row. A destroyed entity and one
// that never had a transform both come back NULL, and both are ordinary rather
// than wrong — see the header.
//
// SETTLING NEVER WRITES A ROTATION THAT IS NOT A ROTATION, WHICHEVER DOOR THE
// INTENT CAME THROUGH. Neither typed call checks one, so what is checked here is
// checked for the importer, for a call site placing something by hand and for a
// tool that only knows the offsets alike. scene/transform_component.h holds the
// rules and the reasoning at length.
//
// THE RUN FLAG AND THE COUNTERS ARE FILE-SCOPE STATICS AND THEREFORE PER
// PROCESS. Two worlds in one process share them. The header says why that is
// enough.
//
// THE TRANSFORM REGISTERS ITS DESCRIPTION WHEN THE BUILD HAS ONE, AND NULL WHEN IT
// DOES NOT. The accessor only exists when descriptions are compiled in
// (base/describe.h), so the #if below is the condition that header tests. The
// table it returns is this file's own static copy and lives as long as the program.
#include <base/assert.h>
#include <base/report.h>
#include <ecs/intent.h>
#include <scene/identity_component.h>
#include <scene/transform_system.h>

#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

// One intent per transform per frame is the sizing this assumes, so the queue is
// as long as the table.
static const struct voe_ecs_key transform_intent_key = {
	"voe_scene_transform_intent"
};

// Long enough for the longest sentence below with four `%g` floats in it.
#define REASON 160

// What settling decided about one intent. The middle one is a correction the
// submitter should hear about; a normalisation inside the tolerance is
// SETTLED_NOTHING, because that is arithmetic drifting rather than a mistake.
typedef enum {
	SETTLED_NOTHING,
	SETTLED_CORRECTED,
	SETTLED_KEPT,
} settling;

// Whether the last drain settled anything, how many intents the run it belongs
// to has settled so far, and whether any of them was kept rather than corrected.
// Per process — see the header.
static bool in_run;
static uint32_t run_count;
static bool run_kept;

static bool finite_vector(voe_math_float3 v)
{
	return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

static bool finite_rotation(voe_math_quat q)
{
	return isfinite(q.x) && isfinite(q.y) && isfinite(q.z) &&
	       isfinite(q.w);
}

static const voe_base_struct_description *transform_description(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	return voe_scene_transform_description();
#else
	return &voe_ecs_description_compiled_out;
#endif
}

// The world's identity table, or false when it has none. A world may register no
// identities at all — dev registers none — so this walks the types rather than
// asking voe_ecs_component_type for a key, which asserts when nothing was
// registered against it. The comparison is by address, which is what a key is
// (ecs/world.h).
static bool identity_type(const voe_ecs_world *world, voe_ecs_type *out)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) ==
		    &voe_scene_identity_key) {
			*out = type;
			return true;
		}
	}

	return false;
}

// What the report calls this entity: the name its identity carries where it has
// one, and its index and generation where it has none. The name is printed with
// a precision rather than as a plain string, so a row a tool wrote straight into
// the table without a terminating zero is cut here instead of read past.
static void label_of(const voe_ecs_world *world, voe_ecs_entity entity,
		     char *out, size_t size)
{
	voe_ecs_type type;
	const voe_scene_identity *identity = NULL;

	if (identity_type(world, &type))
		identity = voe_ecs_component_get(world, type, entity);

	if (identity != NULL)
		(void)snprintf(out, size, "%.*s",
			       VOE_SCENE_IDENTITY_NAME - 1, identity->name);
	else
		(void)snprintf(out, size, "%uv%u", entity.index,
			       entity.generation);
}

// Settles the row in place, in the order scene/transform_component.h lists the
// rules, and writes the sentence the report prints for the first fault it found.
// A row it says SETTLED_KEPT about is not written at all; one it says
// SETTLED_CORRECTED or SETTLED_NOTHING about is written as it stands here, which
// for both means with a unit rotation in it.
static settling settled(voe_scene_transform *transform, char *reason,
			size_t size)
{
	float length;

	if (!finite_vector(transform->position)) {
		(void)snprintf(reason, size,
			       "position was not finite (%g, %g, %g), kept the last valid transform",
			       (double)transform->position.x,
			       (double)transform->position.y,
			       (double)transform->position.z);
		return SETTLED_KEPT;
	}

	if (!finite_rotation(transform->rotation)) {
		(void)snprintf(reason, size,
			       "rotation was not finite (%g, %g, %g, %g), kept the last valid transform",
			       (double)transform->rotation.x,
			       (double)transform->rotation.y,
			       (double)transform->rotation.z,
			       (double)transform->rotation.w);
		return SETTLED_KEPT;
	}

	if (!finite_vector(transform->scale)) {
		(void)snprintf(reason, size,
			       "scale was not finite (%g, %g, %g), kept the last valid transform",
			       (double)transform->scale.x,
			       (double)transform->scale.y,
			       (double)transform->scale.z);
		return SETTLED_KEPT;
	}

	length = voe_math_quat_length(transform->rotation);

	// FINITE COMPONENTS ARE NOT A FINITE LENGTH. The sum of four squares is
	// computed in float (math/src/quat.c), so a component past about 1.8e19
	// squares to an infinity and the length comes back one — and dividing by
	// that would write four zeros, which is the one thing this drain exists
	// to prevent. There is no rotation nearest such a value either.
	if (!isfinite(length)) {
		(void)snprintf(reason, size,
			       "rotation was too long to measure (%g, %g, %g, %g), kept the last valid transform",
			       (double)transform->rotation.x,
			       (double)transform->rotation.y,
			       (double)transform->rotation.z,
			       (double)transform->rotation.w);
		return SETTLED_KEPT;
	}

	if (length < VOE_SCENE_TRANSFORM_LEAST_ROTATION) {
		(void)snprintf(reason, size,
			       "rotation had no length, kept the last valid transform");
		return SETTLED_KEPT;
	}

	// Unit either way, and the tolerance decides only whether anybody is
	// told: inside it the division is rounding, outside it the submitter
	// handed over something that was not a rotation.
	transform->rotation = voe_math_quat_normalize(transform->rotation);

	if (fabsf(length - 1.0f) > VOE_SCENE_TRANSFORM_ROTATION_TOLERANCE) {
		(void)snprintf(reason, size,
			       "rotation had length %.2f, normalised",
			       (double)length);
		return SETTLED_CORRECTED;
	}

	return SETTLED_NOTHING;
}

void voe_scene_transform_register(voe_ecs_world *world, uint32_t capacity)
{
	voe_ecs_type type;
	voe_ecs_intent intent;

	VOE_BASE_ASSERT(world != NULL, "registering transforms in no world");

	type = voe_ecs_component_register(world, &voe_scene_transform_key,
					  sizeof(voe_scene_transform), capacity,
					  transform_description());
	intent = voe_ecs_intent_register(world, &transform_intent_key,
					 sizeof(voe_scene_transform_intent),
					 capacity);
	voe_ecs_component_replace_set(
		world, type, intent,
		offsetof(voe_scene_transform_intent, transform));
	voe_ecs_component_default_set(
		world, type,
		&(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
					.scale = { 1.0f, 1.0f, 1.0f } });
}

bool voe_scene_transform_add(voe_ecs_world *world, voe_ecs_entity entity,
			     voe_scene_transform transform)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "adding a transform to no world");

	return voe_ecs_component_add(
		world, voe_ecs_component_type(world, &voe_scene_transform_key),
		entity, &transform);
}

bool voe_scene_transform_submit(voe_ecs_world *world,
				voe_scene_transform_intent intent)
{
	VOE_BASE_DEBUG_ASSERT(world != NULL, "submitting a transform to no world");

	return voe_ecs_intent_submit(
		world, voe_ecs_intent_type(world, &transform_intent_key),
		&intent);
}

void voe_scene_transform_system_run(voe_ecs_world *world)
{
	voe_ecs_intent queue;
	voe_ecs_type type;
	const voe_scene_transform_intent *intents;
	uint32_t count;
	uint32_t settled_count = 0;
	bool kept = false;

	VOE_BASE_DEBUG_ASSERT(world != NULL, "running the transform system on no world");

	queue = voe_ecs_intent_type(world, &transform_intent_key);
	type = voe_ecs_component_type(world, &voe_scene_transform_key);
	intents = voe_ecs_intent_queue(world, queue);
	count = voe_ecs_intent_count(world, queue);

	for (uint32_t i = 0; i < count; i++) {
		voe_scene_transform row = intents[i].transform;
		char reason[REASON];
		settling what;

		if (voe_ecs_component_get(world, type, intents[i].entity) ==
		    NULL)
			continue;

		what = settled(&row, reason, sizeof reason);

		if (what != SETTLED_NOTHING) {
			// One line at the start of a run and no more: the first
			// settling says what went wrong, and the count at the
			// end says how much of it there was.
			if (!in_run && settled_count == 0) {
				char label[VOE_SCENE_IDENTITY_NAME];

				label_of(world, intents[i].entity, label,
					 sizeof label);
				if (what == SETTLED_KEPT)
					VOE_BASE_ERROR("scene",
						       "voe_scene_transform: entity %s: %s",
						       label, reason);
				else
					VOE_BASE_WARNING("scene",
							 "voe_scene_transform: entity %s: %s",
							 label, reason);
			}
			settled_count++;
		}

		if (what == SETTLED_KEPT) {
			kept = true;
			continue;
		}

		(void)voe_ecs_component_set(world, type, intents[i].entity,
					    &row);
	}

	if (settled_count > 0) {
		in_run = true;
		run_count += settled_count;
		run_kept = run_kept || kept;
	} else if (in_run) {
		if (run_kept)
			VOE_BASE_ERROR("scene",
				       "voe_scene_transform: %" PRIu32
				       " intents settled in that run",
				       run_count);
		else
			VOE_BASE_WARNING("scene",
					 "voe_scene_transform: %" PRIu32
					 " intents settled in that run",
					 run_count);
		in_run = false;
		run_count = 0;
		run_kept = false;
	}

	voe_ecs_intent_clear(world, queue);
}
