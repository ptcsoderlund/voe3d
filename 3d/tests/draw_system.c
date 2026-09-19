// The one entity a pass may leave out: voe_3d_frame's `hidden` (ADR-0158).
// Everything with a mesh, a transform and a material is drawn, and everything
// with a panel, a transform and a fresh range is drawn — except the entity the
// frame names, of either table and either layer.
//
// THE DRAW COUNT IS THE MEASUREMENT FOR HIDING, BECAUSE A PICTURE SAYS LESS.
// What is observable is voe_render_frame_draw_count — every mesh drawn is one
// command and every panel is one command — so "not drawn" is exactly "one
// command fewer", and a skip that happened in the wrong place is a number that
// does not match.
//
// A COUNT ALONE CANNOT SAY *WHICH* ONE WENT, SO ONE CASE USES THE PROBE. Hiding
// either of two identical meshes gives the same number, and a run that skipped
// the first row whatever was hidden would pass that. The instrument that tells
// them apart is 3d/tests/panel.c's: a mesh naming a geometry the device never
// handed out is refused by render, and a refused draw stops the walk it was
// issued from. Put the probe first in the mesh table and the mesh behind it
// never draws — unless the probe is the entity that was hidden, in which case it
// does. So the two numbers name the entity that went.
//
// WHAT A WRONG SKIP LOOKS LIKE. Hiding nothing and getting fewer draws than
// there are drawables means an id is being compared to a zeroed one as though
// zero matched something; hiding a drawable and getting the same count means the
// test is not reaching the walk — a lookup in front of it, or the panel table
// forgotten; hiding an entity that draws nothing and getting fewer means the
// comparison is on the index alone and a stale generation matches.
//
// ONE CASE READS THE PICTURE, THROUGH render's PUBLIC voe_render_target_read: a
// shaped cube coloured red, in front of the camera, reads red at the centre —
// the shape's colour reaching the object's record (ADR-0191). It has a device
// of its own, sized for the built-in shapes.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, for the reason
// 3d/tests/import.c gives at length: a box with no Vulkan is the box and not
// this engine.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
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
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH (256 * 1024)
#define SIDE 16

// One triangle's worth of pools, room for every drawable a world below holds at
// once, and a shading slot for each material any of them uploads over the whole
// run — records are made once and never freed.
static const voe_render_capacities CAPACITIES = {
	.vertices = 3,
	.indices = 3,
	.geometries = 1,
	.objects = 8,
	.shadings = 16,
	.elements = 8,
	.passes = 1,
};

// A geometry id this device never handed out. render refuses a draw naming it,
// with a line on stderr, and that refusal is what the probe case reads.
static const voe_render_geometry NO_SUCH_MESH = { .index = 4242,
						  .generation = 4242 };

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 8,
		.intent_types = 8,
		// Only for the shaped case: the shape system queues removals
		// through it (3d/shape_system.h).
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	voe_3d_panel_register(world, 16);
	return world;
}

// At the origin, looking along its own -Z, so a thing at a more negative z is
// further away.
static void add_a_camera(voe_ecs_world *world)
{
	voe_ecs_entity eye = { 0 };
	voe_scene_camera camera = {
		.eye = { 0.0f, 0.0f, 0.0f },
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_camera_add(world, eye, camera));
}

static void add_the_sun(voe_ecs_world *world)
{
	voe_ecs_entity sun = { 0 };
	voe_scene_light light = {
		.direction = { 0.0f, -1.0f, 0.0f },
		.colour = { 1.0f, 1.0f, 1.0f },
		.intensity = 1.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, light));
}

// A transform `back` metres down the camera's line of sight, unrotated and
// unscaled.
static voe_scene_transform at_depth(float back)
{
	voe_scene_transform transform = {
		.position = { 0.0f, 0.0f, -back },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	return transform;
}

// The one real geometry, uploaded once at startup and shared by every mesh
// below: what is being counted is draw commands, and one triangle issues the
// same command a model would.
static voe_render_geometry a_triangle(voe_render_device *device)
{
	const voe_render_vertex vertices[3] = {
		{ .position = { -0.5f, -0.5f, 0.0f },
		  .normal = { 0.0f, 0.0f, 1.0f },
		  .uv = { 0.0f, 1.0f } },
		{ .position = { 0.5f, -0.5f, 0.0f },
		  .normal = { 0.0f, 0.0f, 1.0f },
		  .uv = { 1.0f, 1.0f } },
		{ .position = { 0.0f, 0.5f, 0.0f },
		  .normal = { 0.0f, 0.0f, 1.0f },
		  .uv = { 0.5f, 0.0f } },
	};
	const uint32_t indices[3] = { 0, 1, 2 };
	voe_render_geometry geometry = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_geometry_create(device, vertices, 3, indices,
						  3, &geometry, &error));
	return geometry;
}

