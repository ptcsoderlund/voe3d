// One field's value spelled into a scene's text — used by scene_write.c, once
// for each field of each section it writes.
//
// THE OUTPUT IS MEASURED OR WRITTEN. An output with no bytes counts and writes
// nothing; that is the writer's measuring pass, and the only pass on which a
// refusal is reported, so a refusal is reported once, not twice.
//
// EVERY REFUSAL ABOUT A VALUE IS MADE HERE — an ENUM, a NaN or infinite float, a
// control byte in a string, a description compiled out — naming the site the
// caller gives. A refused value means nothing is written.
#pragma once

#include <base/describe.h>
#include <ecs/component.h>

#include <stddef.h>
#include <stdint.h>

// The text being measured or written.
typedef struct {
	// NULL on the measuring pass, which counts and writes nothing.
	char *bytes;
	size_t size;
} voe_authoring_output;

// Where a value sits, for a report: which entity, which component, which field.
// `field` is a buffer and not a pointer because an element inside an array
// names its index too — `tags[1]` — built up as the shape is walked down and
// torn down as it returns, one field at a time, never two at once. `world` and
// `identity` are what an ENTITY element is spelled as an authored id through.
typedef struct {
	const voe_ecs_world *world;
	voe_ecs_type identity;
	uint64_t id;
	const char *component;
	char field[128];
} voe_authoring_value_site;

bool voe_authoring_measuring(const voe_authoring_output *output);
void voe_authoring_put(voe_authoring_output *output, const char *bytes,
		       size_t size);
void voe_authoring_put_string(voe_authoring_output *output,
			      const char *string);
void voe_authoring_put_unsigned(voe_authoring_output *output, uint64_t value);

// True when `entity` is a placed copy's part other than its root: its part row
// names another entity. The walk does not write it, and an ENTITY naming it is
// written 0 (0283 point 3). False in a world with no part table.
bool voe_authoring_prefab_part_skipped(const voe_ecs_world *world,
				       voe_ecs_entity entity);

// `name = value\n` for one field of `row`; false, reported, when the value
// cannot be written.
[[nodiscard]] bool
voe_authoring_value_put_field(voe_authoring_output *output,
			      voe_authoring_value_site *site,
			      const voe_base_field_description *field,
			      const uint8_t *row);

// True when the section at `site` has a description to write it through;
// false, reported, when this build compiled it out.
[[nodiscard]] bool voe_authoring_value_described_or_refuse(
	const voe_authoring_output *output,
	const voe_authoring_value_site *site,
	const voe_base_struct_description *description);
