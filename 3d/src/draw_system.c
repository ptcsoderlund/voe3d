// The draw system: the camera, the sun, the matrices each object is drawn with,
// and one draw per drawable into the frame the loop has opened.
//
// IT WALKS TWO TABLES AND LOOKS THE OTHER COMPONENTS UP BY ENTITY. That is what
// a flat-table ECS with no archetypes costs and it is the trade this engine
// took: each walk is linear and each lookup is one load (ecs/component.h). If
// that ever measures slow it is a later card with a number attached, and nothing
// above this line changes.
//
// THE DEPTH CLEAR IS render'S CALL AND `3d` LEARNS NOTHING FROM MAKING IT. It
// takes no value: the number depth is cleared to lives in `render` beside the
// convention it belongs to, and this folder neither supplies it nor is told it.
// See render/include/render/device.h.
#include <3d/depth_sort.h>
#include <3d/draw_system.h>
#include <3d/gizmo.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/normal_matrix.h>
#include <3d/outline.h>
#include <3d/panel_component.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <base/assert.h>
#include <base/error.h>
#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>

// The world's one light, in the shape `render` takes it. The direction is
// already unit length — scene's light system is the only thing that writes one
// and it normalizes — so this is a copy of three fields and not arithmetic.
static voe_render_light the_sun(const voe_ecs_world *world)
{
	voe_scene_light light = voe_scene_light_rows(world)[0];
	voe_render_light sun = {
		.direction = light.direction,
		.intensity = light.intensity,
		.colour = light.colour,
	};

	return sun;
}

// Whether two entity ids name the same entity. A zeroed id — which is what a
// frame that hides nothing carries — never matches a live one, because
// generation 0 is never handed out (ecs/world.h), so the hidden test needs no
// "is anything hidden at all" branch in front of it.
static bool is_the_same_entity(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

// One entity, held back until its group's turn: everything that group needs in
// order to issue the draw without looking anything up again.
//
// IT IS ONE OF TWO THINGS AND `panel` SAYS WHICH. A mesh draw is a range in
// render's geometry pools plus the record it is shaded with; a panel draw is a
// range of this frame's element buffer plus the one matrix that puts those
// elements where the panel is. They go into the same groups, through the same
// sort, in one order — see draw_group, and see voe_3d_draw_system_run in
// 3d/draw_system.h for why a separate pass for panels would be a bug rather
// than a simplification.
//
// A UNION AND NOT BOTH SETS OF FIELDS, because an entry is a hundred and forty
// bytes of matrices either way and every group is sized for every drawable in
// the world. What the two arms have in common is nothing: no field means the
// same thing in both, so there is nothing to hoist out of them.
struct deferred {
	bool panel;
	union {
		struct {
			voe_render_geometry geometry;
			voe_render_object object;
		} mesh;
		struct {
			// Element millimetres all the way to clip space:
			// projection × view × the transform's matrix × the
			// surface's own plane. Composed once, here, because
			// render takes the finished product and cannot compose
			// it — it has never heard of a camera.
			voe_math_float4x4 transform;
			uint32_t first;
			uint32_t count;
		} elements;
	};
};

// One group of draws that could not be issued as the mesh table was walked,
// because it has to wait for a sort, for the depth clear, or for both.
//
// `depths` AND `order` ARE BOTH THERE OR BOTH ABSENT, AND THAT IS WHAT SAYS
// WHICH KIND OF GROUP THIS IS. A group with them sorts and draws blended; a
// group without them draws solid, in the order it was filled, which is table
// order. One field would do and two is what the sort already takes.
struct group {
	struct deferred *deferred;
	float *depths;
	uint32_t *order;
	uint32_t count;
	// What it was sized for, kept so that hold can say so. It is not read
	// anywhere else: the arrays are filled once and walked once, and `count`
	// is what says how far.
	uint32_t capacity;
};

// The view-space depth of an object's origin, which is the key the blended pass
// sorts on.
//
// THE ORIGIN IS THE WORLD MATRIX'S LAST COLUMN AND THAT IS NOT A SHORTCUT. A
// matrix applied to (0, 0, 0, 1) is its translation, and matrices here are
// row-major — m[row][column] — so the translation is m[0..2][3]. Reading it out
// costs three loads where the multiply would cost sixteen, and it is the same
// number.
//
// MORE NEGATIVE IS FURTHER AWAY, because a camera looks along its own −Z. The
// sign is not corrected here: voe_3d_depth_sort takes the view-space z as it is
// and there is exactly one place the convention is spelled out.
static float view_depth(voe_math_float4x4 view, voe_math_float4x4 world)
{
	voe_math_float4 origin = { world.m[0][3], world.m[1][3], world.m[2][3],
				   1.0f };

	return voe_math_float4x4_mul_float4(view, origin).z;
}

// The record an entity is drawn with, which is the same two matrices and the
// same shading id whichever pass it ends up in.
// The world's shape table, or false when it has none — dev registers none. A
// walk of the types rather than voe_ecs_component_type, which asserts on a key
// nothing registered; once per run, not per object.
static bool shape_type(const voe_ecs_world *world, voe_ecs_type *out)
{
	for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (voe_ecs_component_key(world, type) == &voe_3d_shape_key) {
			*out = type;
			return true;
		}
	}

	return false;
}

