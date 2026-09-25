// One field's value read from a scene's text into a row — used by
// scene_read.c's first pass, once for each key a section holds.
//
// A REFERENCE IS HELD, NOT RESOLVED. An ENTITY element is read as an authored
// id; one the file holds is appended to the caller's patches, one it does not
// hold is loaded as no entity with a warning. The entity it names does not
// exist until the caller's second pass, which writes it through the patch.
//
// EVERY REFUSAL IS REPORTED HERE, naming the site the caller gives, and the
// row may be partly written when it is.
#pragma once

#include "authored.h"

#include <base/describe.h>

#include <stdint.h>

// Where a value sits, for a report.
typedef struct {
	uint32_t line;
	const char *section;
	const char *field;
} voe_authoring_site;

// An ENTITY element, held as an authored id until the entity exists. Only ids
// the file holds are held.
typedef struct {
	uint8_t *bytes;
	uint64_t id;
} voe_authoring_patch;

// What a reference is checked against and held in: the file's `[N]`s,
// ascending by id, and room for every ENTITY element its rows could hold.
typedef struct {
	voe_authoring_authored *authored;
	uint32_t authored_count;

	voe_authoring_patch *patches;
	uint32_t patch_count;
} voe_authoring_field_refs;

// Reads `value`, as the sectioned reader handed it back, into `field`'s bytes
// of `row`. False, reported, when the text is not the field's kind and shape.
[[nodiscard]] bool
voe_authoring_field_value(voe_authoring_field_refs *refs,
			  const voe_authoring_site *site,
			  const voe_base_field_description *field,
			  const char *value, uint8_t *row);
