// The panel component and what the draw system does with it: the table, the
// range that is one frame's, and the claim that a panel is sorted among the
// see-through meshes rather than drawn in a pass of its own.
//
// THE INTERESTING HALF CANNOT BE SEEN AND IS MEASURED INSTEAD. This folder must
// not read a target back — that would mean reaching into render's source
// directory for its internals, which the folder rule forbids — so the order the
// draws came out in has to be observed some other way. What is observable from
// here is voe_render_frame_draw_count, and the lever that turns an order into a
// count is the rule the draw system already has: a refused draw stops its group
// and not the frame.
//
// SO A MESH WITH A GEOMETRY ID NAMING NOTHING IS THE PROBE. render refuses such
// a draw, which stops whatever group it is in at that point. Put it furthest
// away in a group and everything sorted after it is silently not drawn; put it
// nearest and everything before it is. The draw count then says exactly where
// the panel came in the order — which is the claim, and it is a claim two sorted
// lists laid end to end would fail.
//
// THE TWO CASES ARE BOTH NEEDED. A panel drawn in a pass of its own after the
// meshes passes the case where the panel is furthest away; a panel drawn in a
// pass of its own before them passes the case where it is nearest. Only running
// both says there is one list.
//
// THE TABLE HALF NEEDS NO GRAPHICS CARD AND RUNS FIRST, so a build box with no
// Vulkan still checks the component itself. The ordering half skips and says so,
// for the reason 3d/tests/import.c gives at length: a box with no Vulkan is the
// box and not this engine.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/float2.h>
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

// Four elements is more than any surface below submits, and two objects is
// enough that running out of them is never what stops a group — the probe is
// the refused id and nothing else.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 2,
	.shadings = 4,
	.elements = 4,
	.passes = 1,
};

// A geometry id this device never handed out. render refuses a draw naming it,
// with a line on stderr, and that refusal is this file's instrument.
static const voe_render_geometry NO_SUCH_MESH = { .index = 4242,
						  .generation = 4242 };

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 8,
		.intent_types = 8,
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
// further away and the depths below read as distances.
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

// One panel entity: a transform, and a surface of `count` elements starting at
// `first`.
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

// One see-through mesh naming a geometry that does not exist — the probe. It is
// blended so that it lands in a sorted group, which is the only kind of group a
// panel can be in.
static void add_the_probe(voe_ecs_world *world, voe_render_device *device,
			  float back, voe_3d_layer layer)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_material material = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 0.5f },
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
	};
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_3d_material_upload(device, &material, &error));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at_depth(back)));
	VOE_TEST_CHECK(voe_3d_mesh_add(world, entity,
				       (voe_3d_mesh){ .geometry = NO_SUCH_MESH,
						      .layer = layer }));
	VOE_TEST_CHECK(voe_3d_material_add(world, entity, material));
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

// Opens a frame, puts `records` elements in it, walks the world and hands back
// how many draw commands came out. Nought when there was nothing to draw into,
// which a caller distinguishes by never asking for nought.
static uint32_t draws_of_a_frame(voe_ecs_world *world,
				 voe_render_device *device,
				 voe_base_arena *arena, uint32_t records)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size);
	voe_render_pass_camera camera = { .view = frame.view,
					  .light = frame.light };
	bool drawing = false;
	uint32_t drawn;

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

