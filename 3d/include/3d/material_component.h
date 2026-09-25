// How an entity's surface is shaded: the factors glTF gives, one texture id per
// texture the material references, and the id of the record `render` holds for
// it. Read by anyone, const.
//
// UNLESS IT IS UNLIT, IN WHICH CASE NONE OF THAT HAPPENS. `unlit` skips the
// whole shading model and writes the base colour as it is. It is a property of
// the surface and not of a pass, so it changes nothing about which draw the
// entity goes through — see render/include/render/device.h.
//
// BASE COLOUR, METALNESS, ROUGHNESS AND OCCLUSION ARE ALL SHADED WITH; EMISSION
// AND THE NORMAL MAP ARE STORED AND NOT READ. Card 019 lit the engine and reads
// the first four, factors and pictures both. The other two are still what card
// 018 made them — parsed, uploaded and waiting — because a normal map needs
// tangents the importer does not produce and emission without tone mapping is a
// colour that clips. Storing what the file said is not the same as pretending to
// shade with it.
//
// A TEXTURE ID HERE IS ALREADY IN THE RIGHT COLOUR SPACE. The base colour and
// emissive ones name pictures uploaded as sRGB and the other three name pictures
// uploaded raw; that is the importer's doing and there is nothing to say about it
// at this level except that swapping two of these ids swaps two colour spaces
// with them. See voe_render_texture_kind. The one exception is a base colour
// slot holding a distance field, which is numbers and goes up raw —
// `base_colour_distance_field` below is what says so.
//
// AND THE BASE COLOUR SLOT IS THE ONE MAP READ THROUGH A RECTANGLE. A material
// says which part of its base colour texture it reads, which is what lets a
// sheet of frames sit behind one geometry and one texture — see
// `base_colour_uv_offset` below. Nothing says it about the other four maps: a
// picture on a surface is read at the mesh's own coordinates.
//
// THE ALPHA MODE IS `render`'s ENUM AND NOT A SECOND ONE. This component already
// holds `render`'s texture and shading ids, so it holds `render`'s spelling of
// the three words as well; `assets` has its own because that folder may not name
// `render`, and 3d/src/import.c is the one place the two are mapped. A zeroed
// material is opaque, which is what a material nobody said anything about should
// be.
//
// AND THE MODE DECIDES WHICH PASS THE DRAW SYSTEM PUTS THE ENTITY IN. Opaque and
// cutout go through the ordinary draw in table order; blended goes through the
// blended one, furthest first. See 3d/draw_system.h.
//
// A MISSING TEXTURE IS AN ID AND NOT A NULL. VOE_RENDER_NO_TEXTURE names the
// one-pixel white default, so a shader may test it and gets white either way —
// which means a material with no picture on it is exactly its factors.
//
// THE SAME MATERIAL ON TWO ENTITIES IS TWO COPIES OF THIS AND ONE GPU RECORD.
// The component is per entity because a component table is; `shading` is the row
// `render` holds, and two entities sharing a material share that number. The
// same is true one level down: two materials referencing one picture hold the
// same texture id, which is the whole of how a file's textures are deduplicated.
//
// NO SYSTEM AND NO INTENT YET, for the same reason mesh has none: nothing
// changes a material after it is built. A card that fades or tints something is
// the card that gives this module a system — and it will want to think about the
// GPU record, which is written once today.
#pragma once

#include <base/error.h>
#include <base/imported.h>
#include <ecs/component.h>
#include <ecs/world.h>
#include <math/float2.h>
#include <math/float3.h>
#include <math/float4.h>
#include <render/device.h>

#include <stdint.h>

