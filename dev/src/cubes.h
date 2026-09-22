// The two cubes exhibit: their geometry by hand, the two pictures they wear and
// the call that builds both entities. Placeholder scene data at a call site:
// this is what `dev` shows until something loads a scene, and it is the same
// cube card 014 put in `render` — moved out here when card 018 took the scene
// out of that folder.
//
// ITS OWN FILE BECAUSE IT IS ONE EXHIBIT. main.c wires the exhibits together;
// what the cubes are, what they wear and what is wrong if they look wrong is
// here, above voe_dev_add_the_cubes in cubes.c.
//
// IT IS DATA AND NOT LOGIC, WHICH IS WHY IT MAY LIVE IN dev AT ALL. Nothing here
// decides anything: it is twenty-four vertices and thirty-six indices, handed to
// `render` exactly as an importer would hand over a mesh it read from a file.
//
// TWENTY-FOUR VERTICES AND NOT EIGHT, because a cube corner is shared by three
// faces and those three faces need three different texture coordinates at that
// corner — the same point is the top-left of one face and the bottom-right of
// another, and a vertex carries one of each attribute.
//
// EVERY FACE IS WOUND COUNTER-CLOCKWISE SEEN FROM OUTSIDE, which is what the
// engine's front-face constant means. A cube wound the other way is inside out
// and every face of it is culled.
#pragma once

#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>

#include <stdint.h>

#define VOE_DEV_CUBE_VERTEX_COUNT 24
#define VOE_DEV_CUBE_INDEX_COUNT 36

extern const voe_render_vertex voe_dev_cube_vertices[VOE_DEV_CUBE_VERTEX_COUNT];
extern const uint32_t voe_dev_cube_indices[VOE_DEV_CUBE_INDEX_COUNT];

// Where the turning cube stands: this far along +X from the still one at the
// origin. The overlay quads stand inside it, so quad.c reads it too.
#define VOE_DEV_CUBES_APART 1.6f

// The geometry, a picture each, a shading record each and the two entities.
// `arena` holds each decoded picture while it uploads and is rewound after.
// `turning` is the cube main.c spins every frame.
[[nodiscard]] bool voe_dev_add_the_cubes(voe_ecs_world *world,
					 voe_render_device *gpu,
					 voe_base_arena *arena,
					 voe_ecs_entity *turning,
					 voe_base_error *error);
