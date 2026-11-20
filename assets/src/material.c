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
// ONE BODY READS FOR BOTH READERS; the strict one then walks every section and
// key against the table of keys read, so a key added to the reader and not to
// the table is refused by the strict reader alone.
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

static bool refuse_line(voe_base_error *error, uint32_t line, const char *name,
			const char *message)
{
	VOE_BASE_ERROR("assets", "material: line %u: %s: %s", line, name,
		       message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

// Every key read_section reads, then the retired map keys, which read as no
// maps (0400).
static const char *const keys_taken[] = {
	"shader",	"colour",	"roughness",	     "metal",
	"repeat",	"colormap",	"normalmap",	     "ormmap",
	"cutout",	"double_sided", "emission",	     "emission_strength",
	"emissionmap",	"colour_map",	"normal_map",	     "roughness_map",
};

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
	size_t length;

	if (value == NULL)
		return true;
	length = strlen(value);
	if (length >= VOE_ASSETS_MATERIAL_PATH)
		return refuse(error, key, "a path of 128 bytes or more");
	memcpy(path, value, length + 1);
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

static bool read_double_sided(const voe_assets_sectioned *doc,
			      uint32_t section, bool *double_sided,
			      voe_base_error *error)
{
	const char *word =
		voe_assets_sectioned_value(doc, section, "double_sided");

	if (word == NULL)
		return true;
	if (strcmp(word, "true") == 0)
		*double_sided = true;
	else if (strcmp(word, "false") == 0)
		*double_sided = false;
	else
		return refuse(error, "double_sided", "not true or false");
	return true;
}

// The ranges a parsed number must also be in; a missing key holds its
// default, which is in range.
static bool check_ranges(const voe_assets_material_file *material,
			 voe_base_error *error)
{
	if (!(material->cutout >= 0.0f && material->cutout <= 1.0f))
		return refuse(error, "cutout", "outside 0 to 1");
	for (uint32_t i = 0; i < 3; i++)
		if (material->emission[i] < 0.0f)
			return refuse(error, "emission", "a negative channel");
	if (material->emission_strength < 0.0f)
		return refuse(error, "emission_strength", "negative");
	return true;
}

static bool read_section(const voe_assets_sectioned *doc,
			 voe_assets_material_file *material, voe_base_error *error)
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
	       read_path(doc, section, "colormap", material->colormap, error) &&
	       read_path(doc, section, "normalmap", material->normalmap,
			 error) &&
	       read_path(doc, section, "ormmap", material->ormmap, error) &&
	       read_numbers(doc, section, "cutout", &material->cutout, 1,
			    error) &&
	       read_double_sided(doc, section, &material->double_sided,
				 error) &&
	       read_numbers(doc, section, "emission", material->emission, 3,
			    error) &&
	       read_numbers(doc, section, "emission_strength",
			    &material->emission_strength, 1, error) &&
	       read_path(doc, section, "emissionmap", material->emissionmap,
			 error) &&
	       check_ranges(material, error);
}

static bool key_taken(const char *name)
{
	for (size_t i = 0; i < sizeof(keys_taken) / sizeof(keys_taken[0]); i++)
		if (strcmp(name, keys_taken[i]) == 0)
			return true;
	return false;
}

// Every section `[Material]` and each of its keys one in keys_taken.
static bool check_strict(const voe_assets_sectioned *doc, voe_base_error *error)
{
	for (uint32_t s = 0; s < doc->section_count; s++) {
		const voe_assets_sectioned_section *section = &doc->sections[s];

		if (strcmp(section->name, "Material") != 0)
			return refuse_line(error, section->line, section->name,
					   "a section other than [Material]");
		for (uint32_t i = 0; i < section->key_count; i++) {
			const voe_assets_sectioned_key *key =
				&doc->keys[section->first_key + i];

			if (!key_taken(key->name))
				return refuse_line(error, key->line, key->name,
						   "a key the reader does not take");
		}
	}
	return true;
}

voe_assets_material_file voe_assets_material_default(void)
{
	return (voe_assets_material_file){
		.shader = VOE_ASSETS_MATERIAL_LIT,
		.colour = { 1.0f, 1.0f, 1.0f },
		.roughness = 0.5f,
		.metal = 0.0f,
		.repeat = 1.0f,
		.cutout = 0.0f,
		.double_sided = false,
		.emission = { 0.0f, 0.0f, 0.0f },
		.emission_strength = 1.0f,
	};
}

static bool read_material(const char *text, size_t size,
			  voe_assets_material_file *out, voe_base_error *error,
			  bool strict)
{
	voe_assets_material_file material = voe_assets_material_default();
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
		read = read_section(&doc, &material, error) &&
		       (!strict || check_strict(&doc, error));
	voe_base_arena_destroy(scratch);
	if (read)
		*out = material;
	return read;
}

bool voe_assets_material_read(const char *text, size_t size,
			      voe_assets_material_file *out, voe_base_error *error)
{
	return read_material(text, size, out, error, false);
}

bool voe_assets_material_read_strict(const char *text, size_t size,
				     voe_assets_material_file *out,
				     voe_base_error *error)
{
	return read_material(text, size, out, error, true);
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
	char colormap[QUOTED_PATH];
	char normalmap[QUOTED_PATH];
	char ormmap[QUOTED_PATH];
	char emissionmap[QUOTED_PATH];
};

// The whole text into `buffer`, or only its length when `buffer` is NULL.
static size_t print_material(char *buffer, size_t room,
			     const voe_assets_material_file *material,
			     const struct quoted_paths *paths)
{
	const int written = snprintf(
		buffer, room,
		"[Material]\nshader=%s\ncolour=%.9g %.9g %.9g\n"
		"roughness=%.9g\nmetal=%.9g\nrepeat=%.9g\ncolormap=%s\n"
		"normalmap=%s\normmap=%s\ncutout=%.9g\ndouble_sided=%s\n"
		"emission=%.9g %.9g %.9g\nemission_strength=%.9g\n"
		"emissionmap=%s\n",
		material->shader == VOE_ASSETS_MATERIAL_UNLIT ? "unlit" : "lit",
		(double)material->colour[0], (double)material->colour[1],
		(double)material->colour[2], (double)material->roughness,
		(double)material->metal, (double)material->repeat,
		paths->colormap, paths->normalmap, paths->ormmap,
		(double)material->cutout,
		material->double_sided ? "true" : "false",
		(double)material->emission[0], (double)material->emission[1],
		(double)material->emission[2],
		(double)material->emission_strength, paths->emissionmap);

	VOE_BASE_ASSERT(written > 0, "printing a material failed");
	VOE_BASE_ASSERT(buffer == NULL || (size_t)written < room,
			"the material text was measured by this same print");
	return (size_t)written;
}

voe_assets_material_text
voe_assets_material_write(const voe_assets_material_file *material,
			  voe_base_arena *arena)
{
	struct quoted_paths paths;
	size_t size;
	char *text;

	VOE_BASE_ASSERT(material != NULL, "writing no material");
	VOE_BASE_ASSERT(arena != NULL, "writing a material with no arena");

	quote(material->colormap, paths.colormap);
	quote(material->normalmap, paths.normalmap);
	quote(material->ormmap, paths.ormmap);
	quote(material->emissionmap, paths.emissionmap);
	size = print_material(NULL, 0, material, &paths);
	text = voe_base_arena_push(arena, size + 1);
	print_material(text, size + 1, material, &paths);
	return (voe_assets_material_text){ .text = text, .size = size };
}
