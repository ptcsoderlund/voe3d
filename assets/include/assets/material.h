// A material file (`<name>.material`, ADR 0399 point 1): one engine shader and
// its values in sectioned text, read into a struct and written back.
//
//     [Material]
//     shader=lit
//     colour=1 1 1
//     roughness=0.5
//     metal=0
//     repeat=1
//     colormap="Assets/brick.png"
//     normalmap=""
//     ormmap=""
//
// `shader` is `lit` or `unlit`; `colour` is three linear floats; `repeat`
// multiplies the texture coordinates before any map is read. Each map is a
// project-relative path, "" for none. `ormmap` is one picture: occlusion in
// red, roughness in green, metal in blue (0400). A file with the keys these
// replaced (colour, normal and roughness, each `_map`) reads as having no maps.
//
// A MISSING KEY READS ITS DEFAULT: lit, colour 1 1 1, roughness 0.5, metal 0,
// repeat 1, no maps. A file written before a key existed still opens, and
// Create can write the defaults without this header growing a second writer.
// A key this reader does not know is passed over for the same reason.
//
// MAPS ARE PATHS, NOT IDS. A path is what the editor's rename and delete
// already follow (0378), what a person reads in the file, and what the cook
// and the game's folder lookup take; an id would need a table kept beside it.
//
// NOTHING HERE OPENS A FILE; `platform` owns files, and the caller hands the
// bytes in and writes the text out. The game never reads this (0236): the cook
// turns it into a table.
//
// THE STRUCT IS `voe_assets_material_file`: the file's values, apart from the
// glTF material in `model.h`, so one unit can include both headers.
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>

// The room of one map path, its NUL included.
#define VOE_ASSETS_MATERIAL_PATH 128

typedef enum {
	VOE_ASSETS_MATERIAL_LIT = 0,
	VOE_ASSETS_MATERIAL_UNLIT,
} voe_assets_material_shader;

typedef struct {
	voe_assets_material_shader shader;
	// Linear, not sRGB.
	float colour[3];
	float roughness;
	float metal;
	float repeat;
	// Project-relative, NUL-terminated, "" for none.
	char colormap[VOE_ASSETS_MATERIAL_PATH];
	char normalmap[VOE_ASSETS_MATERIAL_PATH];
	char ormmap[VOE_ASSETS_MATERIAL_PATH];
} voe_assets_material_file;

typedef struct {
	const char *text;
	size_t size;
} voe_assets_material_text;

// What a missing key reads as, and what Create writes.
voe_assets_material_file voe_assets_material_default(void);

// Reads `size` bytes of text, which need not be NUL-terminated. MALFORMED for
// text the sectioned reader refuses, no `[Material]` section, a number that
// does not parse, a shader that is not `lit` or `unlit`, or a path of
// VOE_ASSETS_MATERIAL_PATH bytes or more. A line on stderr names what. `out`
// untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_material_read(const char *text, size_t size,
					    voe_assets_material_file *out,
					    voe_base_error *error);

// The text voe_assets_material_read takes back exactly, NUL-terminated,
// `size` not counting the NUL, on `arena`. Floats are printed to nine digits,
// which a float round-trips through. Cannot fail; asserts each path is
// NUL-terminated in its room and holds no line break.
voe_assets_material_text
voe_assets_material_write(const voe_assets_material_file *material,
			  voe_base_arena *arena);
