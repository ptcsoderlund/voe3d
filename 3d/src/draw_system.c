// The draw system: the camera, the sun, the matrices each object is drawn with,
// and one draw per drawable into the frame the loop has opened.
//
// IT WALKS THE MESH, MODEL AND PANEL TABLES AND LOOKS THE OTHER COMPONENTS UP
// BY ENTITY; a model row's parts come from the frame's store. That is what
// a flat-table ECS with no archetypes costs and it is the trade this engine
// took: each walk is linear and each lookup is one load (ecs/component.h). If
// that ever measures slow it is a later card with a number attached, and nothing
// above this line changes.
//
// THE DEPTH CLEAR IS render'S CALL AND `3d` LEARNS NOTHING FROM MAKING IT. It
// takes no value: the number depth is cleared to lives in `render` beside the
// convention it belongs to, and this folder neither supplies it nor is told it.
// See render/include/render/device.h.
//
// Each object is drawn with two matrices and a colour. The solid pass runs in
// table order and the blended one furthest first, on each side of the overlay's
// depth clear, over every table; the outline with the collider's lines, and the
// move gizmo, each sit behind a depth clear of their own. The held-back groups
// are draw_group.c's and the marker, outline, collider and gizmo
// draw_marks.c's; this file walks and orders them.
#include "draw_group.h"
#include "draw_marks.h"

#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <base/assert.h>
#include <scene/camera_component.h>
#include <scene/light_component.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

// The direction is the light entity's world rotation's -Z, unit length from
// voe_scene_light_direction, and -Z itself with no transform (ADR-0273); the
// fill is its colour times its strength. No light is the zeroed light, which
// draws lit surfaces black (ADR-0287, 0290 point 1).
voe_render_light voe_3d_draw_system_light(const voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "lighting no world");
	uint32_t count = voe_scene_light_count(world);
	VOE_BASE_ASSERT(count <= 1,
			"a world to draw has at most one light — see 3d/draw_system.h");
	if (count == 0)
		return (voe_render_light){ 0 };

	voe_scene_light light = voe_scene_light_rows(world)[0];
	voe_ecs_entity lit = voe_scene_light_entities(world)[0];
	voe_render_light sun = {
		.direction =
			voe_scene_transform_get(world, lit) ?
				voe_scene_light_direction(
					voe_scene_transform_world(world, lit)
						.rotation) :
				(voe_math_float3){ 0.0f, 0.0f, -1.0f },
		.fill = voe_math_float3_scale(light.fill_colour,
					      light.fill_intensity),
		.intensity = light.intensity,
		.colour = light.colour,
	};

	return sun;
}

