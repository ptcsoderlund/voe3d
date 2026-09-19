// The cube's vertices and indices. See cube.h for what this is and why it is
// twenty-four vertices.
//
// THE FIRST VERTEX OF EACH FACE IS ITS TOP-LEFT CORNER and +u runs to that
// face's right, because (0,0) is the top-left of a picture in Vulkan and in
// glTF — see assets/include/assets/image.h for why nothing turns an image over
// on the way in.
#include "cube.h"

// Half a metre, so the cube is one metre across. Units are metres (CLAUDE.md).
#define H 0.5f

const voe_render_vertex voe_3d_cube_vertices[VOE_3D_CUBE_VERTICES] = {
	// +Z
	{ { -H, H, H }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { H, H, H }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { H, -H, H }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, H }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	// -Z
	{ { H, H, -H }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
	{ { -H, H, -H }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, -H }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
	{ { H, -H, -H }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
	// +X
	{ { H, H, H }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, H, -H }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, -H, -H }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { H, -H, H }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	// -X
	{ { -H, H, -H }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { -H, H, H }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { -H, -H, H }, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, -H }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f } },
	// +Y
	{ { -H, H, -H }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, H, -H }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, H, H }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, H, H }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f } },
	// -Y
	{ { -H, -H, H }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
	{ { H, -H, H }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } },
	{ { H, -H, -H }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },
	{ { -H, -H, -H }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
};

// Two triangles a face, and the order is what makes each one counter-clockwise
// from outside. Thirty-two bits, which is what render's index pool holds.
const uint32_t voe_3d_cube_indices[VOE_3D_CUBE_INDICES] = {
	3, 2, 1, 3, 1, 0, // +Z
	7, 6, 5, 7, 5, 4, // -Z
	11, 10, 9, 11, 9, 8, // +X
	15, 14, 13, 15, 13, 12, // -X
	19, 18, 17, 19, 17, 16, // +Y
	23, 22, 21, 23, 21, 20, // -Y
};
