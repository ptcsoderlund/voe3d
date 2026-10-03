// The things a caller uploads before the first frame: geometry into the two
// shared pools, textures into the descriptor array, shading records into their
// buffer. What is checked is the bookkeeping — that two meshes get two ranges
// that do not overlap, that an id names what it named and nothing else, and that
// running out of room is a returned failure and not a surprise.
//
// A FULL POOL IS THE CASE WORTH HAVING A TEST FOR. Every capacity here is a
// number the caller chose when it opened the device, so asking for one more than
// that is an ordinary thing for a loader to do — a model bigger than the program
// expected — and it has to come back as a false with a message rather than as a
// truncated mesh or a slot handed out twice.
//
// THE STALE ID IS THE OTHER ONE. Slots are reused, so an id kept across a
// destroy names a live slot holding something else; the generation half is what
// catches that, and it is only worth having if something refuses an id whose
// generation has moved on. That is what the checks on a destroyed texture, mesh
// and shading record are; a freed mesh range is also checked to be reused, and
// two freed neighbours to merge into one that a larger mesh fits.
//
// A MESH'S OWN BOX IS HANDED BACK EXACTLY (0332): vertices spanning (−1, 0, −2)
// to (3, 5, 2) give that box, and once the mesh is destroyed the query is false.
//
// It includes render's internal header by relative path, as the other tests in
// this folder do: what a geometry id resolves to is not public — a caller has an
// id and a draw, and the range behind it is this folder's business — so the only
// way to check the ranges do not overlap is from inside.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. Uploading anything needs a
// driver, and a headless build box has none.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16

// Deliberately tiny, so that filling any of them is a handful of calls. Two
// triangles' worth of vertices, two meshes, two shading records.
#define POOL_VERTICES 6
#define POOL_INDICES 9
#define POOL_GEOMETRIES 2
#define POOL_SHADINGS 2

static const voe_render_capacities CAPACITIES = {
	.vertices = POOL_VERTICES,
	.indices = POOL_INDICES,
	.geometries = POOL_GEOMETRIES,
	.objects = 1,
	.shadings = POOL_SHADINGS,
	.passes = 1,
};