// THE CAMERA IS REQUIRED AND THE DIRECTIONAL LIGHT IS NOT. Row zero of the
// camera table, because there is exactly one; the light is
// voe_3d_draw_system_light's, which is zeroed and draws black for none
// (ADR-0287) — see voe_3d_draw_system_frame in 3d/draw_system.h for
// why more than one of either is a mistake rather than a choice.
voe_3d_frame voe_3d_draw_system_frame(const voe_ecs_world *world,
				      voe_platform_size size, float lag)
{
	voe_3d_frame frame;
	voe_scene_camera lens;
	voe_scene_transform pose;
	// A window with no area has no aspect ratio. One is as good as any
	// other then: _begin is about to say there is nothing to draw into and
	// nothing reads the matrix, so this only keeps the division below away
	// from a zero.
	float aspect = 1.0f;

	VOE_BASE_ASSERT(world != NULL, "framing no world");
	VOE_BASE_ASSERT(lag >= 0.0f && lag <= 1.0f,
			"a lag is a fraction of one step — see 3d/draw_system.h");
	VOE_BASE_ASSERT(voe_scene_camera_count(world) == 1,
			"a world to draw needs exactly one camera — see 3d/draw_system.h");

	if (size.width > 0 && size.height > 0)
		aspect = (float)size.width / (float)size.height;

	lens = voe_scene_camera_rows(world)[0];
	VOE_BASE_ASSERT(voe_scene_transform_get(world,
					voe_scene_camera_entities(world)[0]) != NULL,
			"a camera with no transform — the camera needs one (0222)");
	// Where the camera was `lag` of a step ago, as everything it sees is
	// drawn (0254).
	pose = voe_scene_transform_between(world, voe_scene_camera_entities(world)[0],
					   lag);
	// A pose that sees nothing leaves `view` zeroed and says so: _run draws
	// no world for it (0223).
	frame.view = (voe_render_view){ 0 };
	frame.blind = !voe_3d_view(pose, lens, aspect, &frame.view);
	// The view is about the camera's own position; every matrix in _run is
	// taken about the same point (ADR-0250).
	frame.eye = pose.position;
	frame.lag = lag;
	frame.light = voe_3d_draw_system_light(world);
	// No shadow until voe_3d_draw_system_shadows fills one (ADR-0258).
	frame.shadow = (voe_render_shadow){ 0 };
	// Nothing is hidden unless the caller says so, and zero is the way of
	// saying nothing — see `hidden` in 3d/draw_system.h. The same for the
	// outline and the gizmo: a zeroed record outlines nothing and stands no
	// gizmo, so a caller that never sets one of these fields gets the
	// picture it would have got before they existed.
	frame.hidden = (voe_ecs_entity){ 0 };
	frame.outlined = (voe_3d_outlined){ 0 };
	frame.gizmo = (voe_3d_gizmoed){ 0 };
	frame.marker = (voe_3d_camera_marked){ 0 };
	frame.sun = (voe_3d_sun_marked){ 0 };
	frame.models = NULL;

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

// The loaded entry a model row draws from, NULL when it draws nothing: no
// store, no transform, or a path the store lacks or failed on.
static const voe_3d_model_entry *drawn_model(const voe_ecs_world *world,
					    const voe_3d_models *models,
					    uint32_t row)
{
	const voe_3d_model_entry *entry;

	VOE_BASE_ASSERT(row < voe_3d_model_count(world), "a model row past the table");
	if (models == NULL ||
	    voe_scene_transform_get(world, voe_3d_model_entities(world)[row]) == NULL)
		return NULL;
	entry = voe_3d_models_find(models, voe_3d_model_rows(world)[row].path);
	if (entry == NULL || !entry->loaded)
		return NULL;
	VOE_BASE_ASSERT(entry->part_count <= VOE_3D_MODEL_PARTS,
			"a model with more parts than the store holds");
	return entry;
}

// Every loaded part of every model row, which the world's blended group is
// sized for as it is for every mesh and panel. Nought with no store.
static uint32_t model_part_count(const voe_ecs_world *world,
				 const voe_3d_models *models)
{
	uint32_t parts = 0;

	VOE_BASE_ASSERT(world != NULL, "counting parts in no world");
	if (models == NULL)
		return 0;
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *entry = drawn_model(world, models, row);

		parts += entry != NULL ? entry->part_count : 0;
	}
	VOE_BASE_ASSERT(parts <= voe_3d_model_count(world) * VOE_3D_MODEL_PARTS,
			"more parts than the rows can wear");
	return parts;
}

// Each model row's parts, as the mesh walk does a world-layer mesh: solid ones
// drawn now, blended ones held in `world_blended`. White, since a model has no
// shape (ADR-0191). False when a draw is refused, which stops this walk only.
static bool draw_models(voe_ecs_world *world, voe_render_device *device,
			const voe_3d_frame *frame,
			struct voe_3d_draw_group *world_blended)
{
	const voe_ecs_entity *owners;

	VOE_BASE_ASSERT(world != NULL && frame != NULL && world_blended != NULL,
			"drawing models with no world, frame or group");
	if (frame->models == NULL)
		return true;
	owners = voe_3d_model_entities(world);
	for (uint32_t row = 0; row < voe_3d_model_count(world); row++) {
		const voe_3d_model_entry *model;
		voe_scene_transform drawn;

		if (voe_3d_draw_group_is_the_same_entity(owners[row], frame->hidden))
			continue;
		model = drawn_model(world, frame->models, row);
		if (model == NULL)
			continue;
		drawn = voe_scene_transform_between(world, owners[row], frame->lag);
		for (uint32_t part = 0; part < model->part_count; part++) {
			const voe_3d_model_part *piece = &model->parts[part];
			struct voe_3d_deferred entry = {
				.panel = false,
				.mesh = { .geometry = piece->geometry,
					  .object = voe_3d_draw_group_object_of(
						  &drawn, &piece->material, NULL,
						  frame->eye) },
			};

			if (piece->material.alpha_mode == VOE_RENDER_ALPHA_BLENDED)
				voe_3d_draw_group_hold(world_blended, entry,
						       entry.mesh.object.world,
						       frame->view.view);
			else if (!voe_render_frame_draw(device, entry.mesh.geometry,
							entry.mesh.object))
				return false;
		}
	}
	return true;
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
	struct voe_3d_draw_group world_blended = { 0 };
	struct voe_3d_draw_group overlay_solid = { 0 };
	struct voe_3d_draw_group overlay_blended = { 0 };
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
	// A camera that sees nothing draws no world (3d/draw_system.h, `blind`).
	if (frame.blind)
		return;

