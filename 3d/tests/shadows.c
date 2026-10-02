// The draw system casts the sun's shadows: voe_3d_draw_system_shadows between
// the frame's begin and the view's pass (ADR-0258).
//
// ONE PICTURE, TWO FLOOR PIXELS. A camera at the origin looks along -Z; a
// flattened cube lies a metre below as the floor, and a cube stands a metre
// above the floor's middle five metres out, lit by a sun straight down with
// bounces 1 (0319), so each caster is also drawn into the bounce map. The
// sun, both shapes and the model are built from literals that say
// `cast_shadows = true`, since zero casts nothing (0324). The
// floor pixel straight under the cube is in its shadow, and with no fill a
// shadowed surface is black; the floor pixel two metres to the side is lit.
// So the first reads darker than the second.
//
// A FILL LIFTS THE SHADOW AND NOTHING MORE (ADR-0275): with a white fill of 0.2
// the pixel under the cube reads lighter than it did with none, and still
// darker than the lit floor beside it; the lit floor reads as it did with none,
// within the 100 km case's two levels.
//
// WHERE THE PIXELS ARE. At five metres, a vertical field of sixty degrees over
// a square picture spans 2.89 m either way of the middle. The floor under the
// cube, (0, -0.95, -5), is a third of that below the middle; beside it,
// (2, -0.95, -5), is seven tenths of it to the right. Both are far inside what
// they are part of — the shadow is a metre across, the lit floor twenty.
//
// NO LIGHT CASTS NOTHING (ADR-0287): the call opens no pass, draws nothing,
// leaves `shadow` zeroed and still returns true, and the two pixels are both
// black, the floor lit by the zeroed light.
//
// WHAT DOES NOT CAST LEAVES THE FLOOR LIT (0324 points 4 and 5). With the
// sun's `cast_shadows` false no pass opens and nothing is drawn into one; with
// the sun casting and the cube's shape's false, only the floor is drawn into
// each cascade and the bounce map. Either way the floor under the cube reads
// within LIT_ALIKE of the floor beside it, and the view still draws the cube.
// Both run first, bouncing into a second target, so no earlier frame's bounce
// sits in the window's grid.
//
// 100 KM OUT (0250) the camera, floor and cube all stand 100 km along X and the
// same two pixels read as they did at the origin: the cascades are fitted about
// the eye and snapped in double, so nothing is lost to a float's reach. Both
// pictures are drawn with the bounce off (0310), the shadows call naming a
// second target, not the window the pass draws into, so the claim is the
// cascades' alone and no bounce under the cube is read.
//
// A MODEL CASTS AS A MESH DOES (0277 point 3): the cube swapped for a thing
// wearing model_data.inc's model, turned a quarter turn about +Z so the file's
// triangles, all in its x = 1 plane, lie flat a metre up, and standing so the
// green part's middle is over the floor under the cube. That pixel darkens as
// the cube's does, with two parts drawn into each cascade beside the floor.
//
// A LAMP CASTS THROUGH THE POINT-SHADOW PASS (0325 point 6): a camera six
// metres up looking down, a sun that does not cast, the floor, a cube at the
// origin and a lamp of range 6 at (−1.5, 1.5, 0). The floor at x = +1.5 reads
// darker than with the lamp's `cast_shadows` false, and as that with the cube's
// false; the lamp at +1.5 darkens x = −1.5 instead. Skipped, said, on a card
// whose point shadows are not ready.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_system.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include "model_data.inc"

#include <stdio.h>
#include <stdlib.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 64
// The two floor pixels, worked out in the header.
#define UNDER_X 32
#define BESIDE_X 54
#define FLOOR_Y 42
#define FAR_OUT 100000.0
// How far apart, in 8-bit levels, two pixels of the same lit floor may read.
#define LIT_ALIKE 4

// Three drawn objects at most (the floor and the model's two parts), each
// drawn once more into each of the four cascades and the bounce map, and the
// four shadow passes, the bounce pass and the point-shadow pass before the
// view's one, which the lamp case's two casters fit beside; room for the
// model's 6 vertices, 6 indices, 2 geometries and 2 shading records.
static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES + 6,
	.indices = VOE_3D_SHAPES_INDICES + 6,
	.geometries = VOE_3D_SHAPES_GEOMETRIES + 2,
	.objects = 3 + (VOE_RENDER_SHADOW_CASCADES + 1) * 3,
	.shadings = VOE_3D_SHAPES_SHADINGS + 2,
	.passes = 3 + VOE_RENDER_SHADOW_CASCADES,
	.targets = 1,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
	.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
};

// Which two pixels of a picture a_frame reads, on one row.
typedef struct {
	uint32_t under_x;
	uint32_t beside_x;
	uint32_t y;
} pixel_spots;

