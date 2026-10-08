// The landscape cook: heights printed as one int32_t array
// (authoring/landscape_cook.h).
//
// The text grows through cook_text.h's buffer, which needs only the arena; the
// world-walking parts of that cook are not used. Eight numbers to a line.
#include <authoring/landscape_cook.h>

#include "cook_text.h"

#include <base/assert.h>

#include <ctype.h>
#include <math.h>
#include <stdint.h>

#define NUMBERS_PER_LINE 8

static bool is_identifier(const char *name)
{
	if (name[0] == '\0' || isdigit((unsigned char)name[0]))
		return false;
	for (const char *p = name; *p != '\0'; p++)
		if (!isalnum((unsigned char)*p) && *p != '_')
			return false;
	return true;
}

voe_authoring_text
voe_authoring_landscape_cook(const voe_assets_landscape *landscape,
			     const char *name, voe_base_arena *arena)
{
	VOE_BASE_ASSERT(landscape != NULL && landscape->heights != NULL,
			"cooking no landscape");
	VOE_BASE_ASSERT(name != NULL && is_identifier(name),
			"a landscape array named by a C identifier");
	VOE_BASE_ASSERT(arena != NULL, "cooking a landscape with no arena");
	const size_t count = (size_t)(landscape->cells + 1) *
			     (landscape->cells + 1);
	voe_authoring_cook c = { .arena = arena };

	voe_authoring_cook_putf(&c, "static const int32_t ");
	voe_authoring_cook_put(&c, name);
	voe_authoring_cook_putf(&c, "[%zu] = {", count);
	for (size_t i = 0; i < count; i++) {
		// Adding 0.0 turns a rounded -0 into 0, as the file's writer does.
		double millimetres =
			round((double)landscape->heights[i] * 1000.0) + 0.0;

		VOE_BASE_ASSERT(millimetres > (double)INT32_MIN &&
					millimetres <= (double)INT32_MAX,
				"a height that fits an int32_t in millimetres");
		voe_authoring_cook_put(&c, i % NUMBERS_PER_LINE == 0 ? "\n\t" :
								     " ");
		voe_authoring_cook_putf(&c, "%.0f,", millimetres);
	}
	voe_authoring_cook_put(&c, "\n};\n");
	return (voe_authoring_text){ .text = c.bytes, .size = c.size };
}
