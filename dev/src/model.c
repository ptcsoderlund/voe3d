// The model exhibit: the embedded `.glb` and the one call that reads it into
// the world and moves it aside. See model.h for why it is its own file.
#include "model.h"

#include <3d/import.h>
#include <scene/transform_system.h>

#include <stdio.h>

// The model, embedded the same way as the cubes' pictures (src/cubes.c) and for
// the same reason:
// `platform` has no file API yet, so nothing here opens a file. The card that
// gives it one is the card that makes this path.
//
// IT CAME OUT OF BLENDER, WHICH IS THE POINT OF IT. Everything else
// here was built by this repository and agrees with this repository by
// construction; this is a file a real exporter wrote, with three primitives
// sharing one material, an albedo map and an ORM map, both a thousand pixels
// square. What it is really testing is that the reader survives a file nobody
// here wrote.
//
// BOTH ITS MAPS SHOW NOW. The albedo map is the texture coordinates' half and
// the ORM map — occlusion, roughness and metalness in the red, green and blue
// channels of one picture — is the shading's: card 019 lit the engine and reads
// all three of those channels. Its own export wires that picture into glTF's
// metallic-roughness slot and its normal slot rather than its occlusion slot, so
// the occlusion channel of it is not read as occlusion; that is the file's
// arrangement and assets/include/assets/model.h says why it is taken at its
// word.
const uint8_t voe_dev_human_glb[] = {
#embed "textured_primitives_human.glb"
};

const size_t voe_dev_human_glb_size = sizeof voe_dev_human_glb;

// One model, then one transform intent per entity to move the whole thing aside.
//
// A figure standing on nothing: `dev/src/textured_primitives_human.glb`, three
// primitives out of Blender sharing one material — a body, a bar of arms and a
// spherical head, about two metres tall, standing with its feet at y = 0 rather
// than centred like the cubes.
//
// The figure wears its own albedo map, which is the thing to look at for
// whether a real exporter's texture coordinates arrive intact.
//
// EVERY ENTITY HAS TO BE MOVED AND NOT JUST THE FIRST, WHICH IS THE FLATTENING
// SHOWING THROUGH. There is no parent component: the import composed the file's
// tree into world transforms, so moving a model means moving each of the things
// it turned into. The card that adds a hierarchy is the card that makes this one
// intent.
//
// What is wrong if it looks wrong:
//
//   - A model missing while the cubes are there — that import failed and said so
//     on stderr, or the world ran out of room for it. Each model is tried on its
//     own, so one of them can be missing without the other.
//   - The figure's texture smeared or in the wrong place while the cubes read
//     properly — a real exporter's texture coordinates, which nothing in this
//     repository generated. That is what having a file nobody here wrote is for.
bool voe_dev_add_a_model(voe_ecs_world *world, voe_render_device *gpu,
			 voe_base_arena *arena, const char *name,
			 const uint8_t *bytes, size_t size, float offset_x,
			 voe_base_error *error)
{
	voe_3d_import imported = { 0 };
	// Everything the import builds on the way through — the parsed model,
	// the decoded picture, the JSON, the list of entities — is scratch: the
	// GPU has taken its own copy of the uploads and the components hold the
	// ids, so none of it is read after this function returns. It is a few
	// megabytes and this is the mark that gives them back.
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

	if (!voe_3d_import_glb(world, gpu, arena, bytes, size, &imported,
			       error)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}

	for (uint32_t i = 0; i < imported.entity_count; i++) {
		const voe_scene_transform *placed =
			voe_scene_transform_get(world, imported.entities[i]);
		voe_scene_transform moved;

		if (placed == NULL)
			continue;

		// Read, change, submit: an intent carries the whole transform,
		// so a submitter reads the current one first. Reading is
		// anybody's; writing is the transform system's.
		moved = *placed;
		moved.position.x += offset_x;
		if (!voe_scene_transform_submit(
			    world, (voe_scene_transform_intent){
					   .entity = imported.entities[i],
					   .transform = moved })) {
			voe_base_arena_rewind(arena, mark);
			return false;
		}
	}

	// Textures and not pictures: a picture wanted as both a colour and a data
	// map is uploaded twice, so the two numbers are not always the same. See
	// 3d/import.h.
	printf("model      %-9s %u entities, %u meshes, %u materials, %u textures\n",
	       name, imported.entity_count, imported.geometry_count,
	       imported.material_count, imported.texture_count);

	// The intents carry the transforms by value, so nothing above is read
	// again and the whole import's working memory goes back here.
	voe_base_arena_rewind(arena, mark);
	return true;
}