static const pixel_spots SUN_SPOTS = { UNDER_X, BESIDE_X, FLOOR_Y };
// From six metres up, x = ±1.5 on the floor is 12 pixels either side of the
// middle: the half width there is 6.95 m · tan 30° = 4.01 m over 32 pixels.
static const pixel_spots LAMP_SPOTS = { 44, 20, 32 };

// One grey shape standing `x` metres along X at `y` and `z`, scaled by `scale`,
// casting when `casts`.
static void add_a_shape(voe_ecs_world *world, double x, double y, double z,
			voe_math_float3 scale, bool casts)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_transform transform = {
		.position = { x, y, z },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = scale,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, transform));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f },
				.cast_shadows = casts }));
}

// Who casts in a world: the sun, and the standing cube. The floor always does.
typedef struct {
	bool sun;
	bool cube;
} casting;

static const casting ALL_CAST = { .sun = true, .cube = true };

// The camera, the floor and the cube, all `x` metres along X, and the sun
// straight down when `lit`, with a white fill of `fill`, each casting as `casts`
// says. With `model` the cube is a thing wearing it instead, casting, placed as
// the header says.
static voe_ecs_world *a_world(voe_base_arena *arena, const voe_3d_shapes *shapes,
			      double x, bool lit, float fill, const char *model,
			      casting casts)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 8);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 8);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { x, 0.0, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	if (lit) {
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_scene_transform_add(
			world, entity,
			(voe_scene_transform){
				.rotation = voe_scene_light_facing(
					(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
				.scale = { 1.0f, 1.0f, 1.0f } }));
		VOE_TEST_CHECK(voe_scene_light_add(
			world, entity,
			(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
					   .intensity = 3.0f,
					   .fill_colour = { 1.0f, 1.0f, 1.0f },
					   .fill_intensity = fill,
					   .bounces = 1,
					   .cast_shadows = casts.sun }));
	}
	add_a_shape(world, x, -1.0, -5.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f },
		    true);
	if (model == NULL) {
		add_a_shape(world, x, 1.0, -5.0,
			    (voe_math_float3){ 1.0f, 1.0f, 1.0f }, casts.cube);
	} else {
		voe_3d_model row = { .cast_shadows = true };

		// The green part lies at (-2,1,0) (-2,1,2) (0,1,0) once turned.
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
		VOE_TEST_CHECK(voe_scene_transform_add(
			world, entity,
			(voe_scene_transform){
				.position = { x + 4.0 / 3.0, 0.0, -5.0 - 2.0 / 3.0 },
				.rotation = { 0.0f, 0.0f, 0.70710678f, 0.70710678f },
				.scale = { 1.0f, 1.0f, 1.0f } }));
		snprintf(row.path, sizeof(row.path), "%s", model);
		VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	}
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// The two floor pixels of one picture, under the cube and beside it, and how
// many draws the view's pass made.
typedef struct {
	uint8_t under;
	uint8_t beside;
	uint32_t drawn;
} floor_pixels;

// One frame of `world` with its point lights and shadows, and the two pixels at
// `spots`. `count` is the shadow's cascade count; `added` how many draws the
// shadow call made. `bounced` is the target whose bounce grid the shadows call
// updates: the window to read the bounce, another to draw with it off.
static floor_pixels a_frame(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *arena, const voe_3d_models *models,
			    voe_render_target bounced, pixel_spots spots,
			    uint32_t *count, uint32_t *added)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	floor_pixels pixels = { 0 };
	bool drawing = false;
	uint32_t before;

	VOE_TEST_CHECK_INT(frame.shadow.count, 0);
	frame.models = models;
	frame.target = bounced;
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return pixels;
	before = voe_render_frame_draw_count(device);
	VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
	*added = voe_render_frame_draw_count(device) - before;
	*count = frame.shadow.count;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .shadow = frame.shadow,
					   .points = frame.points };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	before = voe_render_frame_draw_count(device);
	voe_3d_draw_system_run(world, device, arena, frame);
	pixels.drawn = voe_render_frame_draw_count(device) - before;
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	if (picture.pixels == NULL)
		return pixels;
	pixels.under = picture.pixels[((size_t)spots.y * SIDE + spots.under_x) * 4];
	pixels.beside =
		picture.pixels[((size_t)spots.y * SIDE + spots.beside_x) * 4];
	printf("floor under %u, beside %u\n", pixels.under, pixels.beside);
	return pixels;
}

// The floor under the cube is in its shadow; the floor beside it is not. The
// bounce grid updated is `bounced`'s.
static floor_pixels the_cube_shadows_the_floor(voe_render_device *device,
					       const voe_3d_shapes *shapes,
					       double x,
					       voe_render_target bounced)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world =
		a_world(arena, shapes, x, true, 0.0f, NULL, ALL_CAST);
	uint32_t count = 0;
	uint32_t added = 0;
	floor_pixels pixels =
		a_frame(world, device, arena, NULL, bounced, SUN_SPOTS,
			&count, &added);

	VOE_TEST_CHECK_INT(count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK_INT(added, (VOE_RENDER_SHADOW_CASCADES + 1) * 2);
	VOE_TEST_CHECK(pixels.under + 32 < pixels.beside);
	voe_base_arena_destroy(arena);
	return pixels;
}

