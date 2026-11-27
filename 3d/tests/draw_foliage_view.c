// The view and the sun draw a frame's foliage (0442 point 5), in the world and
// store of draw_foliage_world.inc: the casting sun shines along (-1, -1,
// -0.5), so the first copy's quad, standing in x = 1 from y 0 to 2, throws its
// shadow back over its own place, the origin under the picture's centre (its
// point y 1, z 0.5 lands there), and no fill lights the shadow.
//
// 5. The centre pixel with the copies differs from the same frame with none.
// 6. The whole frame's draws, shadows and view, with three copies equal those
//    with the first alone, and exceed those with none.
// 15. The camera draws its G seen groups as G × 2 draws, laid then shaded
//     (0508), its triangles one cascade's; each cascade's draws are G, whole.
//
// Each copy is gathered seen, cast or unseen (0464 point 4):
//
// 7. The camera turned to look straight up: the gather counts every copy
//    skipped, none drawn, and no group is seen.
// 8. The camera 1.5 m up at z = −5 looking along −Z 20° below level, every
//    copy behind it, and the sun low beyond them along (−0.5, −1, −4): every
//    copy counted skipped, yet the floor at column 17, row 28, in the first
//    copy's shadow three pixels wide there, reads darker than with none.
// 9. With the camera of 5, every copy is counted drawn.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include "draw_foliage_world.inc"

// What a frame cost: its draws and triangles, the shadows' among them, and
// its seen groups.
struct frame_cost {
	uint32_t draws, shadow_draws;
	uint64_t triangles, shadow_triangles;
	uint32_t seen;
};

// One whole frame of `world` from a store of the first `count` copies, as the
// loop draws it: gathered, its shadows, the view's pass and the run. Its cost
// in `cost`, its counts after the shadows in `counts`; the pixel at `column`,
// `row`, rgb packed.
static uint32_t a_frame_at(voe_ecs_world *world, voe_render_device *device,
			   voe_base_arena *arena, uint32_t count,
			   uint32_t column, uint32_t row,
			   struct frame_cost *cost, voe_3d_view_counts *counts)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_models *models = a_store(device, arena, true, count);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	const uint8_t *pixel;
	uint32_t centre = 0;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_3d_draw_system_foliage(world, device, &frame,
							  arena));
		cost->seen = 0;
		for (uint32_t i = 0; i < frame.foliage_count; i++)
			cost->seen += frame.foliage[i].tier == VOE_3D_FOLIAGE_SEEN;
		VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
		VOE_TEST_CHECK_INT(frame.shadow.count,
				   VOE_RENDER_SHADOW_CASCADES);
		cost->shadow_draws = voe_render_frame_draw_count(device);
		cost->shadow_triangles = voe_render_frame_triangle_count(device);
		*counts = frame.counts;
		camera = voe_3d_draw_system_camera(&frame);
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		cost->draws = voe_render_frame_draw_count(device);
		cost->triangles = voe_render_frame_triangle_count(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK(voe_render_target_read(
			device, VOE_RENDER_TARGET_WINDOW, arena, &picture, &error));
	}
	if (picture.pixels != NULL) {
		pixel = picture.pixels +
			((size_t)row * picture.width + column) * 4;
		centre = (uint32_t)pixel[0] << 16 | (uint32_t)pixel[1] << 8 |
			 pixel[2];
		printf("%u copies: %u groups, %u draws, %u drawn, %u skipped, "
		       "%u cast, %u cast skipped, (%u, %u) %06x\n",
		       count, frame.foliage_count, cost->draws, frame.counts.drawn,
		       frame.counts.skipped, frame.counts.cast,
		       frame.counts.cast_skipped, column, row, centre);
	}
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	return centre;
}

// The frame of a_frame_at, its centre pixel.
static uint32_t a_frame(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *arena, uint32_t count,
			struct frame_cost *cost, voe_3d_view_counts *counts)
{
	return a_frame_at(world, device, arena, count, SIDE / 2, SIDE / 2,
			  cost, counts);
}

// The sum of a packed pixel's three channels.
static uint32_t brightness(uint32_t rgb)
{
	return (rgb >> 16 & 0xff) + (rgb >> 8 & 0xff) + (rgb & 0xff);
}