// One mesh entity: a transform, a material whose record is uploaded, and the
// geometry it draws. `blended` picks the pass it lands in and `layer` the side
// of the depth clear, so between them the four groups are reachable.
static voe_ecs_entity add_a_mesh(voe_ecs_world *world,
				 voe_render_device *device,
				 voe_render_geometry geometry, float back,
				 voe_3d_layer layer, bool blended)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_material material = {
		.base_colour = { 1.0f, 1.0f, 1.0f, blended ? 0.5f : 1.0f },
		.roughness = 1.0f,
		.alpha_mode = blended ? VOE_RENDER_ALPHA_BLENDED :
					VOE_RENDER_ALPHA_OPAQUE,
	};
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_3d_material_upload(device, &material, &error));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at_depth(back)));
	VOE_TEST_CHECK(voe_3d_mesh_add(world, entity,
				       (voe_3d_mesh){ .geometry = geometry,
						      .layer = layer }));
	VOE_TEST_CHECK(voe_3d_material_add(world, entity, material));
	return entity;
}

// One panel entity: a transform and a surface of `count` elements starting at
// `first`. A panel is always blended, so its layer is the whole of where it
// lands.
static voe_ecs_entity add_a_panel(voe_ecs_world *world, float back,
				  voe_3d_layer layer, uint32_t first,
				  uint32_t count)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_panel panel = {
		.first = first,
		.count = count,
		.size = { 240.0f, 135.0f },
		.layer = layer,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at_depth(back)));
	VOE_TEST_CHECK(voe_3d_panel_add(world, entity, panel));
	return entity;
}

static voe_render_element a_rectangle(void)
{
	return (voe_render_element){
		.bounds = { 0.0f, 0.0f, 10.0f, 10.0f },
		.clip = { 0.0f, 0.0f, 10.0f, 10.0f },
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
}

// Opens a frame, puts `records` elements in it, walks the world with `hidden`
// left out, and hands back how many draw commands came out. Nought when there
// was nothing to draw into, which a caller distinguishes by never asking for
// nought.
static uint32_t draws_of_a_frame(voe_ecs_world *world,
				 voe_render_device *device,
				 voe_base_arena *arena, uint32_t records,
				 voe_ecs_entity hidden)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size);
	voe_render_pass_camera camera;
	bool drawing = false;
	uint32_t drawn;

	// The answer comes back hiding nothing, and hiding something is a write
	// to that answer — which is the whole of the caller's side of this.
	VOE_TEST_CHECK_INT(frame.hidden.index, 0);
	VOE_TEST_CHECK_INT(frame.hidden.generation, 0);
	frame.hidden = hidden;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light };

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	if (!drawing) {
		VOE_TEST_CHECK(drawing);
		return 0;
	}
	if (!voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW, &camera)) {
		VOE_TEST_CHECK(false);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		return 0;
	}

	for (uint32_t i = 0; i < records; i++)
		VOE_TEST_CHECK(
			voe_render_frame_submit_element(device, a_rectangle()));

	voe_3d_draw_system_run(world, device, arena, frame);
	drawn = voe_render_frame_draw_count(device);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return drawn;
}

static const voe_ecs_entity NOTHING_HIDDEN = { 0 };

// The field itself, over one world drawn seven times: nothing hidden draws
// everything, each drawable hidden in turn draws one fewer, and an entity that
// draws nothing changes nothing whether it is alive or not. The same world every
// time, because hiding is a property of one pass and must not outlive the frame
// that asked for it.
static void hiding_one_entity_removes_exactly_that_one(voe_base_arena *arena,
						       voe_render_device *device,
						       voe_render_geometry mesh)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity near_mesh;
	voe_ecs_entity far_mesh;
	voe_ecs_entity panel;
	voe_ecs_entity bare = { 0 };
	voe_ecs_entity stale;

	add_a_camera(world);
	add_the_sun(world);
	far_mesh = add_a_mesh(world, device, mesh, 9.0f, VOE_3D_LAYER_WORLD,
			      false);
	near_mesh = add_a_mesh(world, device, mesh, 3.0f, VOE_3D_LAYER_WORLD,
			       false);
	panel = add_a_panel(world, 5.0f, VOE_3D_LAYER_WORLD, 0, 2);

	// Three drawables, three commands.
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, NOTHING_HIDDEN), 3);

	// Each mesh in turn, and the panel: one fewer every time, and the world
	// is untouched between the frames.
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2, far_mesh),
			   2);
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2, near_mesh),
			   2);
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2, panel), 2);

	// An entity with no mesh and no panel: there is nothing to leave out and
	// nothing else may go either.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2, bare), 3);

	// And a drawable's index carrying the wrong generation, which is what a
	// program holding a destroyed entity hands over. It names nothing, so
	// nothing goes — a comparison on the index alone would hide a live mesh
	// here and this is the case that catches it.
	stale = near_mesh;
	stale.generation++;
	VOE_TEST_CHECK(!voe_ecs_entity_alive(world, stale));
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2, stale), 3);

	// Nothing stuck: a frame that hides nothing draws everything again.
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, NOTHING_HIDDEN), 3);
}

