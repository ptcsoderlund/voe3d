// The quad's vertices and indices. See quad.h for what this is and why there are
// two faces in the same place.
//
// THE TWO FACES ARE THE CUBE'S +Z AND -Z FACES WITH THE DEPTH TAKEN OUT, corner
// for corner and index for index — see cubes.c. Copying the pair that is already
// proven counter-clockwise-from-outside is what keeps a hand-wound face from
// being wound the wrong way and silently culled.
#include "quad.h"

// Half a metre, so the quad is one metre across. Units are metres (CLAUDE.md).
#define H 0.5f

const voe_render_vertex voe_dev_quad_vertices[VOE_DEV_QUAD_VERTEX_COUNT] = {
	// +Z
	{ { -H, H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { H, H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { H, -H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	// -Z
	{ { H, H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
	{ { -H, H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { H, -H, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
};

const uint32_t voe_dev_quad_indices[VOE_DEV_QUAD_INDEX_COUNT] = {
	3, 2, 1, 3, 1, 0, // +Z
	7, 6, 5, 7, 5, 4, // -Z
};
