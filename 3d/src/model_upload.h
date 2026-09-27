// A read model's pictures and materials handed to `render`: every picture
// uploaded once per colour space it is wanted in, one uploaded material per glTF
// material, and the default one for a primitive that names none. Internal to 3d:
// import.c and the model store both start a model with it.
//
//     voe_3d_model_upload upload;
//
//     if (!voe_3d_model_upload_create(device, arena, &model, &upload, &error))
//             ... // upload.textures and upload.shadings are what was made
//     material = voe_3d_model_upload_material(&upload, primitive->material);
//
// IT MOVED OUT OF import.c so that the store (ADR-0277 point 2) uploads a model
// the same way the import does rather than a second way that drifts; nothing it
// does changed in the move.
//
// A PICTURE IS UPLOADED ONCE PER COLOUR SPACE AND NOT ONCE. A texture slot holds
// one format, and a base colour map has to be sRGB while an ORM map has to be raw
// (render/device.h) — so a file whose one picture is referenced as both costs
// two slots, short of two views onto one image, which is a card of its own.
// Nothing real does this; what the two arrays buy is that such a file is merely
// wasteful rather than wrong.
//
// WHICH SPACE A PICTURE IS IN COMES OUT OF THE MATERIALS, because nothing in a
// PNG says whether its bytes are a colour, so the materials are walked for their
// texture indices before anything is uploaded, then again for the rest. `render`
// deduplicates nothing: `assets` resolved every glTF texture to a picture, so
// every material wanting a picture one way round gets the same id.
//
// EVERY ID IT MADE IS LISTED, textures and shading records both, so a caller
// that replaces the model (a re-export, 0277 point 5) can free all of them. The
// lists are kept true as it goes, so after a failure they are what was made
// before it. Everything is pushed into the caller's arena, which must outlive
// the answer.
#pragma once

#include <3d/material_component.h>

#include <assets/model.h>

#include <base/arena.h>
#include <base/error.h>

#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	// One per glTF material, in the file's order, each already uploaded.
	voe_3d_material *materials;
	uint32_t material_count;
	// glTF's default material, uploaded only when some primitive names
	// none; `has_default` says whether it was.
	voe_3d_material default_material;
	bool has_default;

	voe_render_texture *textures;
	uint32_t texture_count;
	voe_render_shading *shadings;
	uint32_t shading_count;
} voe_3d_model_upload;

// Uploads `model`'s pictures and materials through `device` into `out`, its
// arrays in `arena`. False when `render` has no room, with `error` set and
// `out` listing what was made before.
[[nodiscard]] bool voe_3d_model_upload_create(voe_render_device *device,
					      voe_base_arena *arena,
					      const voe_assets_model *model,
					      voe_3d_model_upload *out,
					      voe_base_error *error);

// The material a primitive naming `material` wears: its glTF material, or the
// default one for VOE_ASSETS_MODEL_NONE and for an index past the file's.
voe_3d_material voe_3d_model_upload_material(const voe_3d_model_upload *upload,
					     uint32_t material);