// No light: nothing cast, nothing drawn, the floor black under and beside.
static void no_light_casts_nothing(voe_render_device *device,
				   const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world =
		a_world(arena, shapes, 0.0, false, 0.0f, NULL, ALL_CAST);
	uint32_t count = 1;
	uint32_t added = 1;
	floor_pixels pixels = a_frame(world, device, arena, NULL,
				      VOE_RENDER_TARGET_WINDOW, SUN_SPOTS,
				      &count, &added);

	VOE_TEST_CHECK_INT(count, 0);
	VOE_TEST_CHECK_INT(added, 0);
	VOE_TEST_CHECK_INT(pixels.under, 0);
	VOE_TEST_CHECK_INT(pixels.beside, 0);
	voe_base_arena_destroy(arena);
}

// The floor under the cube as lit as beside it, within LIT_ALIKE, when `casts`
// leaves the sun or the cube out: with the sun out no pass and no draw, with
// the cube out the floor alone drawn into each cascade and the bounce map. The
// cube is drawn in the view either way, beside the floor. `bounced` as for
// the_cube_shadows_the_floor.
static void nothing_cast_leaves_the_floor_lit(voe_render_device *device,
					      const voe_3d_shapes *shapes,
					      casting casts,
					      voe_render_target bounced)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world =
		a_world(arena, shapes, 0.0, true, 0.0f, NULL, casts);
	uint32_t count = 1;
	uint32_t added = 1;
	floor_pixels pixels =
		a_frame(world, device, arena, NULL, bounced, SUN_SPOTS,
			&count, &added);

	VOE_TEST_CHECK_INT(count, casts.sun ? VOE_RENDER_SHADOW_CASCADES : 0);
	VOE_TEST_CHECK_INT(added, casts.sun ? VOE_RENDER_SHADOW_CASCADES + 1 : 0);
	VOE_TEST_CHECK_INT(pixels.drawn, 2);
	VOE_TEST_CHECK(abs((int)pixels.under - (int)pixels.beside) <= LIT_ALIKE);
	VOE_TEST_CHECK(pixels.beside > 32);
	voe_base_arena_destroy(arena);
}

// A white fill of 0.2 lifts the floor under the cube above `unfilled`'s and
// leaves it darker than the lit floor beside it; that lit floor reads as
// `unfilled`'s, since the fill fades out where the sun reaches (ADR-0276).
static void a_fill_lifts_the_shadow(voe_render_device *device,
				    const voe_3d_shapes *shapes,
				    floor_pixels unfilled)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world =
		a_world(arena, shapes, 0.0, true, 0.2f, NULL, ALL_CAST);
	uint32_t count = 0;
	uint32_t added = 0;
	floor_pixels pixels = a_frame(world, device, arena, NULL,
				      VOE_RENDER_TARGET_WINDOW, SUN_SPOTS,
				      &count, &added);

	VOE_TEST_CHECK(pixels.under > unfilled.under);
	VOE_TEST_CHECK(pixels.under < pixels.beside);
	VOE_TEST_CHECK(abs((int)pixels.beside - (int)unfilled.beside) <= 2);
	voe_base_arena_destroy(arena);
}

// A thing wearing the model darkens the floor under it as the cube does: the
// floor and the model's two parts drawn into each cascade.
static void the_model_shadows_the_floor(voe_render_device *device,
					const voe_3d_shapes *shapes)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world(arena, shapes, 0.0, true, 0.0f,
				       "Assets/two.glb", ALL_CAST);
	voe_3d_models *models = voe_3d_models_new();
	voe_base_error error = VOE_BASE_OK;
	uint32_t count = 0;
	uint32_t added = 0;
	floor_pixels pixels;

	VOE_TEST_CHECK(voe_3d_models_load(models, device, "Assets/two.glb", 1,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error));
	pixels = a_frame(world, device, arena, models, VOE_RENDER_TARGET_WINDOW,
			 SUN_SPOTS, &count, &added);
	VOE_TEST_CHECK_INT(count, VOE_RENDER_SHADOW_CASCADES);
	VOE_TEST_CHECK_INT(added, (VOE_RENDER_SHADOW_CASCADES + 1) * 3);
	VOE_TEST_CHECK(pixels.under + 32 < pixels.beside);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_base_arena_destroy(arena);
}

