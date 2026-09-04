// From a `.glb`'s bytes to entities in a world. The one place in the engine that
// names `assets`, `render`, `scene` and `ecs` at once, which is why it is here
// and not in any of them.
//
//     voe_3d_import imported;
//     if (!voe_3d_import_glb(world, gpu, arena, MODEL, sizeof(MODEL),
//                            &imported, &error))
//             ...
//
// THE PIPELINE, AND EACH STEP BELONGS TO EXACTLY ONE FOLDER: `assets` parses the
// file into CPU arrays and knows nothing about entities or the GPU; this reads
// those arrays, uploads them through `render`, gets ids back, and writes
// components. File → assets → 3d → ids from render → components in tables.
//
// THE NODE TREE IS FLATTENED HERE. Every node that references a mesh becomes one
// entity per primitive of that mesh, and the entity's transform is the node's
// *world* transform — the tree composed once, with an explicit stack and a named
// depth limit (rule 14). No parent component is written, because there is not
// one: `scene`'s transform is a world transform and a hierarchy is a later card.
//
// A WORLD MATRIX IS DECOMPOSED BACK INTO POSITION, ROTATION AND SCALE, AND THAT
// IS WHERE THE ONE LOSS IN THIS FILE IS. The transform component holds three
// parts and a composed matrix is a matrix, so the parts are recovered from it:
// the translation is exact, the scale is the length of each basis vector, and
// the rotation is what is left. A file whose node transforms shear — a rotation
// under a non-uniform scale — loses the shear, and there is nothing to do about
// that short of a transform component that holds a matrix. It is written down
// here rather than discovered later.
//
// NO COORDINATE IS CONVERTED AND NONE MAY BE ADDED (ADR-0033). glTF's handedness
// is this engine's exactly. What is transposed is matrix *layout*, and that
// happens in `assets` before anything here sees it (ADR-0035).
//
// EVERY PICTURE IS UPLOADED ONCE PER COLOUR SPACE AND ITS ID GOES INTO EVERY
// MATERIAL THAT REFERENCES IT THAT WAY ROUND. That is the whole of the
// deduplication and it falls out of ids being values: `assets` has already
// resolved a glTF texture to a picture, so two materials over one picture hold
// the same texture id and the shader samples one slot.
//
// PER COLOUR SPACE, BECAUSE A COLOUR MAP AND A DATA MAP ARE DIFFERENT FORMATS.
// A base colour or emissive picture goes up as VOE_RENDER_TEXTURE_COLOUR and a
// metallic-roughness, normal or occlusion picture as VOE_RENDER_TEXTURE_DATA;
// which one a picture is is decided by the material slot that referenced it,
// because nothing in the picture itself says. A file that references one picture
// both ways round therefore costs two texture slots — no real exporter does
// that, and see 3d/src/import.c for why it is allowed rather than refused.
//
// IT IS A STARTUP OPERATION. Every upload it does waits for the GPU to go idle,
// so this is called before the first frame or between frames, and never while
// one is being recorded.
//
// WHAT IT DOES NOT DO: it is not a system and it writes no component it does not
// own. A transform goes in through `scene`'s own creation call, and if this ever
// needs to change one it submits an intent like everything else.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stddef.h>
#include <stdint.h>

// How deeply a file's nodes may nest before the file is refused (rule 14). A
// real model is a handful deep; this is far above that and it is a bound rather
// than a budget, so that a file cannot make the walk run forever.
#define VOE_3D_IMPORT_MAX_DEPTH 64

// What was made, so that a caller can find what it just loaded without having
// to walk the tables looking for it.
//
// `entities` LIVES IN THE ARENA THE IMPORT WAS HANDED, in the order the nodes
// were walked. It is what a call site uses to move a model after loading it —
// by submitting transform intents, like anything else that moves something.
typedef struct {
	const voe_ecs_entity *entities;
	uint32_t entity_count;

	// What was uploaded, counted so that a caller sizing a device's
	// capacities can see what a file cost: one texture per picture per
	// colour space it was wanted in — so not always the file's picture
	// count, see above — one material per glTF material, and one geometry
	// per primitive.
	uint32_t texture_count;
	uint32_t material_count;
	uint32_t geometry_count;
} voe_3d_import;

// False on failure, with `error` saying which category and a line on stderr
// saying exactly what happened. It fails when the file does — malformed or
// unsupported, see assets/model.h — and when something runs out of room: a
// texture slot, a geometry range, a shading record, an entity or a component
// table. The second kind comes back as VOE_BASE_ERROR_REFUSED, because what is
// wrong is that the world and the device were made smaller than this file needs.
//
// A FAILURE PART WAY THROUGH LEAVES WHAT IT HAD ALREADY MADE. Entities, uploads
// and components created before the failure are still there; nothing here
// unwinds, because `render` cannot free geometry yet and half a model in a world
// nobody drew is harmless. A caller that cares destroys the world and the
// device, which is what a program that could not load its model does anyway.
[[nodiscard]] bool voe_3d_import_glb(voe_ecs_world *world,
				     voe_render_device *device,
				     voe_base_arena *arena,
				     const uint8_t *bytes, size_t size,
				     voe_3d_import *out,
				     voe_base_error *error);