// `shape` is the entity's shape or NULL; its colour, opaque, is the object's,
// and anything without one is drawn white — its material's colour as it is.
static voe_render_object object_of(const voe_scene_transform *transform,
				   const voe_3d_material *material,
				   const voe_3d_shape *shape)
{
	voe_render_object object = { 0 };

	object.world = voe_scene_transform_matrix(*transform);
	// One inverse per drawn object per frame, which is the cost of getting a
	// non-uniformly scaled thing lit correctly. It is computed rather than
	// stored for the same reason the world matrix is
	// (scene/transform_component.h): a second copy of the truth is a thing
	// to invalidate. If it ever measures slow it becomes a cached column in
	// the transform table and nothing here changes.
	object.normal = voe_3d_normal_matrix(object.world);
	object.shading = material->shading.index;
	object.colour = shape != NULL ?
				(voe_math_float4){ shape->colour.x,
						   shape->colour.y,
						   shape->colour.z, 1.0f } :
				(voe_math_float4){ 1.0f, 1.0f, 1.0f, 1.0f };

	return object;
}

// Room in the arena for one group, sized for the whole mesh table — see below
// for why that bound and not a measured one. A sorted group gets the two
// arrays the sort works in; an unsorted one has nothing to sort and gets neither.
//
// A TABLE WITH NOTHING IN IT PUSHES NOTHING. voe_base_arena_push asserts on a
// size of nought (base/arena.h), so an empty group is the zeroed struct and the
// fill and the draw below both do nothing with it.
//
// EACH GROUP'S SCRATCH IS SIZED BY THE WHOLE OF BOTH TABLES AND NOT BY WHAT
// LANDS IN IT. Which group a drawable is in is not known until both walks have
// finished, so the bound for each of them is every mesh and every panel there
// is; it is an arena, it is rewound at the end of the frame, and counting first
// would be a second walk to save memory that is given back a millisecond later.
// The overlay's solid group is the one exception and is sized by the meshes
// alone: a panel is blended and cannot land in it.
static struct group group_new(voe_base_arena *arena, uint32_t capacity,
			      bool sorted)
{
	struct group group = { 0 };

	if (capacity == 0)
		return group;

	group.capacity = capacity;
	group.deferred = voe_base_arena_push(
		arena, (size_t)capacity * sizeof(*group.deferred));
	if (sorted) {
		group.depths = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.depths));
		group.order = voe_base_arena_push(
			arena, (size_t)capacity * sizeof(*group.order));
	}
	return group;
}

// Sets one entity aside in its group. The view matrix rather than a depth,
// because only a sorted group has anywhere to put a key — so the work of
// computing one is not done at all for a group drawn in the order it was filled.
//
// `world` IS PASSED RATHER THAN READ OFF THE ENTRY, because the two kinds of
// entry keep their matrices in different places and neither of them keeps a
// world matrix as such — a panel's is already composed into a chain by the time
// it gets here. The key is the same key either way: the view-space depth of the
// object's origin.
static void hold(struct group *group, struct deferred entry,
		 voe_math_float4x4 world, voe_math_float4x4 view)
{
	// A group is sized for every drawable in the world, so a drawable that
	// exists always has room. It is asserted rather than assumed because the
	// size is now arithmetic over two tables: a group sized for nothing has
	// no arrays at all, and one sized for too few would write past an arena
	// push and corrupt whatever came after it. Either is a bug in the three
	// lines below the walk and not something a caller can cause.
	VOE_BASE_ASSERT(group->deferred != NULL &&
				group->count < group->capacity,
			"holding a drawable in a group that was not sized for it — see group_new");

	group->deferred[group->count] = entry;
	if (group->depths != NULL)
		group->depths[group->count] = view_depth(view, world);
	group->count++;
}

