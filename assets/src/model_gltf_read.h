// What the parts of the glTF reader share, internal to this folder: the state
// of one read, the helpers more than one part calls, and the one call each part
// makes. model_gltf.c holds the helpers' bodies and the entry point; the parts
// are model_gltf_materials.c, model_gltf_meshes.c and model_gltf_nodes.c.
//
// THE SPLIT IS BY WHAT IS READ, BECAUSE THAT IS HOW THE READER IS CHANGED. A
// change to how a material reads touches the materials and nothing else; a
// change to how a primitive reads touches the meshes. Each part checks what it
// reads with the helpers below, so every rule model_gltf.c's header states —
// lengths checked, no recursion, the one buffer — holds in every part by
// construction rather than by each part restating it.
//
// EVERY REFUSAL IS ONE LINE, IN ONE OF TWO SHAPES (ADR-0445 point 2). A damaged
// file, voe_assets_gltf_malformed: the sentence that it is damaged and to export
// it again from Blender, then `(glTF: <words>)`. A file this reader will not
// read, voe_assets_gltf_unsupported: the artist's sentence with its numbers and
// what to do in Blender, then `(glTF: <words>)`. The step that meets the cause
// says it; its callers only return false.
//
// A SUBJECT IS PUSHED BY THE PART THAT READS A THING: "the material `leaf`" or
// "picture 2" while that one is read, so a line below names it and never has to.
// Two levels at most, a part and then a material within it.
#pragma once

#include "json.h"

#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// glTF's component types, which are OpenGL's enumerants.
#define COMPONENT_BYTE 5120
#define COMPONENT_UNSIGNED_BYTE 5121
#define COMPONENT_SHORT 5122
#define COMPONENT_UNSIGNED_SHORT 5123
#define COMPONENT_UNSIGNED_INT 5125
#define COMPONENT_FLOAT 5126

struct gltf {
	const voe_assets_json *json;
	const uint8_t *bin;
	size_t bin_size;
	voe_base_arena *arena;

	// The arrays at the top of the document, or VOE_ASSETS_JSON_NONE. Read
	// once, because every lookup below needs one of them.
	uint32_t accessors;
	uint32_t views;
	uint32_t buffers;
	uint32_t materials;
	uint32_t textures;
	uint32_t images;
	uint32_t meshes;
	uint32_t nodes;
	uint32_t scenes;

	// The subjects pushed on base/report.h's stack, which keeps them by
	// pointer: one a level, the words around a name included.
	char subjects[2][VOE_ASSETS_MODEL_NAME + 32];
	uint32_t subject_count;

	voe_base_error error;
};

// Where an accessor's elements actually are: the first byte of the first one,
// how far apart they are, how many there are and what each component is.
//
// EVERYTHING IN HERE HAS BEEN CHECKED AGAINST THE BINARY CHUNK BEFORE IT IS
// FILLED IN. A caller may walk `count` elements at `stride` from `base` without
// checking anything itself, and that is the whole point of resolving an accessor
// in one place.
struct view {
	const uint8_t *base;
	size_t stride;
	uint32_t count;
	uint32_t component;
	uint32_t components;
};

// A damaged file: its line, printf over the format's words, and the error kept
// for the caller; always false.
[[gnu::format(printf, 2, 3)]]
bool voe_assets_gltf_malformed(struct gltf *gltf, const char *format_words,
			       ...);

// A file this reader will not read: its line, printf over the artist's
// sentence, the format's words after it; always false.
[[gnu::format(printf, 3, 4)]]
bool voe_assets_gltf_unsupported(struct gltf *gltf, const char *format_words,
				 const char *artist, ...);

// Pushes "the <kind> `name`", or "<kind> <index>" when `name` is NULL or "", as
// the report's subject until the matching pop. Push, read, pop, then return the
// read's answer, so a pushed subject is popped on every path.
void voe_assets_gltf_subject_push(struct gltf *gltf, const char *kind,
				  const voe_assets_model_name *name,
				  uint32_t index);
void voe_assets_gltf_subject_pop(struct gltf *gltf);

// A member that has to be an array if it is there at all; count is nought when
// it is not there.
[[nodiscard]] bool voe_assets_gltf_array_member(struct gltf *gltf,
						uint32_t object,
						const char *name,
						uint32_t *token,
						uint32_t *count);

// An object out of one of the document's arrays, by an index already checked
// against the array's length; VOE_ASSETS_JSON_NONE when it is not an object.
uint32_t voe_assets_gltf_object_at(struct gltf *gltf, uint32_t array,
				   uint32_t index);

// An unsigned member with a limit, and `fallback` when it is absent.
[[nodiscard]] bool voe_assets_gltf_uint_member(struct gltf *gltf,
					       uint32_t object,
					       const char *name, uint32_t limit,
					       uint32_t fallback,
					       uint32_t *out);

// A number member, and `fallback` when it is absent.
[[nodiscard]] bool voe_assets_gltf_float_member(struct gltf *gltf,
						uint32_t object,
						const char *name,
						float fallback, float *out);

// A fixed-length array of numbers; absent leaves `out` as the caller set it.
[[nodiscard]] bool voe_assets_gltf_floats_member(struct gltf *gltf,
						 uint32_t object,
						 const char *name,
						 uint32_t length, float *out,
						 bool *found);

// The one buffer allowed, the container's binary chunk, by index.
[[nodiscard]] bool voe_assets_gltf_check_buffer(struct gltf *gltf,
						uint32_t index);

// An accessor by index, checked against the binary chunk into `out`.
[[nodiscard]] bool voe_assets_gltf_resolve_accessor(struct gltf *gltf,
						    uint32_t index,
						    struct view *out);

// Element `index`'s components as floats; float accessors only.
[[nodiscard]] bool voe_assets_gltf_read_floats(struct gltf *gltf,
					       const struct view *view,
					       uint32_t index, float *out);

// Element `index` of an unsigned byte, short or int accessor, widened.
uint32_t voe_assets_gltf_read_index(const struct view *view, uint32_t index);

// An object's `name` into `out`, "" when absent, cut at a whole character
// (0247); not a string is malformed. In model_gltf_materials.c.
[[nodiscard]] bool voe_assets_gltf_read_name(struct gltf *gltf, uint32_t object,
					     voe_assets_model_name *out);

// The materials, with their textures resolved to images. In
// model_gltf_materials.c.
[[nodiscard]] bool voe_assets_gltf_read_materials(struct gltf *gltf,
						  voe_assets_model *model);

// The images, decoded. In model_gltf_materials.c.
[[nodiscard]] bool voe_assets_gltf_read_images(struct gltf *gltf,
					       voe_assets_model *model);

// The meshes and their primitives, after the materials. In
// model_gltf_meshes.c.
[[nodiscard]] bool voe_assets_gltf_read_meshes(struct gltf *gltf,
					       voe_assets_model *model);

// The node tree and its roots, after the meshes. In model_gltf_nodes.c.
[[nodiscard]] bool voe_assets_gltf_read_nodes(struct gltf *gltf,
					      voe_assets_model *model);
