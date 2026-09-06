// The see-through quad's geometry, by hand. Placeholder scene data at a call
// site, the same kind of thing cubes.h is and there for the same reason: it is
// data, it decides nothing, and it is handed to `render` exactly as an importer
// would hand over a mesh it read from a file.
//
// A FLAT SQUARE, ONE METRE ACROSS, IN THE XY PLANE. Its normal is +Z, so it is
// lit like any other surface and a person can see the sun cross it.
//
// IT IS EIGHT VERTICES AND NOT FOUR, BECAUSE THE ENGINE CULLS BACK FACES. A
// single-sided square disappears for half of the camera's lap, which would make
// "is the sort right from every angle" impossible to look at — so this is two
// squares in the same place, wound opposite ways, with opposite normals. Exactly
// one of them survives culling from any given side, so nothing is drawn twice
// and nothing blends over itself.
//
// THAT IS A DOUBLE-SIDED *MESH* AND NOT A DOUBLE-SIDED MATERIAL. The engine
// culls back faces whatever a material says and `doubleSided` is not carried
// (assets/include/assets/model.h); geometry is a different matter and it is the
// call site's own. A file wanting a two-sided leaf is a different card.
#pragma once

#include <render/device.h>

#include <stdint.h>

#define VOE_DEV_QUAD_VERTEX_COUNT 8
#define VOE_DEV_QUAD_INDEX_COUNT 12

extern const voe_render_vertex voe_dev_quad_vertices[VOE_DEV_QUAD_VERTEX_COUNT];
extern const uint32_t voe_dev_quad_indices[VOE_DEV_QUAD_INDEX_COUNT];
