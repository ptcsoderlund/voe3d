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
// ONE NEEDS NO CARD: a camera whose transform is scaled to nothing frames
// `blind` (0223). And a marked camera is one draw more, a zeroed marker none.
//
// AND ONE MORE READS IT FOR THE GIZMO (ADR-0205): a dark cube with a gizmo
// standing in it, drawn twice — once with the gizmo and once with the field
// zeroed. A pixel the arrow along +X covers, inside the rectangle the cube
// covers, is the gizmo's colour in the first picture and the cube's in the
// second. That is the second depth clear doing its work: without it the cube
// the gizmo stands inside is in front of the arrow and the pixel stays the
// cube's. The colour is linear (1, 0, 1), which an sRGB target hands back as
// exactly (255, 0, 255) — nought and one are the two values the curve leaves
// alone — and no lit pixel of a dark cube can be that.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, for the reason
// 3d/tests/import.c gives at length: a box with no Vulkan is the box and not
// this engine.
#include <3d/draw_system.h>
#include <3d/gizmo.h>
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

// The gizmo case's own picture and the room two of them need beside a device:
// wide enough that a shaft three pixels across and the cube it stands in are
// both several pixels of it.
#define GIZMO_SCRATCH (4 * 1024 * 1024)
#define GIZMO_SIDE 128
// One shaft, in pixels: two thirds of the way from the middle to the edge, so
// the near end of the arrow is well inside the cube and its far end well
// outside.
#define GIZMO_PIXELS 60.0f

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
// further away, with its transform scaled by `scale`.
static void add_a_camera_scaled(voe_ecs_world *world, voe_math_float3 scale)
{
	voe_ecs_entity eye = { 0 };
	voe_scene_transform pose = {
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = scale,
	};
	voe_scene_camera lens = {
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_transform_add(world, eye, pose));
	VOE_TEST_CHECK(voe_scene_camera_add(world, eye, lens));
}

static void add_a_camera(voe_ecs_world *world)
{
	add_a_camera_scaled(world, (voe_math_float3){ 1.0f, 1.0f, 1.0f });
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

// One frame of the world with `gizmo` standing in it, and the picture that came
// out. The answer comes back with no gizmo in it and standing one on something
// is a write to that answer, which is the whole of the caller's side of this.
static voe_render_picture a_gizmo_frame(voe_ecs_world *world,
					voe_render_device *device,
					voe_base_arena *arena,
					voe_3d_gizmoed gizmo)
{
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK_INT(frame.gizmo.entity.index, 0);
	VOE_TEST_CHECK_INT(frame.gizmo.entity.generation, 0);
	frame.gizmo = gizmo;
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
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

// WHICH PIXELS OF THE ARROW ALONG +X ARE READ. The arrow runs right from the
// middle of the picture, where the cube's centre is, and the shaft is three
// pixels across — so it is one row off the middle for the whole of its length.
// The cube covers twenty-three pixels either way of the middle: half a metre
// three metres in front of an eye whose vertical field of view is sixty
// degrees, over a picture GIZMO_SIDE tall. The stretch read is well inside
// that and starts well outside the label at the tip of the arrow along +Z,
// which points at the eye and is therefore drawn large across the middle.
#define ARROW_ROW (GIZMO_SIDE / 2 - 1)
#define ARROW_NEAR (GIZMO_SIDE / 2 + 12)
#define ARROW_FAR (GIZMO_SIDE / 2 + 20)

static const uint8_t *pixel_at(voe_render_picture picture, uint32_t x,
			       uint32_t y)
{
	VOE_TEST_CHECK(x < picture.width);
	VOE_TEST_CHECK(y < picture.height);
	return picture.pixels + ((size_t)y * picture.width + x) * 4;
}

// Linear (1, 0, 1), which an sRGB target hands back as exactly (255, 0, 255):
// nought and one are the two values the curve leaves alone.
static bool is_the_gizmos_colour(const uint8_t *pixel)
{
	return pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 255;
}

// Whether the whole of that stretch is the gizmo's colour.
static bool the_arrow_covers_the_stretch(voe_render_picture picture)
{
	for (uint32_t x = ARROW_NEAR; x <= ARROW_FAR; x++) {
		if (!is_the_gizmos_colour(pixel_at(picture, x, ARROW_ROW)))
			return false;
	}
	return true;
}

// A gizmo stands in the middle of the cube it moves, and it is drawn after
// everything else behind a clear of its own — so a pixel of an arrow that is
// inside the cube is the gizmo's colour and not the cube's (ADR-0205).
static void a_gizmo_shows_through_what_it_stands_in(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// The shapes' own pools, one object for the cube and one for each of
	// the gizmo's two draws, and one gizmo's worth of this frame's geometry
	// twice over — what a program that draws one pays for
	// (3d/draw_system.h).
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 3,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = VOE_3D_GIZMO_VERTICES,
		.transient_indices = VOE_3D_GIZMO_INDICES,
		.transient_geometries = 2,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity cube = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_3d_gizmoed gizmo;
	voe_render_picture with_a_gizmo;
	voe_render_picture without_one;
	const uint8_t *cube_pixel;
	const uint8_t *background;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	// Lit from above and in front, so the face of the cube turned towards
	// the camera has a colour of its own to be told from the background by.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, sun,
		(voe_scene_light){ .direction = { 0.0f, -0.6f, -0.8f },
				   .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 1.0f }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at_depth(3.0f)));
	// Dark, so that no pixel of it can be mistaken for the gizmo's colour
	// however the highlight falls.
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.02f, 0.02f, 0.15f } }));
	voe_3d_shape_system_run(world, &shapes);

	gizmo = (voe_3d_gizmoed){
		.entity = cube,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.marked_colour = { 0.0f, 1.0f, 0.0f },
		.marked = VOE_3D_GIZMO_NONE,
		.pixels = GIZMO_PIXELS,
		.size = { GIZMO_SIDE, GIZMO_SIDE },
	};

	// The gizmo stands inside the cube, so every pixel of that stretch
	// would be the cube's without the second depth clear — the arrow is at
	// the cube's own centre depth and the cube's near face is half a metre
	// in front of it.
	with_a_gizmo = a_gizmo_frame(world, device, arena, gizmo);
	VOE_TEST_CHECK(the_arrow_covers_the_stretch(with_a_gizmo));

	// And the same frame with nothing standing on the cube: the middle of
	// that stretch is the cube's colour, which is neither the gizmo's nor
	// the background's — which is what says the arrow above was drawn
	// through a cube that really covers it.
	without_one = a_gizmo_frame(world, device, arena,
				    (voe_3d_gizmoed){ 0 });
	cube_pixel = pixel_at(without_one, (ARROW_NEAR + ARROW_FAR) / 2,
			      ARROW_ROW);
	background = pixel_at(without_one, 0, 0);
	VOE_TEST_CHECK(!is_the_gizmos_colour(cube_pixel));
	VOE_TEST_CHECK(cube_pixel[0] != background[0] ||
		       cube_pixel[1] != background[1] ||
		       cube_pixel[2] != background[2]);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