// A triangle: three vertices and three indices, which is the unit everything
// below is counted in.
static const voe_render_vertex TRIANGLE[3] = {
	{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
	{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
};
static const uint32_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

static const unsigned char ONE_PIXEL[4] = { 10, 20, 30, 255 };

static void two_meshes_get_two_ranges(voe_render_device *device)
{
	voe_render_geometry first = { 0 };
	voe_render_geometry second = { 0 };
	voe_render_geometry third = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const struct voe_render_geometry_slot *a;
	const struct voe_render_geometry_slot *b;

	VOE_TEST_CHECK(voe_render_geometry_create(device, TRIANGLE, 3,
						  TRIANGLE_INDICES, 3, &first,
						  &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, TRIANGLE, 3,
						  TRIANGLE_INDICES, 3, &second,
						  &error));

	// Two ids, and neither of them is a zeroed struct: generation 0 is never
	// handed out, so a component that was never filled in names nothing.
	VOE_TEST_CHECK(first.generation > 0);
	VOE_TEST_CHECK(second.generation > 0);
	VOE_TEST_CHECK(first.index != second.index);

	a = voe_render_geometry_at(device, first);
	b = voe_render_geometry_at(device, second);
	VOE_TEST_CHECK(a != NULL);
	VOE_TEST_CHECK(b != NULL);
	if (a == NULL || b == NULL)
		return;

	// The second mesh begins exactly where the first one ended, in both
	// pools. Anything else is either an overlap — two meshes over the same
	// bytes, which draws one of them as the other — or a gap, which spends
	// a pool nothing can use.
	VOE_TEST_CHECK_INT(a->first_vertex, 0);
	VOE_TEST_CHECK_INT(a->first_index, 0);
	VOE_TEST_CHECK_INT(a->index_count, 3);
	VOE_TEST_CHECK_INT(b->first_vertex, 3);
	VOE_TEST_CHECK_INT(b->first_index, 3);
	VOE_TEST_CHECK_INT(b->index_count, 3);

	// A third mesh has nowhere to go: the slot table holds two. It is a
	// returned failure and the device is still usable afterwards, which the
	// checks after this one rely on.
	VOE_TEST_CHECK(!voe_render_geometry_create(device, TRIANGLE, 3,
						   TRIANGLE_INDICES, 3, &third,
						   &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);

	// And an id nothing handed out names nothing, which is what makes a draw
	// with a stale or a zeroed id refusable.
	VOE_TEST_CHECK(voe_render_geometry_at(device, (voe_render_geometry){
						      0 }) == NULL);
	VOE_TEST_CHECK(voe_render_geometry_at(
			       device, (voe_render_geometry){
					       .index = first.index,
					       .generation =
						       first.generation + 1 }) ==
		       NULL);
}

// A pool that has run out of bytes, as opposed to a table that has run out of
// slots. Checked on a device of its own, because the check above spent the slots
// and this one needs to spend the vertices.
static void a_full_pool_says_so(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_device *device;
	voe_render_geometry geometry = { 0 };
	voe_base_error error = VOE_BASE_OK;

	// Room for two meshes' worth of slots but only one triangle's worth of
	// vertices, so the second mesh runs out of pool rather than out of
	// slots.
	room.vertices = 3;
	room.indices = 3;

	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL)
		return;

	VOE_TEST_CHECK(voe_render_geometry_create(device, TRIANGLE, 3,
						  TRIANGLE_INDICES, 3,
						  &geometry, &error));
	VOE_TEST_CHECK(!voe_render_geometry_create(device, TRIANGLE, 3,
						   TRIANGLE_INDICES, 3,
						   &geometry, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);

	// The mesh that did fit is still there and still names its range: a
	// refused upload leaves the pool as it was.
	VOE_TEST_CHECK(voe_render_geometry_at(device, geometry) != NULL);

	voe_render_device_destroy(device);
}

// A frame with one pass and a draw of each id, returning whether each was
// accepted. Nothing is read back: what is checked is which ids the draw takes.
static void draw_each(voe_render_device *device,
		      const voe_render_geometry *ids, bool *drawn, uint32_t count)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { 0 };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	for (uint32_t i = 0; i < count; i++)
		drawn[i] = voe_render_frame_draw(
			device, ids[i],
			(voe_render_object){ .colour = { 1, 1, 1, 1 } });
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// Room for exactly two meshes: a third fits only in the range the first gave
// back, and the first's id is refused by the draw and by a second destroy.
static void a_freed_range_is_reused(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_device *device;
	voe_render_geometry ids[3] = { 0 };
	bool drawn[3] = { true, false, false };
	voe_base_error error = VOE_BASE_OK;
	const struct voe_render_geometry_slot *slot;

	room.vertices = 6;
	room.indices = 6;
	room.objects = 3;
	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL)
		return;

	for (uint32_t i = 0; i < 2; i++)
		VOE_TEST_CHECK(voe_render_geometry_create(device, TRIANGLE, 3,
							  TRIANGLE_INDICES, 3,
							  &ids[i], &error));
	VOE_TEST_CHECK(voe_render_geometry_destroy(device, ids[0]));
	VOE_TEST_CHECK(!voe_render_geometry_destroy(device, ids[0]));
	VOE_TEST_CHECK(voe_render_geometry_create(device, TRIANGLE, 3,
						  TRIANGLE_INDICES, 3, &ids[2],
						  &error));
	VOE_TEST_CHECK_INT(ids[2].index, ids[0].index);
	VOE_TEST_CHECK(ids[2].generation != ids[0].generation);
	slot = voe_render_geometry_at(device, ids[2]);
	VOE_TEST_CHECK(slot != NULL);
	if (slot != NULL) {
		VOE_TEST_CHECK_INT(slot->first_vertex, 0);
		VOE_TEST_CHECK_INT(slot->first_index, 0);
	}

	draw_each(device, ids, drawn, 3);
	VOE_TEST_CHECK(!drawn[0]);
	VOE_TEST_CHECK(drawn[1]);
	VOE_TEST_CHECK(drawn[2]);

	voe_render_device_destroy(device);
}

// Two neighbouring ranges under a live one, freed upper first so the second
// merges into the hole above it: a mesh of their summed size then fits there.
static void neighbouring_ranges_merge(voe_base_arena *arena)
{
	static const uint32_t SIX_INDICES[6] = { 0, 1, 2, 3, 4, 5 };
	voe_render_vertex six[6];
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_device *device;
	voe_render_geometry ids[3] = { 0 };
	voe_render_geometry merged = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const struct voe_render_geometry_slot *slot;

	for (uint32_t i = 0; i < 6; i++)
		six[i] = TRIANGLE[i % 3];
	room.vertices = 9;
	room.indices = 9;
	room.geometries = 3;
	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL)
		return;

	for (uint32_t i = 0; i < 3; i++)
		VOE_TEST_CHECK(voe_render_geometry_create(device, TRIANGLE, 3,
							  TRIANGLE_INDICES, 3,
							  &ids[i], &error));
	VOE_TEST_CHECK(voe_render_geometry_destroy(device, ids[1]));
	VOE_TEST_CHECK(voe_render_geometry_destroy(device, ids[0]));
	VOE_TEST_CHECK(voe_render_geometry_create(device, six, 6, SIX_INDICES,
						  6, &merged, &error));
	slot = voe_render_geometry_at(device, merged);
	VOE_TEST_CHECK(slot != NULL);
	if (slot != NULL) {
		VOE_TEST_CHECK_INT(slot->first_vertex, 0);
		VOE_TEST_CHECK_INT(slot->first_index, 0);
	}

	voe_render_device_destroy(device);
}

// The box is the vertices' own corners, no corner a vertex: each axis' low and
// high come from different vertices.
static void a_mesh_hands_back_its_box(voe_base_arena *arena)
{
	static const voe_render_vertex SPREAD[3] = {
		{ { -1.0f, 0.0f, 2.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 3.0f, 1.0f, -2.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.0f, 5.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
	};
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry geometry = { 0 };
	voe_math_float3 min = { 9.0f, 9.0f, 9.0f };
	voe_math_float3 max = { 9.0f, 9.0f, 9.0f };
	voe_base_error error = VOE_BASE_OK;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL)
		return;

	VOE_TEST_CHECK(voe_render_geometry_create(device, SPREAD, 3,
						  TRIANGLE_INDICES, 3,
						  &geometry, &error));
	VOE_TEST_CHECK(voe_render_geometry_box(device, geometry, &min, &max));
	VOE_TEST_CHECK(min.x == -1.0f && min.y == 0.0f && min.z == -2.0f);
	VOE_TEST_CHECK(max.x == 3.0f && max.y == 5.0f && max.z == 2.0f);

	VOE_TEST_CHECK(voe_render_geometry_destroy(device, geometry));
	min = (voe_math_float3){ 9.0f, 9.0f, 9.0f };
	VOE_TEST_CHECK(!voe_render_geometry_box(device, geometry, &min, &max));
	VOE_TEST_CHECK(min.x == 9.0f && min.y == 9.0f && min.z == 9.0f);

	voe_render_device_destroy(device);
}

static void a_texture_id_names_one_texture(voe_render_device *device)
{
	voe_render_texture texture = { 0 };
	voe_render_texture again = { 0 };
	voe_render_texture stale;
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_texture_create(device,
						 VOE_RENDER_TEXTURE_COLOUR,
						 VOE_RENDER_SAMPLING_SMOOTH, 1,
						 1, ONE_PIXEL, &texture,
						 &error));

	// Never slot zero: that is the one-pixel white default every unclaimed
	// slot points at, and it is never handed out.
	VOE_TEST_CHECK(texture.index > VOE_RENDER_NO_TEXTURE);
	VOE_TEST_CHECK(texture.generation > 0);

	stale = texture;
	stale.generation++;

	// Destroying it gives the slot back and bumps the generation, so both
	// the id just used and any copy of it are refused from here on.
	VOE_TEST_CHECK(voe_render_texture_destroy(device, texture));
	VOE_TEST_CHECK(!voe_render_texture_destroy(device, texture));
	VOE_TEST_CHECK(!voe_render_texture_destroy(device, stale));

	// The white default is not a texture a caller may destroy, and neither
	// is a slot that was never claimed.
	VOE_TEST_CHECK(!voe_render_texture_destroy(
		device, (voe_render_texture){ .index = VOE_RENDER_NO_TEXTURE,
					      .generation = 1 }));
	VOE_TEST_CHECK(!voe_render_texture_destroy(
		device, (voe_render_texture){ .index = VOE_RENDER_MAX_TEXTURES,
					      .generation = 1 }));

	// The slot comes back, and the id that names it now is not the id that
	// named it before — which is the whole of what a generation is for.
	// The other kind and the other sampling mode, because a slot holds both
	// now and the one being reused was made with the opposite of each: what
	// a generation promises is that the old id names nothing, whatever the
	// new one holds.
	VOE_TEST_CHECK(voe_render_texture_create(device,
						 VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_SHARP, 1,
						 1, ONE_PIXEL, &again, &error));
	VOE_TEST_CHECK_INT(again.index, texture.index);
	VOE_TEST_CHECK(again.generation != texture.generation);
}

static void a_full_shading_buffer_says_so(voe_render_device *device)
{
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 1.0f,
		.roughness = 1.0f,
	};
	voe_render_shading shading = { 0 };
	voe_base_error error = VOE_BASE_OK;

	for (uint32_t i = 0; i < POOL_SHADINGS; i++) {
		VOE_TEST_CHECK(voe_render_shading_create(device, values,
							 &shading, &error));
		VOE_TEST_CHECK_INT(shading.index, i);
		VOE_TEST_CHECK(shading.generation > 0);
	}

	VOE_TEST_CHECK(!voe_render_shading_create(device, values, &shading,
						  &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_REFUSED);
}

// Run on the full shading buffer the check above left: a record destroyed
// frees its slot for the next create, and the old id names nothing.
static void a_destroyed_shading_slot_is_reused(voe_render_device *device)
{
	voe_render_shading_values values = { .base_colour = { 1, 1, 1, 1 } };
	voe_render_shading old = { .index = 0, .generation = 1 };
	voe_render_shading again = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_shading_destroy(device, old));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &again,
						 &error));
	VOE_TEST_CHECK_INT(again.index, old.index);
	VOE_TEST_CHECK(again.generation != old.generation);
	VOE_TEST_CHECK(!voe_render_shading_destroy(device, old));
	VOE_TEST_CHECK(voe_render_shading_destroy(device, again));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	two_meshes_get_two_ranges(device);
	a_texture_id_names_one_texture(device);
	a_full_shading_buffer_says_so(device);
	a_destroyed_shading_slot_is_reused(device);

	voe_render_device_destroy(device);

	a_full_pool_says_so(arena);
	a_freed_range_is_reused(arena);
	neighbouring_ranges_merge(arena);
	a_mesh_hands_back_its_box(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