// One group's draws: sorted furthest away first through the blended pipeline, or
// in the order it was filled through the solid one. Returns false only when a
// draw was refused, which stops this group and not the frame — the same rule the
// draws issued during the walk follow.
//
// A PANEL IS ISSUED FROM THE SAME LOOP AND IN THE SAME ORDER, WHICH IS THE
// WHOLE OF WHAT THIS CARD CHANGED. Two loops, one over the meshes and one over
// the panels, would be two sorted lists laid end to end — which is not a sort,
// and which comes out right from most angles and wrong from the rest. The
// element draw's pipeline state is the blended pipeline's, so a panel and a
// see-through quad are the same kind of thing to sort and there is no reason to
// tell them apart here.
static bool draw_group(voe_render_device *device, const struct group *group)
{
	bool sorted = group->order != NULL;

	if (sorted)
		voe_3d_depth_sort(group->depths, group->count, group->order);

	for (uint32_t i = 0; i < group->count; i++) {
		const struct deferred *drawn =
			&group->deferred[sorted ? group->order[i] : i];
		bool drawn_ok;

		// A panel ignores `sorted`: there is one element draw and it is
		// blended whichever group it landed in. A panel never reaches an
		// unsorted group anyway — see the walk — and the day one does,
		// drawing it correctly is better than drawing it as a mesh.
		if (drawn->panel)
			drawn_ok = voe_render_frame_draw_elements(
				device, drawn->elements.transform,
				drawn->elements.first, drawn->elements.count);
		else if (sorted)
			drawn_ok = voe_render_frame_draw_blended(
				device, drawn->mesh.geometry,
				drawn->mesh.object);
		else
			drawn_ok = voe_render_frame_draw(device,
							 drawn->mesh.geometry,
							 drawn->mesh.object);

		if (!drawn_ok)
			return false;
	}
	return true;
}

// THE CAMERA AND THE SUN ARE READ THE SAME WAY AND BOTH ARE REQUIRED. Row zero
// of each table, because there is exactly one of each — see
// voe_3d_draw_system_frame in 3d/draw_system.h for why more than one is a
// mistake rather than a choice, and why a world with no sun asserts here rather
// than drawing something black.
voe_3d_frame voe_3d_draw_system_frame(const voe_ecs_world *world,
				      voe_platform_size size)
{
	voe_3d_frame frame;
	voe_scene_camera camera;
	// A window with no area has no aspect ratio. One is as good as any
	// other then: _begin is about to say there is nothing to draw into and
	// nothing reads the matrix, so this only keeps the division below away
	// from a zero.
	float aspect = 1.0f;

	VOE_BASE_ASSERT(world != NULL, "framing no world");
	VOE_BASE_ASSERT(voe_scene_camera_count(world) == 1,
			"a world to draw needs exactly one camera — see 3d/draw_system.h");
	VOE_BASE_ASSERT(voe_scene_light_count(world) == 1,
			"a world to draw needs exactly one light — see 3d/draw_system.h");

	if (size.width > 0 && size.height > 0)
		aspect = (float)size.width / (float)size.height;

	camera = voe_scene_camera_rows(world)[0];
	frame.view.view = voe_scene_camera_view(camera);
	frame.view.projection = voe_3d_projection(camera, aspect);
	// Where the eye is, for the half of the shading that depends on which
	// direction a surface is being looked from. It is the camera's own
	// number and not something recovered from the view matrix.
	frame.view.eye = camera.eye;
	frame.view.reserved = 0.0f;
	frame.light = the_sun(world);
	// Nothing is hidden unless the caller says so, and zero is the way of
	// saying nothing — see `hidden` in 3d/draw_system.h. The same for the
	// outline and the gizmo: a zeroed record outlines nothing and stands no
	// gizmo, so a caller that never sets one of these fields gets the
	// picture it would have got before they existed.
	frame.hidden = (voe_ecs_entity){ 0 };
	frame.outlined = (voe_3d_outlined){ 0 };
	frame.gizmo = (voe_3d_gizmoed){ 0 };

	return frame;
}

