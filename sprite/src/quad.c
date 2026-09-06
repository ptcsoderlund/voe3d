// The quad's eight vertices and twelve indices, and the one call that puts them
// in the pools. See quad.h for what it is and why there are two faces.
//
// THE TWO FACES ARE A CUBE'S +Z AND -Z FACES WITH THE DEPTH TAKEN OUT, corner
// for corner and index for index. Copying a pair already proven
// counter-clockwise-from-outside is what keeps a hand-wound face from being
// wound the wrong way and silently culled — a mistake that costs nothing at
// build time and shows up as half a sprite missing.
//
// THE ARRAYS ARE NOT PUBLIC. Nothing outside this file needs the numbers: what a
// caller wants is the id, and rule 10 says a surface nothing calls is not
// written. A card that wants a quad of some other shape is the card that changes
// that.
#include <sprite/quad.h>

#include <base/assert.h>

#include <stdint.h>

// Half a metre, so the quad is one metre across. Units are metres (CLAUDE.md).
#define H 0.5f

#define VERTEX_COUNT 8
#define INDEX_COUNT 12

// (0, 0) is the top-left of the texture, which is what Vulkan and glTF both mean
// by it — see voe_render_vertex. So the quad's top-left corner in the world is
// where the picture's top-left corner lands.
static const voe_render_vertex VERTICES[VERTEX_COUNT] = {
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

static const uint32_t INDICES[INDEX_COUNT] = {
	3, 2, 1, 3, 1, 0, // +Z
	7, 6, 5, 7, 5, 4, // -Z
};

bool voe_sprite_quad_create(voe_render_device *device,
			    voe_render_geometry *out, voe_base_error *error)
{
	VOE_BASE_ASSERT(device != NULL, "making a sprite quad on no device");
	VOE_BASE_ASSERT(out != NULL, "making a sprite quad into nothing");

	return voe_render_geometry_create(device, VERTICES, VERTEX_COUNT,
					  INDICES, INDEX_COUNT, out, error);
}
