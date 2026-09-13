// The cube the scene's two cubes are drawn with, by hand. Placeholder scene data
// at a call site, exactly as the scene in scene.c is: it is here until something
// loads a scene, and it is `dev/src/cubes.c`'s cube copied rather than shared,
// because `dev` is a leaf and so is this and neither includes the other's files.
//
// IT IS DATA AND NOT LOGIC. Twenty-four vertices and thirty-six indices, handed
// to `render` exactly as an importer would hand over a mesh it read from a file.
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

#include <render/device.h>

#include <stdint.h>

#define VOE_EDITOR_CUBE_VERTEX_COUNT 24
#define VOE_EDITOR_CUBE_INDEX_COUNT 36

extern const voe_render_vertex
	voe_editor_cube_vertices[VOE_EDITOR_CUBE_VERTEX_COUNT];
extern const uint32_t voe_editor_cube_indices[VOE_EDITOR_CUBE_INDEX_COUNT];
