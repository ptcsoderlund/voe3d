// voe_theme_read: the sectioned parse, then this folder's schema over the one
// section it must hold — every key looked up by name, converted, range-checked,
// and the result written to the caller only once all of it has passed. What a
// theme file looks like and every refusal are in include/theme/theme.h.
//
// Constraints. Keys are matched by a linear scan over a table of six names;
// a theme file has at most six keys, so nothing faster would be measurable.
// No render header is named here (ADR-0176).
#include <theme/theme.h>

#include <assets/sectioned.h>
#include <base/assert.h>
#include <base/report.h>

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// The keys this schema knows, in the order the header lists them.
enum key {
	KEY_ACCENT,
	KEY_CONTRAST_STRENGTH,
	KEY_SURFACE_SEPARATION,
	KEY_MODE,
	KEY_FONT,
	KEY_TEXT_SIZE,
	KEY_COUNT,
};

static const char *const KEY_NAMES[KEY_COUNT] = {
	"accent", "contrast_strength", "surface_separation",
	"mode",	  "font",	       "text_size",
};

// The first four are required; `font` and `text_size` fall back.
#define REQUIRED_KEYS 4

// A whole-string decimal number, finite. False for "", trailing text, "nan"
// and "inf".
static bool to_number(const char *text, float *out)
{
	char *end;
	double value;

	if (text[0] == '\0')
		return false;
	value = strtod(text, &end);
	if (*end != '\0' || !isfinite(value))
		return false;
	*out = (float)value;
	return true;
}

static int hex_digit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

// `#RRGGBB` exactly, into sRGB 0..1 per channel.
static bool to_accent(const char *text, voe_math_float3 *out)
{
	float channel[3];

	if (strlen(text) != 7 || text[0] != '#')
		return false;
	for (int i = 0; i < 3; i++) {
		int high = hex_digit(text[1 + 2 * i]);
		int low = hex_digit(text[2 + 2 * i]);

		if (high < 0 || low < 0)
			return false;
		channel[i] = (float)(high * 16 + low) / 255.0f;
	}
	*out = (voe_math_float3){ channel[0], channel[1], channel[2] };
	return true;
}

static bool to_scalar(const voe_assets_sectioned_key *key, float *out)
{
	if (!to_number(key->value, out)) {
		VOE_BASE_ERROR("theme", "line %u: %s '%s' is not a number",
			       key->line, key->name, key->value);
		return false;
	}
	if (*out < VOE_UI_THEME_SCALAR_MIN || *out > VOE_UI_THEME_SCALAR_MAX) {
		VOE_BASE_ERROR("theme", "line %u: %s %s is outside %g..%g",
			       key->line, key->name, key->value,
			       (double)VOE_UI_THEME_SCALAR_MIN,
			       (double)VOE_UI_THEME_SCALAR_MAX);
		return false;
	}
	return true;
}

// One key's value into `theme`. Reports and returns false when it will not
// convert or is out of range.
static bool convert(enum key which, const voe_assets_sectioned_key *key,
		    voe_theme *theme)
{
	const char *value = key->value;

	switch (which) {
	case KEY_ACCENT:
		if (to_accent(value, &theme->inputs.accent))
			return true;
		VOE_BASE_ERROR("theme", "line %u: accent '%s' is not #RRGGBB",
			       key->line, value);
		return false;
	case KEY_CONTRAST_STRENGTH:
		return to_scalar(key, &theme->inputs.contrast_strength);
	case KEY_SURFACE_SEPARATION:
		return to_scalar(key, &theme->inputs.surface_separation);
	case KEY_MODE:
		if (strcmp(value, "light") == 0) {
			theme->inputs.mode = VOE_UI_THEME_MODE_LIGHT;
			return true;
		}
		if (strcmp(value, "dark") == 0) {
			theme->inputs.mode = VOE_UI_THEME_MODE_DARK;
			return true;
		}
		VOE_BASE_ERROR("theme",
			       "line %u: mode '%s' is not light or dark",
			       key->line, value);
		return false;
	case KEY_FONT:
		if (strcmp(value, "oxanium") == 0) {
			theme->typeface = VOE_TEXT_TYPEFACE_OXANIUM;
			return true;
		}
		if (strcmp(value, "pixel_operator") == 0) {
			theme->typeface = VOE_TEXT_TYPEFACE_PIXEL_OPERATOR;
			return true;
		}
		VOE_BASE_ERROR("theme",
			       "line %u: font '%s' is not oxanium or "
			       "pixel_operator",
			       key->line, value);
		return false;
	case KEY_TEXT_SIZE:
		if (to_number(value, &theme->inputs.text_size) &&
		    theme->inputs.text_size > 0.0f)
			return true;
		VOE_BASE_ERROR("theme",
			       "line %u: text_size '%s' is not a number above "
			       "nought",
			       key->line, value);
		return false;
	case KEY_COUNT:
		break;
	}
	VOE_BASE_ASSERT(false, "convert: no such key");
	return false;
}

bool voe_theme_read(const char *text, size_t size, voe_base_arena *arena,
		    voe_theme *out)
{
	voe_assets_sectioned doc;
	const voe_assets_sectioned_section *section;
	voe_theme theme;
	bool seen[KEY_COUNT] = { false };

	VOE_BASE_ASSERT(text != NULL || size == 0, "voe_theme_read: no text");
	VOE_BASE_ASSERT(arena != NULL && out != NULL,
			"voe_theme_read: NULL arena or out");

	if (!voe_assets_sectioned_parse(text, size, arena, &doc))
		return false;
	if (doc.section_count == 0) {
		VOE_BASE_ERROR("theme", "line 1: no [section] in the file");
		return false;
	}
	if (doc.section_count > 1) {
		VOE_BASE_ERROR("theme",
			       "line %u: a second section; a theme file holds "
			       "exactly one",
			       doc.sections[1].line);
		return false;
	}

	section = &doc.sections[0];
	theme = (voe_theme){ .name = section->name,
			     .inputs = voe_ui_theme_default_inputs(),
			     .typeface = VOE_TEXT_TYPEFACE_OXANIUM };

	for (uint32_t i = 0; i < section->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&doc.keys[section->first_key + i];
		int which = 0;

		while (which < KEY_COUNT &&
		       strcmp(key->name, KEY_NAMES[which]) != 0)
			which++;
		if (which == KEY_COUNT) {
			VOE_BASE_ERROR("theme", "line %u: unknown key '%s'",
				       key->line, key->name);
			return false;
		}
		if (!convert((enum key)which, key, &theme))
			return false;
		seen[which] = true;
	}

	for (int which = 0; which < REQUIRED_KEYS; which++) {
		if (!seen[which]) {
			VOE_BASE_ERROR("theme",
				       "line %u: [%s] has no %s, which is "
				       "required",
				       section->line, section->name,
				       KEY_NAMES[which]);
			return false;
		}
	}

	*out = theme;
	return true;
}