typedef struct {
	// Multiplied by whatever the base colour texture says, which is what
	// makes an untextured material just this colour.
	voe_math_float4 base_colour;
	float metallic;
	float roughness;
	voe_math_float3 emissive;

	// What the base colour's fourth channel means, and the cutoff a cutout
	// material is tested against. The cutoff is carried whatever the mode is
	// — it is what the file said — and only cutout reads it.
	voe_render_alpha_mode alpha_mode;
	float alpha_cutoff;

	// Not lit: the base colour, times its texture, and no sun. Text is what
	// asked for it and a user interface will want the same thing. It is
	// independent of the alpha mode — an unlit surface may be opaque,
	// cutout or blended — and it is not set by the importer, because
	// nothing in `assets` reads glTF's unlit extension yet; a material that
	// wants it is one a call site built.
	bool unlit;

	voe_render_texture base_colour_texture;
	// The base colour texture is not a picture: it holds three signed
	// distances per texel, and the fragment stage computes alpha from their
	// median rather than sampling it. The base colour factor is then exactly
	// the colour on screen. Text is what asked for it and a user interface
	// will want the same thing; the importer never sets it, because no glTF
	// carries a distance field.
	//
	// A SHEET THAT SAYS THIS ALSO HAS TO HAVE BEEN CREATED SHARP. See
	// voe_render_sampling — a sheet addressed REPEAT can fetch across its
	// own border into a different glyph, and nothing checks that the two
	// agree.
	bool base_colour_distance_field;

	// Which rectangle of the base colour texture this material reads. The
	// mesh's own coordinates are multiplied by the scale and the offset is
	// added, so a quad whose UVs run 0..1 and a material saying
	// offset (0.25, 0) scale (0.25, 0.5) reads the second cell of a
	// four-by-two grid. It applies to the base colour and to nothing else.
	//
	// A SHEET OF FRAMES IS ONE GEOMETRY, ONE TEXTURE AND ONE MATERIAL PER
	// FRAME. A static geometry cannot change once it is created, so a mesh
	// carrying its frame in its UVs is a sprite that can never change frame; a
	// material can be swapped for the price of pointing an entity at a
	// different record. See sprite/sheet.h, which is what builds them.
	//
	// A SCALE OF NOTHING MEANS THE WHOLE TEXTURE, which is what makes every
	// material written before this field existed still read its whole
	// picture. voe_3d_material_upload is where that is applied, and it
	// writes the answer back here — see its comment.
	voe_math_float2 base_colour_uv_offset;
	voe_math_float2 base_colour_uv_scale;

	voe_render_texture metallic_roughness_texture;
	voe_render_texture normal_texture;
	voe_render_texture occlusion_texture;
	voe_render_texture emissive_texture;

	// The record `render` holds for the numbers above, and the number a
	// drawn object's record carries. Filled by voe_3d_material_upload; a
	// component whose shading was never uploaded is not drawn.
	voe_render_shading shading;
} voe_3d_material;

extern VOE_BASE_IMPORTED const struct voe_ecs_key voe_3d_material_key;

void voe_3d_material_register(voe_ecs_world *world, uint32_t capacity);

// Hands the factors and the texture ids to `render` as one shading record and
// writes the id it comes back with into `material->shading`.
//
// CALLED ONCE PER MATERIAL AND NOT ONCE PER ENTITY. Two entities wearing one
// material want one record: upload it, then add the same component to both.
//
// IT IS ALSO WHERE A UV SCALE OF NOTHING BECOMES THE WHOLE TEXTURE, and it
// writes that back into `material` rather than only into the record. A component
// that read (0, 0) while its record read (1, 1) would be two answers to one
// question, and the component is the one a person looks at. So a material built
// by naming the fields it cares about — which is every material in this engine —
// comes out of here saying it reads its whole picture, because it does.
//
// False when `render` has no room for another record, which is the one way this
// fails. A startup operation, because creating a record is.
[[nodiscard]] bool voe_3d_material_upload(voe_render_device *device,
					  voe_3d_material *material,
					  voe_base_error *error);

// Gives the entity its material. False when the table is full or the entity is
// not alive.
[[nodiscard]] bool voe_3d_material_add(voe_ecs_world *world,
				       voe_ecs_entity entity,
				       voe_3d_material material);

// NULL when the entity has no material or is not alive.
const voe_3d_material *voe_3d_material_get(const voe_ecs_world *world,
					   voe_ecs_entity entity);

uint32_t voe_3d_material_count(const voe_ecs_world *world);
const voe_3d_material *voe_3d_material_rows(const voe_ecs_world *world);
const voe_ecs_entity *voe_3d_material_entities(const voe_ecs_world *world);