// The table itself: what goes in comes out, the range is the one thing that
// changes, and everything else travels through a change to it.
static void the_table_holds_a_range_a_size_and_a_layer(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity panel = add_a_panel(world, 3.0f, VOE_3D_LAYER_OVERLAY, 7,
					   3);
	voe_ecs_entity bare = { 0 };
	const voe_3d_panel *row;

	row = voe_3d_panel_get(world, panel);
	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK_INT(row->first, 7);
	VOE_TEST_CHECK_INT(row->count, 3);
	VOE_TEST_CHECK_FLOAT(row->size.x, 240.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(row->size.y, 135.0f, 1e-6f);
	VOE_TEST_CHECK_INT(row->layer, VOE_3D_LAYER_OVERLAY);

	// The one write that happens every frame, and the two fields it must not
	// touch. A _set taking a whole row would let a caller lose the layer by
	// forgetting it, which is the mistake this shape makes impossible.
	VOE_TEST_CHECK(voe_3d_panel_set_range(world, panel, 0, 12));
	row = voe_3d_panel_get(world, panel);
	VOE_TEST_CHECK(row != NULL);
	if (row == NULL)
		return;
	VOE_TEST_CHECK_INT(row->first, 0);
	VOE_TEST_CHECK_INT(row->count, 12);
	VOE_TEST_CHECK_FLOAT(row->size.x, 240.0f, 1e-6f);
	VOE_TEST_CHECK_INT(row->layer, VOE_3D_LAYER_OVERLAY);

	// An entity with no panel is refused rather than given one.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));
	VOE_TEST_CHECK(!voe_3d_panel_set_range(world, bare, 0, 1));
	VOE_TEST_CHECK(voe_3d_panel_get(world, bare) == NULL);

	// And the table reads back as a table, which is how the draw system
	// walks it.
	VOE_TEST_CHECK_INT(voe_3d_panel_count(world), 1);
	VOE_TEST_CHECK(voe_3d_panel_rows(world) != NULL);
	VOE_TEST_CHECK(voe_3d_panel_entities(world)[0].index == panel.index);
}

// A panel nobody rebuilt this frame is not drawn, and the same panel with a
// range this frame actually holds is. The buffer is empty at the top of every
// frame, so the first of those is what "stale" means.
static void a_stale_range_is_skipped(voe_base_arena *arena,
				     voe_render_device *device)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity panel = add_a_panel(world, 3.0f, VOE_3D_LAYER_WORLD, 0,
					   2);

	add_a_camera(world);
	add_the_sun(world);

	// Nothing submitted: the range names two records that do not exist.
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 0), 0);

	// Two submitted and the range rebuilt to match: drawn, one command.
	VOE_TEST_CHECK(voe_3d_panel_set_range(world, panel, 0, 2));
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2), 1);

	// And a range that starts inside the frame but runs off the end of it,
	// which is the shape a stale range has when this frame is smaller than
	// the last one. Skipped here rather than refused by render, which is the
	// difference between a silent skip and a line on stderr every frame.
	VOE_TEST_CHECK(voe_3d_panel_set_range(world, panel, 1, 2));
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2), 0);

	// A count of nought is not stale, and draws nothing without being
	// skipped for the wrong reason: render records no command for it.
	VOE_TEST_CHECK(voe_3d_panel_set_range(world, panel, 2, 0));
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2), 0);
}

// ONE SORTED LIST OVER BOTH KINDS, WHICH IS THE CARD'S CLAIM. A panel and a
// see-through mesh in the same layer, and the probe says which of the two was
// issued first. Furthest away goes first, so the panel drawing depends entirely
// on which side of the probe it sorted to.
static void a_panel_and_a_blended_mesh_are_one_sorted_list(
	voe_base_arena *arena, voe_render_device *device)
{
	{
		// The panel further away than the probe: sorted first, drawn,
		// and then the probe stops the group. One command, and it is
		// the panel's.
		voe_ecs_world *world = a_world(arena);

		add_a_camera(world);
		add_the_sun(world);
		(void)add_a_panel(world, 9.0f, VOE_3D_LAYER_WORLD, 0, 2);
		add_the_probe(world, device, 4.0f, VOE_3D_LAYER_WORLD);

		VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2), 1);
	}
	{
		// The panel nearer than the probe: the probe is sorted first,
		// is refused, and the group stops before the panel. No
		// commands at all — which is what says the two are in one list.
		// A panel drawn in a pass of its own, before or after the
		// meshes, gives one here.
		voe_ecs_world *world = a_world(arena);

		add_a_camera(world);
		add_the_sun(world);
		(void)add_a_panel(world, 4.0f, VOE_3D_LAYER_WORLD, 0, 2);
		add_the_probe(world, device, 9.0f, VOE_3D_LAYER_WORLD);

		VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2), 0);
	}
}

// A panel's layer puts it on its own side of the depth clear, and the probe says
// so: a probe furthest away in the world stops the world's blended group, so a
// panel in the world does not draw and a panel in the overlay does.
static void a_panel_lands_in_the_group_its_layer_names(voe_base_arena *arena,
						       voe_render_device *device)
{
	voe_ecs_world *world = a_world(arena);

	add_a_camera(world);
	add_the_sun(world);
	// Furthest of the three, so it is issued first and stops the world's
	// blended group where it stands.
	add_the_probe(world, device, 10.0f, VOE_3D_LAYER_WORLD);
	(void)add_a_panel(world, 5.0f, VOE_3D_LAYER_WORLD, 0, 1);
	(void)add_a_panel(world, 2.0f, VOE_3D_LAYER_OVERLAY, 1, 1);

	// Exactly one command: the overlay's panel, in a group the world's
	// refusal cannot reach. Nought would mean the overlay panel had been put
	// in the world's group — where, being nearest, it would sort last and
	// never be reached.
	VOE_TEST_CHECK_INT(draws_of_a_frame(world, device, arena, 2), 1);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	voe_base_error error = VOE_BASE_OK;

	the_table_holds_a_range_a_size_and_a_layer(arena);

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

	// Every refusal below says so on stderr, on purpose: the probe is a
	// draw that names a mesh this device never handed out, and those lines
	// are the instrument working rather than something going wrong.
	printf("the lines about a mesh this device did not hand out are the probe; see the header\n");

	a_stale_range_is_skipped(arena, device);
	a_panel_and_a_blended_mesh_are_one_sorted_list(arena, device);
	a_panel_lands_in_the_group_its_layer_names(arena, device);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