// A camera whose transform is scaled to nothing on an axis sees nothing: the
// frame says `blind` and its view is zeroed (0223). And a seen one does not.
// No device: framing reads the tables alone.
static void a_camera_scaled_to_nothing_frames_blind(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_ecs_world *seen = a_world(arena);
	voe_ecs_world *flat = a_world(arena);
	voe_3d_frame frame;

	add_a_camera(seen);
	add_the_sun(seen);
	frame = voe_3d_draw_system_frame(seen, size);
	VOE_TEST_CHECK(!frame.blind);

	add_a_camera_scaled(flat, (voe_math_float3){ 1.0f, 1.0f, 0.0f });
	add_the_sun(flat);
	frame = voe_3d_draw_system_frame(flat, size);
	VOE_TEST_CHECK(frame.blind);
	VOE_TEST_CHECK_FLOAT(frame.view.projection.m[1][1], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(frame.view.view.m[3][3], 0.0f, 0.0f);
}

// How many commands one frame of `world`, framed as `frame` with `marker` set
// on it, comes to.
static uint32_t draws_with_a_marker(voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *arena, voe_3d_frame frame,
				    voe_3d_camera_marked marker)
{
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_render_pass_camera camera = { .view = frame.view,
					  .light = frame.light };
	bool drawing = false;
	uint32_t drawn = 0;

	VOE_TEST_CHECK_INT(frame.marker.entity.generation, 0);
	frame.marker = marker;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		drawn = voe_render_frame_draw_count(device);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	return drawn;
}

// A marker on a live camera is one more draw than none, and a marker on a
// zeroed entity is the same as none (0223). The marked camera is a second one,
// added after framing, because framing wants exactly one and _run reads no
// camera table.
static void a_marked_camera_is_one_more_draw(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// The shapes' pools, one object for the cube and one for the marker,
	// and one marker's worth of this frame's geometry (3d/draw_system.h).
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = VOE_3D_CAMERA_MARKER_VERTICES,
		.transient_indices = VOE_3D_CAMERA_MARKER_INDICES,
		.transient_geometries = 1,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity cube = { 0 };
	voe_ecs_entity marked = { 0 };
	voe_3d_frame frame;
	voe_3d_camera_marked marker;
	uint32_t without;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	add_the_sun(world);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at_depth(6.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	voe_3d_shape_system_run(world, &shapes);
	frame = voe_3d_draw_system_frame(world, size);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &marked));
	VOE_TEST_CHECK(voe_scene_transform_add(world, marked, at_depth(3.0f)));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, marked,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	marker = (voe_3d_camera_marked){
		.entity = marked,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.pixels = 2.0f,
		.size = size,
	};
	without = draws_with_a_marker(world, device, arena, frame,
				      (voe_3d_camera_marked){ 0 });
	VOE_TEST_CHECK_INT(without, 1);
	VOE_TEST_CHECK_INT(
		draws_with_a_marker(world, device, arena, frame, marker),
		without + 1);
	marker.entity = (voe_ecs_entity){ 0 };
	VOE_TEST_CHECK_INT(
		draws_with_a_marker(world, device, arena, frame, marker),
		without);

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

	a_camera_scaled_to_nothing_frames_blind(arena);
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
	a_gizmo_shows_through_what_it_stands_in();
	a_marked_camera_is_one_more_draw();
	return voe_test_result();
}
