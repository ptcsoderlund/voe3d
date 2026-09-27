// RENDER HOLDS A THOUSAND TEXTURES AND SAMPLES THE LAST OF THEM (0278). On the
// headless device: one-pixel textures are created until one is refused, at least
// 1000 succeed and the refusal is VOE_BASE_ERROR_REFUSED; then a quad over the
// whole target, wearing the last texture created as its base colour, reads that
// texture's colour and nothing else.
//
// THE LAST TEXTURE IS RED AND EVERY OTHER GREEN, which is what makes the picture
// say which slot was read. Its colour is not known until the pool is full, so
// the last green one is destroyed and a red one made in its place: slots are
// taken lowest first, so it lands in the highest slot, far past the old 64 — the
// element a shader array, a set layout or a pool one size too short would lose.
//
// Built on tests/element_scene.h for the device, the readback and the colour
// counts. A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO, as the others do.
// The refused create prints its one `render` line to stderr; that is expected.
#include "element_scene.h"

#include <stdint.h>

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
};

static const uint8_t RED_TEXEL[4] = { 255, 0, 0, 255 };
static const uint8_t GREEN_TEXEL[4] = { 0, 255, 0, 255 };

// Creates green textures until one is refused and returns how many succeeded;
// *last is the last one made.
static uint32_t fill_the_slots(voe_render_device *device,
			       voe_render_texture *last)
{
	uint32_t made = 0;
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture texture;

	// One past the array is the bound: a create that never refuses is a
	// failure below, not a loop that runs for ever.
	for (uint32_t i = 0; i <= VOE_RENDER_MAX_TEXTURES; i++) {
		if (!voe_render_texture_create(device, VOE_RENDER_TEXTURE_COLOUR,
					       VOE_RENDER_SAMPLING_SHARP, 1, 1,
					       GREEN_TEXEL, &texture, &error))
			break;
		*last = texture;
		made++;
	}
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
	return made;
}

// The whole of clip space, wound as elements.c's quad is, at depth 0.5.
static bool upload_quad(voe_render_device *device, voe_render_geometry *quad)
{
	voe_base_error error = VOE_BASE_OK;
	const voe_render_vertex vertices[4] = {
		{ { -1.0f, 1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	};
	const uint32_t indices[6] = { 3, 2, 1, 3, 1, 0 };

	return voe_render_geometry_create(device, vertices, 4, indices, 6, quad,
					  &error);
}

static void the_last_texture_is_the_one_drawn(struct scene *scene,
					      voe_render_texture last)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry quad;
	voe_render_shading shading;
	const struct voe_render_frame *frame;
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_texture = last.index,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(upload_quad(scene->device, &quad));
	VOE_TEST_CHECK(voe_render_shading_create(scene->device, values, &shading,
						 &error));
	frame = voe_render_frame_current(scene->device);
	if (!open_frame(scene->device))
		return;
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, quad,
		(voe_render_object){
			.world = voe_math_float4x4_identity(),
			.normal = voe_math_float4x4_identity(),
			.shading = shading.index,
			.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		}));
	VOE_TEST_CHECK(close_frame(scene->device));
	read_back(scene->device, frame, scene->readback.buffer);

	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };
	voe_render_texture last = { 0 };
	uint32_t made;

	if (!open_scene(&scene, arena, CAPACITIES)) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	made = fill_the_slots(scene.device, &last);
	VOE_TEST_CHECK(made >= 1000);

	// The last green one gives its slot to the red one.
	VOE_TEST_CHECK(voe_render_texture_destroy(scene.device, last));
	VOE_TEST_CHECK(voe_render_texture_create(scene.device,
						 VOE_RENDER_TEXTURE_COLOUR,
						 VOE_RENDER_SAMPLING_SHARP, 1, 1,
						 RED_TEXEL, &last, &error));
	VOE_TEST_CHECK(last.index >= 1000);

	if (scene.pixels != NULL)
		the_last_texture_is_the_one_drawn(&scene, last);
	else
		VOE_TEST_CHECK(scene.pixels != NULL);

	close_scene(&scene);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