// One panel's whole matrix: element millimetres to clip space, through where the
// panel stands and the camera looking at it.
//
// IT IS COMPOSED HERE BECAUSE ONLY THIS FOLDER HAS ALL FOUR PIECES. `render`
// owns what element space is — millimetres, y down from the top-left, centred on
// the surface's origin — and hands that over as one matrix; it has never heard
// of a camera and cannot compose the rest. `scene` owns where the entity is.
// Putting the four together is what `3d` is for.
//
// THE Y SIGN IS IN THE RIGHTMOST FACTOR AND IN NOTHING ELSE HERE. There is no
// minus in front of a y anywhere in this file, and there must not be: the
// engine has one Y flip, in render's viewport, and the element path has one
// negation, in voe_render_element_surface_matrix. A second one here would cancel
// the first and the picture would look right until something was culled.
static voe_math_float4x4 panel_transform(voe_math_float4x4 clip,
					 voe_math_float4x4 model,
					 voe_math_float2 size)
{
	return voe_math_float4x4_mul(
		voe_math_float4x4_mul(clip, model),
		voe_render_element_surface_matrix(size));
}

// Whether a panel's range still describes elements that exist in the frame that
// is open. `submitted` is what this frame has actually been given.
//
// A RANGE IS ONE FRAME'S AND THIS IS WHAT THAT COSTS. The element buffer is
// empty at the top of every frame, so a panel the program did not rebuild is
// pointing at records that are not there — and the honest answer is to not draw
// it, so that "forgotten" looks like "missing" instead of like "wrong".
//
// IT DOES NOT CATCH A STALE RANGE THAT HAPPENS TO FIT, AND NOTHING SHORT OF A
// FRAME STAMP WOULD. A panel holding last frame's numbers, in a frame where some
// other surface has already submitted at least that many records, draws somebody
// else's rectangles and passes every test in here. That is recorded rather than
// fixed: a stamp is a field, a write and a comparison every frame for a case no
// program has hit yet, and the first one that hits it can argue for it.
//
// Written as a subtraction rather than as `first + count`, which is the same
// question with an addition in it that can wrap.
static bool range_is_this_frame_s(voe_3d_panel panel, uint32_t submitted)
{
	return panel.first <= submitted && panel.count <= submitted - panel.first;
}

// One of the gizmo's two meshes, as this frame's geometry and one draw. An
// empty mesh is no draw at all, which is what a gizmo with nothing marked hands
// back for its marked one.
//
// The quads are already in world metres (3d/gizmo.h), so both matrices are the
// identity exactly as the outline's are; the record is the caller's unlit one
// and the colour the caller's, which is the whole of what a handle looks like.
// A refused transient range draws nothing and changes nothing else — render has
// already said so on stderr.
static void draw_gizmo_mesh(voe_render_device *device, voe_3d_gizmo_mesh mesh,
			    voe_3d_material material, voe_math_float3 colour)
{
	voe_render_geometry quads;
	voe_base_error error = VOE_BASE_OK;
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = material.shading.index,
		.colour = { colour.x, colour.y, colour.z, 1.0f },
	};

	VOE_BASE_ASSERT(device != NULL, "drawing a gizmo to no device");
	VOE_BASE_ASSERT(mesh.index_count == 0 ||
				(mesh.vertices != NULL && mesh.indices != NULL),
			"a gizmo mesh of triangles with no arrays behind it");

	if (mesh.index_count == 0)
		return;
	if (voe_render_geometry_create_transient(device, mesh.vertices,
						 mesh.vertex_count,
						 mesh.indices, mesh.index_count,
						 &quads, &error))
		(void)voe_render_frame_draw(device, quads, object);
}

