// The material file behind assets/material.h: the sectioned reader for the
// lines, each value turned into a number, a word or a path here, and one
// measured print for the way back.
//
// THE READER MAKES ITS OWN SCRATCH ARENA. The signature takes none, since the
// result is a plain struct; the sectioned reader's copy of the text goes on an
// arena made and destroyed inside the call, one malloc per read. A file is a
// few hundred bytes read on open and after an Assets command, so that is
// nothing; an arena parameter would lift it if a caller ever read in a loop.
//
// PATHS ARE ALWAYS WRITTEN QUOTED, with `"` and `\` escaped as the sectioned
// reader takes them back (0149), so a path with blanks at its ends, a leading
// quote or a backslash returns as it went.
#include <assets/material.h>
#include <assets/sectioned.h>

#include <base/assert.h>
#include <base/report.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// One escaped path: every byte doubled at worst, two quotes and a NUL.
#define QUOTED_PATH (2 * VOE_ASSETS_MATERIAL_PATH + 3)

static bool refuse(voe_base_error *error, const char *key, const char *message)
{
	VOE_BASE_ERROR("assets", "material: %s: %s", key, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

// A finite float at `at`, blanks before it allowed. Returns the byte after
// it, or NULL.
static const char *number(const char *at, float *value)
{
	char *end;
	const float parsed = strtof(at, &end);

	if (end == at || !isfinite(parsed))
		return NULL;
	*value = parsed;
	return end;
}

// `count` floats separated by blanks and nothing after them, into `values`;
// a missing key leaves `values` as they are.
static bool read_numbers(const voe_assets_sectioned *doc, uint32_t section,
			 const char *key, float *values, uint32_t count,
			 voe_base_error *error)
{
	const char *at = voe_assets_sectioned_value(doc, section, key);

	if (at == NULL)
		return true;
	for (uint32_t i = 0; i < count; i++) {
		at = number(at, &values[i]);
		if (at == NULL || (*at != '\0' && *at != ' ' && *at != '\t'))
			return refuse(error, key, count == 1 ?
						  "not a number" :
						  "not three numbers");
	}
	while (*at == ' ' || *at == '\t')
		at++;
	if (*at != '\0')
		return refuse(error, key, "too many numbers");
	return true;
}

static bool read_path(const voe_assets_sectioned *doc, uint32_t section,
		      const char *key, char *path, voe_base_error *error)
{
	const char *value = voe_assets_sectioned_value(doc, section, key);

	if (value == NULL)
		return true;
	if (strlen(value) >= VOE_ASSETS_MATERIAL_PATH)
		return refuse(error, key, "a path of 128 bytes or more");
	strcpy(path, value);
	return true;
}

static bool read_shader(const voe_assets_sectioned *doc, uint32_t section,
			voe_assets_material_shader *shader,
			voe_base_error *error)
{
	const char *word = voe_assets_sectioned_value(doc, section, "shader");

	if (word == NULL)
		return true;
	if (strcmp(word, "lit") == 0)
		*shader = VOE_ASSETS_MATERIAL_LIT;
	else if (strcmp(word, "unlit") == 0)
		*shader = VOE_ASSETS_MATERIAL_UNLIT;
	else
		return refuse(error, "shader", "not lit or unlit");
	return true;
}

static bool read_section(const voe_assets_sectioned *doc,
			 voe_assets_material *material, voe_base_error *error)
{
	const uint32_t section = voe_assets_sectioned_find(doc, "Material");

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return refuse(error, "[Material]", "no such section");
	return read_shader(doc, section, &material->shader, error) &&
	       read_numbers(doc, section, "colour", material->colour, 3,
			    error) &&
	       read_numbers(doc, section, "roughness", &material->roughness, 1,
			    error) &&
	       read_numbers(doc, section, "metal", &material->metal, 1,
			    error) &&
	       read_numbers(doc, section, "repeat", &material->repeat, 1,
			    error) &&
	       read_path(doc, section, "colour_map", material->colour_map,
			 error) &&
	       read_path(doc, section, "normal_map", material->normal_map,
			 error) &&
	       read_path(doc, section, "roughness_map",
			 material->roughness_map, error);
}

voe_assets_material voe_assets_material_default(void)
{
	return (voe_assets_material){
		.shader = VOE_ASSETS_MATERIAL_LIT,
		.colour = { 1.0f, 1.0f, 1.0f },
		.roughness = 0.5f,
		.metal = 0.0f,
		.repeat = 1.0f,
	};
}

bool voe_assets_material_read(const char *text, size_t size,
			      voe_assets_material *out, voe_base_error *error)
{
	voe_assets_material material = voe_assets_material_default();
	voe_base_arena *scratch;
	voe_assets_sectioned doc;
	bool read;

	VOE_BASE_ASSERT(text != NULL, "reading a material from nothing");
	VOE_BASE_ASSERT(out != NULL, "reading a material into nothing");

	scratch = voe_base_arena_new(4096);
	read = voe_assets_sectioned_parse(text, size, scratch, &doc);
	if (!read)
		refuse(error, "text", "not sectioned text");
	else
		read = read_section(&doc, &material, error);
	voe_base_arena_destroy(scratch);
	if (read)
		*out = material;
	return read;
}

// `path` between quotes with `"` and `\` escaped, into `quoted`.
static void quote(const char *path, char *quoted)
{
	size_t at = 0;

	VOE_BASE_ASSERT(memchr(path, '\0', VOE_ASSETS_MATERIAL_PATH) != NULL,
			"a material path not terminated in its room");
	quoted[at++] = '"';
	for (size_t i = 0; path[i] != '\0'; i++) {
		VOE_BASE_ASSERT(path[i] != '\n' && path[i] != '\r',
				"a material path with a line break");
		if (path[i] == '"' || path[i] == '\\')
			quoted[at++] = '\\';
		quoted[at++] = path[i];
	}
	quoted[at++] = '"';
	quoted[at] = '\0';
	VOE_BASE_ASSERT(at < QUOTED_PATH, "a quoted path outgrew its room");
}

// The paths, quoted once and printed twice.
struct quoted_paths {
	char colour_map[QUOTED_PATH];
	char normal_map[QUOTED_PATH];
	char roughness_map[QUOTED_PATH];
};

// The whole text into `buffer`, or only its length when `buffer` is NULL.
static size_t print_material(char *buffer, size_t room,
			     const voe_assets_material *material,
			     const struct quoted_paths *paths)
{
	const int written = snprintf(
		buffer, room,
		"[Material]\nshader=%s\ncolour=%.9g %.9g %.9g\n"
		"roughness=%.9g\nmetal=%.9g\nrepeat=%.9g\ncolour_map=%s\n"
		"normal_map=%s\nroughness_map=%s\n",
		material->shader == VOE_ASSETS_MATERIAL_UNLIT ? "unlit" : "lit",
		(double)material->colour[0], (double)material->colour[1],
		(double)material->colour[2], (double)material->roughness,
		(double)material->metal, (double)material->repeat,
		paths->colour_map, paths->normal_map, paths->roughness_map);

	VOE_BASE_ASSERT(written > 0, "printing a material failed");
	VOE_BASE_ASSERT(buffer == NULL || (size_t)written < room,
			"the material text was measured by this same print");
	return (size_t)written;
}

voe_assets_material_text
voe_assets_material_write(const voe_assets_material *material,
			  voe_base_arena *arena)
{
	struct quoted_paths paths;
	size_t size;
	char *text;

	VOE_BASE_ASSERT(material != NULL, "writing no material");
	VOE_BASE_ASSERT(arena != NULL, "writing a material with no arena");

	quote(material->colour_map, paths.colour_map);
	quote(material->normal_map, paths.normal_map);
	quote(material->roughness_map, paths.roughness_map);
	size = print_material(NULL, 0, material, &paths);
	text = voe_base_arena_push(arena, size + 1);
	print_material(text, size + 1, material, &paths);
	return (voe_assets_material_text){ .text = text, .size = size };
}
