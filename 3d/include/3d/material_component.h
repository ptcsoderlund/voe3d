// How an entity's surface is shaded: the factors glTF gives, one texture id per
// texture the material references, and the id of the record `render` holds for
// it. Read by anyone, const.
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
// with them. See voe_render_texture_kind.
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
#include <ecs/component.h>
#include <ecs/world.h>
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

	voe_render_texture base_colour_texture;
	voe_render_texture metallic_roughness_texture;
	voe_render_texture normal_texture;
	voe_render_texture occlusion_texture;
	voe_render_texture emissive_texture;

	// The record `render` holds for the numbers above, and the number a
	// drawn object's record carries. Filled by voe_3d_material_upload; a
	// component whose shading was never uploaded is not drawn.
	voe_render_shading shading;
} voe_3d_material;

extern const struct voe_ecs_key voe_3d_material_key;

void voe_3d_material_register(voe_ecs_world *world, uint32_t capacity);

// Hands the factors and the texture ids to `render` as one shading record and
// writes the id it comes back with into `material->shading`.
//
// CALLED ONCE PER MATERIAL AND NOT ONCE PER ENTITY. Two entities wearing one
// material want one record: upload it, then add the same component to both.
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