// THE MESHES FIRST AND THEN THE PANELS, AND THE ORDER OF THE TWO WALKS DECIDES
// NOTHING. Every panel is held back and sorted, and the only draws issued during
// a walk are the world's solid meshes — which nothing later can get in front of,
// because the depth buffer resolves them per pixel and the clear has not
// happened yet. So the panels could be walked first and the picture would be
// identical.
//
// THE FRAME'S HIDDEN ENTITY IS TESTED FIRST IN BOTH WALKS, AHEAD OF THE COMPONENT
// LOOKUPS AND OF EVERY SORT. It is two integers compared against the row's owner,
// which is why it goes in front of the lookups rather than in with the other
// skips: a row that is not going to be drawn does not read the components it
// would have been drawn with, and it reaches no group — no matrix, no depth key,
// no sort slot. That ordering is the only cost of the field to a frame that
// hides nothing. See `hidden` in 3d/draw_system.h and ADR-0158 for why there is
// one such entity and not a list.
//
// THE LATER GROUPS ARE BUILT ON THE WAY THROUGH THE FIRST ONE AND NOT BY MORE
// WALKS. An entity that cannot be drawn where it is found has its record and its
// geometry set aside as the mesh table is walked, together with the depth to sort
// on where its group sorts, so the two lookups it costs happen once. What is set
// aside is the record itself rather than the row, because a later pass would
// otherwise look the same two components up again.
//
// THERE ARE FOUR GROUPS AND ONLY ONE OF THEM CAN BE DRAWN AS IT IS FOUND. The
// layer says which side of the depth clear an object is on and the alpha mode
// says which pass it is in on that side, and the two are independent — so the
// walk classifies into world-solid, world-blended, overlay-solid and
// overlay-blended, and issues the first of those immediately. Nothing that comes
// later can get in front of a world-solid object: the depth buffer resolves it
// per pixel, and the clear has not happened yet.
//
// THE OVERLAY HAS A SOLID GROUP FROM THE START AND THAT WAS A CHOICE. Text is
// blended, so the overlay's blended group is what the first user of this needs
// and the solid one could have waited. It is here because the layer and the alpha
// mode are separate axes: leaving the solid group out would mean an opaque
// drawable marked overlay drew in the world instead, silently and correctly
// enough to look like nothing was wrong. It costs one branch in a walk that is
// already happening.
//
// A DRAW THAT IS REFUSED STOPS ITS GROUP BUT NOT THE FRAME. Running out of room
// for objects means the device was made for fewer than this scene has; the loop
// still ends and presents the frame, so a person sees most of the scene and a
// line on stderr rather than a black window — which is also why this returns
// nothing: there is no answer here the loop should act on. The alternative —
// abandoning the recording — would leave the slot's fence unsignalled.
void voe_3d_draw_system_run(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *arena, voe_3d_frame frame)
{
	voe_render_view view = frame.view;
	const voe_3d_mesh *meshes;
	const voe_ecs_entity *owners;
	uint32_t count;
	const voe_3d_panel *panels;
	const voe_ecs_entity *panel_owners;
	uint32_t panel_count;
	// Projection × view, which every panel's own matrix is built onto and
	// which is the same for all of them. The mesh path does not want it: a
	// mesh hands its world matrix to render and the shader multiplies.
	voe_math_float4x4 clip;
	uint32_t submitted;
	// The three groups this frame holds back, and the scratch they are built
	// in. The world's solid objects are the fourth and are drawn as they are
	// found, so they need none.
	struct voe_base_arena_mark mark;
	struct group world_blended = { 0 };
	struct group overlay_solid = { 0 };
	struct group overlay_blended = { 0 };
	voe_ecs_type shapes = { 0 };
	bool has_shapes;
	// Whether the depth buffer has been emptied yet, which the overlay does
	// and the outline needs.
	bool cleared = false;

	VOE_BASE_ASSERT(world != NULL, "drawing no world");
	VOE_BASE_ASSERT(device != NULL, "drawing to no device");
	VOE_BASE_ASSERT(arena != NULL, "drawing with no arena to sort in");
	// The loop opens the pass and this draws into it; the same rule every
	// draw in render applies, asserted here once rather than found by the
	// first draw — or not found at all, in a world with nothing in it.
	VOE_BASE_DEBUG_ASSERT(voe_render_pass_is_open(device),
			      "drawing the world with no pass open — the loop calls voe_render_pass_begin with the frame's camera first; see 3d/draw_system.h");

	has_shapes = shape_type(world, &shapes);
	meshes = voe_3d_mesh_rows(world);
	owners = voe_3d_mesh_entities(world);
	count = voe_3d_mesh_count(world);

	panels = voe_3d_panel_rows(world);
	panel_owners = voe_3d_panel_entities(world);
	panel_count = voe_3d_panel_count(world);

	clip = voe_math_float4x4_mul(view.projection, view.view);
	submitted = voe_render_frame_elements_submitted(device);

	// The mark is taken here, immediately before the first push, so that
	// every path out above it has nothing to give back and the rewind at the
	// bottom is the only one.
	mark = voe_base_arena_mark(arena);

	// Every drawable there is, meshes and panels together, because which
	// group a thing lands in is not known until both walks have finished —
	// the same bound group_new explains, over one more table.
	world_blended = group_new(arena, count + panel_count, true);
	overlay_solid = group_new(arena, count, false);
	overlay_blended = group_new(arena, count + panel_count, true);

	for (uint32_t row = 0; row < count; row++) {
		const voe_scene_transform *transform;
		const voe_3d_material *material;
		struct deferred entry;
		bool blended;

		// The pass's one hidden entity, tested ahead of the two lookups
		// rather than beside the other skips: it is two integers
		// compared, and a row that is not going to be drawn should not
		// pay for the components it would have been drawn with. It
		// reaches no group either — no matrix, no depth key, no sort
		// slot.
		if (is_the_same_entity(owners[row], frame.hidden))
			continue;

		transform = voe_scene_transform_get(world, owners[row]);
		material = voe_3d_material_get(world, owners[row]);
		if (transform == NULL || material == NULL)
			continue;

		entry = (struct deferred){
			.panel = false,
			.mesh = { .geometry = meshes[row].geometry,
				  .object = object_of(
					  transform, material,
					  has_shapes ? voe_ecs_component_get(
							       world, shapes,
							       owners[row]) :
						       NULL) },
		};
		// Cutout is not blended and belongs with the solid ones — it
		// writes depth and needs no order.
		blended = material->alpha_mode == VOE_RENDER_ALPHA_BLENDED;

		// The two axes meet here and nowhere else: the layer says which
		// side of the depth clear this is on, the alpha mode says which
		// pass it is in on that side.
		if (meshes[row].layer == VOE_3D_LAYER_OVERLAY) {
			hold(blended ? &overlay_blended : &overlay_solid, entry,
			     entry.mesh.object.world, view.view);
			continue;
		}
		// Set aside rather than drawn: it has to go after everything
		// solid in the world, and after every see-through thing further
		// away than it is.
		if (blended) {
			hold(&world_blended, entry, entry.mesh.object.world,
			     view.view);
			continue;
		}

		if (!voe_render_frame_draw(device, entry.mesh.geometry,
					   entry.mesh.object))
			break;
	}

	// THE SECOND TABLE, AND EVERY ROW IN IT IS HELD BACK. A panel is drawn
	// by the element pipeline, which is blended, tests depth and writes
	// none — precisely the blended mesh pipeline's state — so a panel
	// belongs in the blended group of its layer and is sorted among the
	// see-through meshes there. There is no panel pass and there must not
	// be: a pass of its own is what makes a see-through quad standing in
	// front of a panel come out behind it, from some angles only.
	for (uint32_t row = 0; row < panel_count; row++) {
		const voe_scene_transform *transform;
		voe_math_float4x4 model;
		struct deferred entry;

		// The same hiding rule as the mesh table's, in the same place
		// and for the same reason: a pass hides an entity, whichever
		// table draws it, and it is tested before the lookup.
		if (is_the_same_entity(panel_owners[row], frame.hidden))
			continue;

		transform = voe_scene_transform_get(world, panel_owners[row]);
		// A panel with nowhere to be, exactly as a mesh with no
		// transform is skipped rather than guessed at.
		if (transform == NULL)
			continue;
		// And a panel nobody rebuilt this frame, which is not drawn
		// rather than drawn wrong — see range_is_this_frame_s.
		if (!range_is_this_frame_s(panels[row], submitted))
			continue;

		model = voe_scene_transform_matrix(*transform);
		entry = (struct deferred){
			.panel = true,
			.elements = { .transform = panel_transform(
					      clip, model, panels[row].size),
				      .first = panels[row].first,
				      .count = panels[row].count },
		};

		// The same key meshes use: the view-space depth of the origin,
		// out of the model matrix rather than out of the composed one,
		// because the composed one is already in clip space and its
		// last column is not a position any more.
		hold(panels[row].layer == VOE_3D_LAYER_OVERLAY ?
			     &overlay_blended :
			     &world_blended,
		     entry, model, view.view);
	}

	// A refused draw in any group stops that group and not the frame, so
	// every return value here is deliberately dropped: the loop still ends
	// and presents the frame.
	(void)draw_group(device, &world_blended);

	// The world is finished and the overlay starts on an empty depth buffer,
	// which is the whole of what a layer is. The colour the world was drawn
	// in is untouched, so the overlay lands on top of that picture rather
	// than on a cleared one — and inside the overlay, depth works exactly as
	// it did in the world.
	//
	// AN EMPTY OVERLAY CLEARS NOTHING. A full-screen depth clear is real work
	// and a world with nothing above it should not pay for one; with both
	// groups empty the clear has nothing to make room for, and the depth
	// image is thrown away at the end of the frame either way. An outline
	// wants the same empty buffer and asks for the clear itself below when
	// the overlay did not.
	if (overlay_solid.count > 0 || overlay_blended.count > 0) {
		voe_render_frame_clear_depth(device);
		cleared = true;

		(void)draw_group(device, &overlay_solid);
		(void)draw_group(device, &overlay_blended);
	}

	// THE OUTLINE IS THE LAST THING IN THE FRAME, WHICH IS WHAT MAKES IT SHOW
	// THROUGH. It goes after the overlay's own groups and after the depth
	// clear, so nothing already drawn can be in front of it — while the
	// entity it belongs to was drawn where it really is, in its own layer,
	// untouched. A world with nothing above it has not cleared depth at that
	// point, so this clears it: the clear is what the outline is drawn
	// against, and it is made once either way. See `outlined` in
	// 3d/draw_system.h and ADR-0203.
	//
	// A REFUSED TRANSIENT RANGE DRAWS NO OUTLINE AND NOTHING ELSE CHANGES.
	// render has already said so on stderr, and the rest of the frame is
	// drawn, ended and presented — the same rule every other draw in here
	// follows.
	if (voe_ecs_entity_alive(world, frame.outlined.entity)) {
		voe_3d_outline_mesh outline;
		voe_render_geometry quads;
		voe_base_error error = VOE_BASE_OK;

		if (voe_3d_outline_quads(world, frame.outlined, view, arena,
					 &outline) &&
		    voe_render_geometry_create_transient(
			    device, outline.vertices, outline.vertex_count,
			    outline.indices, outline.index_count, &quads,
			    &error)) {
			// The quads are already in world space (3d/outline.h),
			// so both matrices are the identity; the record is the
			// caller's unlit one and the colour the caller's, which
			// is the whole of what the outline looks like.
			voe_render_object object = {
				.world = voe_math_float4x4_identity(),
				.normal = voe_math_float4x4_identity(),
				.shading = frame.outlined.material.shading.index,
				.colour = { frame.outlined.colour.x,
					    frame.outlined.colour.y,
					    frame.outlined.colour.z, 1.0f },
			};

			if (!cleared)
				voe_render_frame_clear_depth(device);
			(void)voe_render_frame_draw(device, quads, object);
		}
	}

	// AND THE GIZMO IS AFTER EVEN THE OUTLINE, BEHIND A CLEAR OF ITS OWN.
	// The outline is in the same depth buffer and cuts across an arrow that
	// stands in front of it, so the gizmo is given an empty buffer too:
	// that second clear is the whole of what puts it in front of everything
	// in the picture, and it still occludes itself. Two draws, because the
	// marked handle is a colour of its own (ADR-0205).
	if (voe_ecs_entity_alive(world, frame.gizmo.entity)) {
		const voe_scene_transform *transform =
			voe_scene_transform_get(world, frame.gizmo.entity);
		voe_3d_gizmo gizmo;
		voe_3d_gizmo_mesh plain;
		voe_3d_gizmo_mesh marked;

		// An entity with nowhere to be has nowhere to stand a gizmo,
		// which is skipped rather than guessed at — the same rule a
		// mesh with no transform is drawn by.
		if (transform != NULL) {
			voe_render_frame_clear_depth(device);
			gizmo = voe_3d_gizmo_at(transform->position, view,
						frame.gizmo.size,
						frame.gizmo.pixels);
			if (voe_3d_gizmo_quads(gizmo, frame.gizmo.marked, arena,
					       &plain, &marked)) {
				draw_gizmo_mesh(device, plain,
						frame.gizmo.material,
						frame.gizmo.colour);
				draw_gizmo_mesh(device, marked,
						frame.gizmo.material,
						frame.gizmo.marked_colour);
			}
		}
	}

	// Everything above is this frame's, and the caller's arena is handed
	// back exactly as it arrived. Ending the frame is the loop's.
	voe_base_arena_rewind(arena, mark);
}
