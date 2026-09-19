// The built-in cube's vertices and indices, by hand. Internal to 3d: only
// shape_system.c uses it, to build the geometry voe_3d_shapes_upload hands
// every entity with a voe_3d_shape of VOE_3D_SHAPE_CUBE (3d/shape_system.h,
// ADR-0163). Moved here from editor/src/cube.c, which drew it by hand until
// this existed; dev/src/cubes.c is a separate copy, kept until something there
// wants the shape instead.
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
//
// ITS TWO COUNTS ARE CONSTANTS HERE, beside capsule.h's and cylinder.h's, and
// shape_system.c asserts at compile time that 3d/shape_system.h's capacity
// constants are their sums, so the two can never drift apart.
#pragma once

#include <render/device.h>

#define VOE_3D_CUBE_VERTICES 24
#define VOE_3D_CUBE_INDICES 36

extern const voe_render_vertex voe_3d_cube_vertices[VOE_3D_CUBE_VERTICES];
extern const uint32_t voe_3d_cube_indices[VOE_3D_CUBE_INDICES];