	has_shapes = voe_3d_draw_group_shape_type(world, &shapes);
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
	// the same bound voe_3d_draw_group_new explains.
	world_blended = voe_3d_draw_group_new(
		arena, count + panel_count + model_part_count(world, frame.models),
		true);
	overlay_solid = voe_3d_draw_group_new(arena, count, false);
	overlay_blended = voe_3d_draw_group_new(arena, count + panel_count, true);

	for (uint32_t row = 0; row < count; row++) {
		const voe_scene_transform *transform;
		voe_scene_transform drawn;
		const voe_3d_material *material;
		struct voe_3d_deferred entry;
		bool blended;

		// The pass's one hidden entity, tested ahead of the two lookups
		// rather than beside the other skips: it is two integers
		// compared, and a row that is not going to be drawn should not
		// pay for the components it would have been drawn with. It
		// reaches no group either — no matrix, no depth key, no sort
		// slot.
		if (voe_3d_draw_group_is_the_same_entity(owners[row], frame.hidden))
			continue;

		transform = voe_scene_transform_get(world, owners[row]);
		material = voe_3d_material_get(world, owners[row]);
		if (transform == NULL || material == NULL)
			continue;
		// Where it was `lag` of a step ago (0254).
		drawn = voe_scene_transform_between(world, owners[row], frame.lag);

		entry = (struct voe_3d_deferred){
			.panel = false,
			.mesh = { .geometry = meshes[row].geometry,
				  .object = voe_3d_draw_group_object_of(
					  &drawn, material,
					  has_shapes ? voe_ecs_component_get(
							       world, shapes,
							       owners[row]) :
						       NULL,
					  frame.eye) },
		};
		// Cutout is not blended and belongs with the solid ones — it
		// writes depth and needs no order.
		blended = material->alpha_mode == VOE_RENDER_ALPHA_BLENDED;

		// The two axes meet here and nowhere else: the layer says which
		// side of the depth clear this is on, the alpha mode says which
		// pass it is in on that side.
		if (meshes[row].layer == VOE_3D_LAYER_OVERLAY) {
			voe_3d_draw_group_hold(blended ? &overlay_blended : &overlay_solid, entry,
			     entry.mesh.object.world, view.view);
			continue;
		}
		// Set aside rather than drawn: it has to go after everything
		// solid in the world, and after every see-through thing further
		// away than it is.
		if (blended) {
			voe_3d_draw_group_hold(&world_blended, entry, entry.mesh.object.world,
			     view.view);
			continue;
		}

		if (!voe_render_frame_draw(device, entry.mesh.geometry,
					   entry.mesh.object))
			break;
	}

	// The model rows' parts, world layer only (0277 point 3); a refused
	// draw stops this walk and not the frame.
	(void)draw_models(world, device, &frame, &world_blended);

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
		struct voe_3d_deferred entry;

		// The same hiding rule as the mesh table's, in the same place
		// and for the same reason: a pass hides an entity, whichever
		// table draws it, and it is tested before the lookup.
		if (voe_3d_draw_group_is_the_same_entity(panel_owners[row], frame.hidden))
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

		model = voe_scene_transform_matrix(
			voe_scene_transform_between(world, panel_owners[row],
						    frame.lag),
			frame.eye);
		entry = (struct voe_3d_deferred){
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
		voe_3d_draw_group_hold(panels[row].layer == VOE_3D_LAYER_OVERLAY ?
			     &overlay_blended :
			     &world_blended,
		     entry, model, view.view);
	}

	// A refused draw in any group stops that group and not the frame, so
	// every return value here is deliberately dropped: the loop still ends
	// and presents the frame. The camera and sun markers are solid and in
	// the world's depth (0223, 0274), so they go with the solids, before the
	// blended group.
	voe_3d_draw_marks_camera(world, device, arena, frame);
	voe_3d_draw_marks_sun(world, device, arena, frame);
	(void)voe_3d_draw_group_draw(device, &world_blended);

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

		(void)voe_3d_draw_group_draw(device, &overlay_solid);
		(void)voe_3d_draw_group_draw(device, &overlay_blended);
	}

	// The outline and the collider's lines behind one depth clear, then the
	// gizmo behind its own (3d/src/draw_marks.h); the first is the overlay's
	// when it made one.
	cleared = voe_3d_draw_marks_outline(world, device, arena, frame, cleared);
	voe_3d_draw_marks_collider(world, device, arena, frame, cleared);
	voe_3d_draw_marks_gizmo(world, device, arena, frame);

	// Everything above is this frame's, and the caller's arena is handed
	// back exactly as it arrived. Ending the frame is the loop's.
	voe_base_arena_rewind(arena, mark);
}