// The camera six metres up looking down, a sun of nothing that does not cast,
// the floor, a cube at the origin casting when `cube_casts`, and a white lamp
// of range 6 at (`lamp_x`, 1.5, 0) casting when `lamp_casts`.
static voe_ecs_world *a_lamp_world(voe_base_arena *arena,
				   const voe_3d_shapes *shapes, double lamp_x,
				   bool lamp_casts, bool cube_casts)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_scene_point_light_register(world, 1);
	voe_3d_mesh_register(world, 8);
	voe_3d_material_register(world, 8);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 6.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f } }));
	add_a_shape(world, 0.0, -1.0, 0.0, (voe_math_float3){ 20.0f, 0.1f, 20.0f },
		    true);
	add_a_shape(world, 0.0, 0.0, 0.0, (voe_math_float3){ 1.0f, 1.0f, 1.0f },
		    cube_casts);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { lamp_x, 1.5, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, entity,
		(voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					 .intensity = 3.0f,
					 .range = 6.0f,
					 .falloff = 1.0f,
					 .cast_shadows = lamp_casts }));
	voe_3d_shape_system_run(world, shapes);
	return world;
}

// One frame of a_lamp_world's world: the floor at x = +1.5 as `under`, at
// x = −1.5 as `beside`, and the draws the shadows call made into `added`.
static floor_pixels a_lamp_frame(voe_render_device *device,
				 const voe_3d_shapes *shapes, double lamp_x,
				 bool lamp_casts, bool cube_casts,
				 uint32_t *added)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world =
		a_lamp_world(arena, shapes, lamp_x, lamp_casts, cube_casts);
	uint32_t count = 1;
	floor_pixels pixels =
		a_frame(world, device, arena, NULL, VOE_RENDER_TARGET_WINDOW,
			LAMP_SPOTS, &count, added);

	VOE_TEST_CHECK_INT(count, 0);
	voe_base_arena_destroy(arena);
	return pixels;
}

// A casting lamp left of the cube darkens the floor right of it, against the
// same lamp not casting, and with the cube not casting reads as not casting;
// the lamp moved right darkens the floor left of the cube instead. The casters
// drawn into the point-shadow pass are the floor and the cube, or the floor.
static void a_lamp_shadows_the_floor(voe_render_device *device,
				     const voe_3d_shapes *shapes)
{
	uint32_t added = 0;
	floor_pixels casting, open, uncast, mirrored, mirrored_open;

	if (!voe_render_point_shadows_ready(device)) {
		printf("skip lamp case: point shadows not ready on this card\n");
		return;
	}
	casting = a_lamp_frame(device, shapes, -1.5, true, true, &added);
	VOE_TEST_CHECK_INT(added, 2);
	open = a_lamp_frame(device, shapes, -1.5, false, true, &added);
	VOE_TEST_CHECK_INT(added, 0);
	uncast = a_lamp_frame(device, shapes, -1.5, true, false, &added);
	VOE_TEST_CHECK_INT(added, 1);
	mirrored = a_lamp_frame(device, shapes, 1.5, true, true, &added);
	mirrored_open = a_lamp_frame(device, shapes, 1.5, false, true, &added);

	VOE_TEST_CHECK(open.under > 32);
	VOE_TEST_CHECK(casting.under + 16 < open.under);
	VOE_TEST_CHECK(abs((int)uncast.under - (int)open.under) <= LIT_ALIKE);
	VOE_TEST_CHECK(mirrored.beside + 16 < mirrored_open.beside);
	VOE_TEST_CHECK(abs((int)mirrored.under - (int)mirrored_open.under) <=
		       LIT_ALIKE);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_render_target other;
	voe_render_texture shown;
	voe_3d_shapes shapes;
	floor_pixels near;
	floor_pixels far;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	VOE_TEST_CHECK(voe_render_target_create(device, SIDE, SIDE, &other, &shown,
						&error));

	// First, while no frame has bounced into the window's grid.
	nothing_cast_leaves_the_floor_lit(
		device, &shapes, (casting){ .sun = false, .cube = true }, other);
	nothing_cast_leaves_the_floor_lit(
		device, &shapes, (casting){ .sun = true, .cube = false }, other);
	near = the_cube_shadows_the_floor(device, &shapes, 0.0,
					  VOE_RENDER_TARGET_WINDOW);
	no_light_casts_nothing(device, &shapes);
	a_fill_lifts_the_shadow(device, &shapes, near);
	near = the_cube_shadows_the_floor(device, &shapes, 0.0, other);
	far = the_cube_shadows_the_floor(device, &shapes, FAR_OUT, other);
	VOE_TEST_CHECK(abs((int)far.under - (int)near.under) <= 2);
	VOE_TEST_CHECK(abs((int)far.beside - (int)near.beside) <= 2);
	the_model_shadows_the_floor(device, &shapes);
	a_lamp_shadows_the_floor(device, &shapes);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