// 5, 6 and 9.
static void the_view_and_the_sun_draw_copies(voe_render_device *device,
					     voe_base_arena *arena)
{
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	struct frame_cost none_cost = { 0 }, one_cost = { 0 }, three_cost = { 0 };
	voe_3d_view_counts counts = { 0 };
	const uint32_t none =
		a_frame(world, device, arena, 0, &none_cost, &counts);
	const uint32_t three =
		a_frame(world, device, arena, COPIES, &three_cost, &counts);

	VOE_TEST_CHECK_INT(counts.drawn, COPIES);
	VOE_TEST_CHECK_INT(counts.skipped, 0);
	(void)a_frame(world, device, arena, 1, &one_cost, &counts);
	VOE_TEST_CHECK(three != none);
	// The same groups, so the same draws: two a seen group in the camera
	// pass since 0508, one a cascade.
	VOE_TEST_CHECK_INT(three_cost.draws, one_cost.draws);
	VOE_TEST_CHECK(one_cost.draws > none_cost.draws);
}

// 15, from the camera of 5: every group seen, casting and not `rest`.
static void the_camera_lays_then_shades(voe_render_device *device,
					voe_base_arena *arena)
{
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	struct frame_cost none = { 0 }, three = { 0 };
	voe_3d_view_counts counts = { 0 };
	long long camera_draws, camera_triangles, cast_triangles;

	(void)a_frame(world, device, arena, 0, &none, &counts);
	(void)a_frame(world, device, arena, COPIES, &three, &counts);
	camera_draws = (long long)(three.draws - three.shadow_draws) -
		       (long long)(none.draws - none.shadow_draws);
	camera_triangles =
		(long long)(three.triangles - three.shadow_triangles) -
		(long long)(none.triangles - none.shadow_triangles);
	cast_triangles = (long long)three.shadow_triangles -
			 (long long)none.shadow_triangles;
	printf("%u seen: %lld camera draws, %lld camera triangles, "
	       "%lld cast triangles\n",
	       three.seen, camera_draws, camera_triangles, cast_triangles);
	VOE_TEST_CHECK(three.seen > 0);
	// 0508 point 1: each seen group laid, then shaded.
	VOE_TEST_CHECK_INT(camera_draws, 2 * (long long)three.seen);
	// Each cascade draws each group whole, once, as before 0508.
	VOE_TEST_CHECK_INT(three.shadow_draws - none.shadow_draws,
			   VOE_RENDER_SHADOW_CASCADES * three.seen);
	// A laid draw adds no triangles: the camera's are G whole draws', as
	// each cascade's are.
	VOE_TEST_CHECK(camera_triangles > 0);
	VOE_TEST_CHECK_INT(camera_triangles * VOE_RENDER_SHADOW_CASCADES,
			   cast_triangles);
}

// 7: the camera turned to look straight up from 5 m.
static void copies_behind_are_skipped(voe_render_device *device,
				      voe_base_arena *arena)
{
	voe_3d_models *models = a_store(device, arena, true, COPIES);
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world_from(
		arena, &land,
		(voe_scene_transform){
			.position = { 0.0, 5.0, 0.0 },
			.rotation = { 0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } },
		(voe_math_float3){ -1.0f, -1.0f, -0.5f });
	voe_3d_frame frame;

	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.counts.drawn, 0);
	VOE_TEST_CHECK_INT(frame.counts.skipped, COPIES);
	for (uint32_t i = 0; i < frame.foliage_count; i++)
		VOE_TEST_CHECK(frame.foliage[i].tier != VOE_3D_FOLIAGE_SEEN);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// 8: the camera 1.5 m up at z = −5, 20° below level, the sun beyond.
static void copies_behind_still_shadow(voe_render_device *device,
				       voe_base_arena *arena)
{
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world_from(
		arena, &land,
		(voe_scene_transform){
			.position = { 0.0, 1.5, -5.0 },
			.rotation = { -0.17364818f, 0.0f, 0.0f, 0.98480775f },
			.scale = { 1.0f, 1.0f, 1.0f } },
		(voe_math_float3){ -0.5f, -1.0f, -4.0f });
	struct frame_cost cost = { 0 };
	voe_3d_view_counts counts = { 0 };
	const uint32_t none = a_frame_at(world, device, arena, 0, 17, 28,
					 &cost, &counts);
	const uint32_t three = a_frame_at(world, device, arena, COPIES, 17, 28,
					  &cost, &counts);

	VOE_TEST_CHECK_INT(counts.drawn, 0);
	VOE_TEST_CHECK_INT(counts.skipped, COPIES);
	VOE_TEST_CHECK(brightness(three) < brightness(none));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_render_device *device = a_device(arena);

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	the_view_and_the_sun_draw_copies(device, arena);
	the_camera_lays_then_shades(device, arena);
	copies_behind_are_skipped(device, arena);
	copies_behind_still_shadow(device, arena);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