// WHICH ENTITY WENT, WHICH A COUNT ON ITS OWN CANNOT SAY. The probe is a mesh
// naming a geometry this device never handed out: render refuses the draw and
// the refusal stops the walk where it stands, so the mesh added behind the probe
// is drawn only when the probe never reached render at all.
static void the_hidden_one_is_the_one_that_goes(voe_base_arena *arena,
					       voe_render_device *device,
					       voe_render_geometry mesh)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity probe = { 0 };
	voe_3d_material material = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
	};
	voe_base_error error = VOE_BASE_OK;

	add_a_camera(world);
	add_the_sun(world);

	// First in the table, so the walk reaches it before the real mesh.
	VOE_TEST_CHECK(voe_3d_material_upload(device, &material, &error));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &probe));
	VOE_TEST_CHECK(voe_scene_transform_add(world, probe, at_depth(3.0f)));
	VOE_TEST_CHECK(voe_3d_mesh_add(world, probe,
				       (voe_3d_mesh){ .geometry = NO_SUCH_MESH,
						      .layer = VOE_3D_LAYER_WORLD }));
	VOE_TEST_CHECK(voe_3d_material_add(world, probe, material));

	(void)add_a_mesh(world, device, mesh, 4.0f, VOE_3D_LAYER_WORLD, false);
	(void)add_a_panel(world, 5.0f, VOE_3D_LAYER_WORLD, 0, 1);

	// Nothing hidden: the probe is refused, the mesh behind it is never
	// reached, and only the panel comes out.
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 1, NOTHING_HIDDEN), 1);

	// The probe hidden: it never reaches render, so the mesh behind it draws
	// and so does the panel. Two, and the picture still holds the other one.
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 1, probe), 2);
}

// BOTH LAYERS AND BOTH PASSES, WHICH IS WHERE A SKIP PUT IN ONE PLACE ONLY SHOWS
// UP. A world-solid mesh is drawn as the table is walked; a world-blended mesh
// waits for a sort; an overlay mesh waits for the depth clear as well; a panel is
// blended in the layer it names. Four drawables, one of each, and hiding any one
// of them costs exactly one command.
static void a_hidden_entity_goes_from_any_group(voe_base_arena *arena,
						voe_render_device *device,
						voe_render_geometry mesh)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity world_solid;
	voe_ecs_entity world_blended;
	voe_ecs_entity overlay_solid;
	voe_ecs_entity overlay_panel;

	add_a_camera(world);
	add_the_sun(world);
	world_solid = add_a_mesh(world, device, mesh, 8.0f, VOE_3D_LAYER_WORLD,
				 false);
	world_blended = add_a_mesh(world, device, mesh, 6.0f,
				   VOE_3D_LAYER_WORLD, true);
	overlay_solid = add_a_mesh(world, device, mesh, 4.0f,
				   VOE_3D_LAYER_OVERLAY, false);
	overlay_panel = add_a_panel(world, 2.0f, VOE_3D_LAYER_OVERLAY, 0, 2);

	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, NOTHING_HIDDEN), 4);
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, world_solid), 3);
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, world_blended), 3);
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, overlay_solid), 3);
	VOE_TEST_CHECK_INT(
		draws_of_a_frame(world, device, arena, 2, overlay_panel), 3);
}

// A cube coloured (1, 0, 0), three metres in front of the camera and lit
// from above and in front, drawn into a headless device's own picture: the centre pixel is red.
static void a_shaped_cube_draws_in_its_colour(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 1,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity cube = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_3d_frame frame;
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	bool drawing = false;
	const uint8_t *centre;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .direction = { 0.0f, -0.6f, -0.8f },
				   .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at_depth(3.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 1.0f, 0.0f, 0.0f } }));
	voe_3d_shape_system_run(world, &shapes);

	frame = voe_3d_draw_system_frame(world, size);
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light };
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	if (picture.pixels != NULL) {
		centre = picture.pixels +
			 ((size_t)(picture.height / 2) * picture.width +
			  picture.width / 2) *
				 4;
		// Red and not white: a lit dielectric adds a little of every
		// channel as its highlight, so green and blue are small next to
		// red rather than nought. A grey cube reads all three alike.
		VOE_TEST_CHECK(centre[0] > 128);
		VOE_TEST_CHECK(centre[1] < centre[0] / 2);
		VOE_TEST_CHECK(centre[2] < centre[0] / 2);
	}

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_render_geometry mesh;
	voe_base_error error = VOE_BASE_OK;

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

	// The probe below says so on stderr, on purpose: it is a draw naming a
	// mesh this device never handed out, and that line is the instrument
	// working rather than something going wrong.
	printf("the lines about a mesh this device did not hand out are the probe; see the header\n");

	mesh = a_triangle(device);

	hiding_one_entity_removes_exactly_that_one(arena, device, mesh);
	the_hidden_one_is_the_one_that_goes(arena, device, mesh);
	a_hidden_entity_goes_from_any_group(arena, device, mesh);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);

	a_shaped_cube_draws_in_its_colour();
	return voe_test_result();
}
