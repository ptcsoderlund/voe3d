// The system that turns the tables into draws. It draws the world into the pass
// the loop has opened: it finds the camera and the sun, works out the matrices,
// and issues one draw per drawable — solid ones in table order, see-through ones
// afterwards and furthest away first.
//
//     voe_3d_frame frame = voe_3d_draw_system_frame(world, size, lag);
//     if (!voe_render_frame_begin(gpu, size, &drawing))
//             break;                          // the GPU stopped answering
//     if (drawing) {
//             (void)voe_3d_draw_system_point_lights(world, &frame, scratch);
//             (void)voe_3d_draw_system_light_blockers(world, &frame, scratch);
//             (void)voe_3d_draw_system_lights(world, &frame, scratch);
//             (void)voe_3d_draw_system_shadows(world, gpu, &frame);
//                                             // false: unshadowed, said on stderr
//             voe_render_pass_camera camera = voe_3d_draw_system_camera(&frame);
//             ...                             // build what changes this frame
//             if (voe_render_pass_begin(gpu, VOE_RENDER_TARGET_WINDOW, &camera)) {
//                     voe_3d_draw_system_run(world, gpu, scratch, frame);
//                     voe_render_pass_end(gpu);
//             }
//             if (!voe_render_frame_end(gpu))
//                     break;
//     }
//
// THE LOOP OWNS THE FRAME AND THIS SYSTEM DRAWS INTO IT (ADR-0098). Begin and
// end are the program's calls, made from its loop exactly as render's own header
// shows, and the phases inside a frame are ordered there: begin; the point
// lights (voe_3d_draw_system_point_lights); the light blockers and the lights
// after the first (voe_3d_draw_system_light_blockers, then
// voe_3d_draw_system_lights); the shadow passes, the sun's one per cascade and
// the lamps' one (voe_3d_draw_system_shadows); build what changes this frame —
// a readout, a user interface — which is the only world write after the
// systems have run; a pass opened with the frame's camera
// (voe_3d_draw_system_camera); this system walks the world; the pass closed;
// end, which presents. That order exists because geometry built for one frame
// (voe_render_geometry_create_transient) can only be built once a frame is open
// and has to be in the mesh table before this walk; a draw system that opened
// and closed the frame itself left no moment for that.
//
// ONE DRAW PER MESH, FROM THE CPU. No instancing, no indirect command buffer, no
// culling, and no extract step into a second layout. Each of those is a change
// to this one loop and each is a later card; what this card guarantees is that
// the data is already in the shape they need — geometry in shared pools, one
// record per object in one buffer, textures by id. A panel is the exception that
// proves it: one draw per panel however many rectangles are on it, because the
// element path was built for exactly that.
//
// GROUPING IS THE ENGINE'S AND NEVER THE USER'S. There is no component, flag or
// authoring concept for putting objects into batches by hand: the engine knows
// the grouping key — the same pipeline — better than a person does, and the day
// this loop groups anything it will do it from what is already in the tables.
//
// EVERY TABLE IT WALKS HAS TO BE REGISTERED, INCLUDING ONES THE WORLD HAS NO
// ROWS IN. This walks meshes and panels, so a world that never builds a panel
// still calls voe_3d_panel_register — asking for an unregistered component type
// asserts in `ecs` rather than reading as an empty table, and an empty table is
// what a world with no panels wants. Registering what a system reads is part of
// building a world that can be drawn, which is the same rule the camera and the
// light tables already state below.
#pragma once

#include <3d/camera_marker.h>
#include <3d/collider_marker.h>
#include <3d/gizmo.h>
#include <3d/gizmo_rings.h>
#include <3d/models.h>
#include <3d/outline.h>
#include <3d/point_light_marker.h>
#include <3d/sun_marker.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <render/device.h>

// The move gizmo a pass draws, after everything else and behind a clear of its
// own (ADR-0205). Where it stands, what it wears and how big it is on the
// picture; the triangles themselves are voe_3d_gizmo_quads'.
//
// A PASS MAY DRAW ONE MOVE GIZMO, AND IT IS THE LAST THING OF ALL (ADR-0205).
// It goes after the outline and behind a second depth clear of its own: the
// outline is in the same depth buffer and would otherwise cut across an arrow
// standing in front of it. So the gizmo meets an empty depth buffer too, and
// nothing already drawn can be in front of any part of it — which is what makes
// every handle aimable at wherever the thing it moves happens to be.
//
// IT IS ONE ENTITY AND NOT A LIST, for the reason `outlined` is one: a gizmo
// stands on what is selected, there is one selection per view, and a set of
// them is a larger decision made when something needs it (rule 10). A zeroed
// entity, a dead one and one with no transform each draw nothing, which is what
// no selection looks like — so a caller that never sets the field loses nothing.
//
// THE TWO COLOURS AND THE MARKED HANDLE ARE THE CALLER'S. Which handle is under
// the pointer or held is the editor's question (ADR-0205), and whose theme a
// gizmo wears is the editor's business exactly as an outline's colour is: this
// draws the handles at rest in `colour` and the marked one in `marked_colour`,
// and knows nothing else about either.
//
// THE QUADS COME FROM voe_3d_gizmo_quads, OR voe_3d_gizmo_rings_quads WHEN
// `rings` IS SET (0274), AND GO INTO THIS FRAME'S TRANSIENT POOL. A program that
// draws a gizmo sizes its voe_render_capacities' transient vertices and indices
// from the larger of VOE_3D_GIZMO_VERTICES and VOE_3D_GIZMO_RING_VERTICES, and
// of the two _INDICES, and counts two more transient ranges and two more
// objects per pass whichever is drawn — the handles
// at rest and the marked one are two draws, because a drawn object's colour is
// one record per draw (ADR-0191). A refused transient range draws no gizmo and
// leaves the rest of the frame alone, as the outline's does.
typedef struct {
	// Where the gizmo stands: the entity whose transform it is on, zeroed
	// for none. A zeroed entity is never a live one, a dead one and one
	// with no transform draw nothing, and all three are what no selection
	// looks like.
	voe_ecs_entity entity;
	// The unlit record the quads wear — voe_3d_shapes' outline material,
	// the same one the silhouette above wears.
	voe_3d_material material;
	// The handles at rest, linear, and the whole of what they look like
	// because the record is unlit (ADR-0205).
	voe_math_float3 colour;
	// The one handle under the pointer or held, in its own colour because
	// a drawn object's colour is one record per draw (ADR-0191).
	voe_math_float3 marked_colour;
	// Which handle that is, VOE_3D_GIZMO_NONE for none. The editor's
	// question, answered with voe_3d_gizmo_hit — or, with `rings`, the ring
	// voe_3d_gizmo_rings_hit answers.
	voe_3d_gizmo_handle marked;
	// False draws the move gizmo's arrows; true the rotate gizmo's three
	// rings (0274), in the same two draws at the same place in the order.
	bool rings;
	// How many pixels one arrow's shaft covers, at any distance.
	float pixels;
	// The size of that picture, in pixels — the height is what the size is
	// worked out against, as the outline's width is.
	voe_platform_size size;
} voe_3d_gizmoed;

// The scene camera a pass draws as a marker (0223): its box and frustum as
// line quads, voe_3d_camera_marker_quads', in the world layer. Only an
// editor's view sets one; a game's frame never does.
typedef struct {
	// The camera entity, zeroed for none. A zeroed entity, a dead one and
	// one without a camera and a transform draw nothing.
	voe_ecs_entity entity;
	// The unlit record the quads wear — the outline's, voe_3d_shapes'.
	voe_3d_material material;
	// Linear, and the whole of what the marker looks like: the outline's
	// colour when selected, the gizmo's rest colour otherwise (0223).
	voe_math_float3 colour;
	// How wide a line is on the picture, in pixels, at any distance.
	float pixels;
	// The size of that picture, in pixels.
	voe_platform_size size;
} voe_3d_camera_marked;

// The sun a pass draws as a marker (0274): its circle and arrow as line quads,
// voe_3d_sun_marker_quads', in the world layer. Only an editor's view sets one.
typedef struct {
	// The light entity, zeroed for none. A zeroed entity, a dead one and
	// one without a light and a transform draw nothing.
	voe_ecs_entity entity;
	// The unlit record the quads wear — the outline's, voe_3d_shapes'.
	voe_3d_material material;
	// Linear, and the whole of what the marker looks like: the outline's
	// colour when selected, the gizmo's rest colour otherwise (0274).
	voe_math_float3 colour;
	// How wide a line is on the picture, in pixels, at any distance.
	float pixels;
	// The size of that picture, in pixels.
	voe_platform_size size;
} voe_3d_sun_marked;

// The point lights a pass draws as markers (0320 point 7): three wire circles
// each, voe_3d_point_light_marker_quads', in the world layer inside the world's
// depth. Only an editor's view sets it. When `shown`, every point light with a
// transform is marked, the `selected` one in `selected_colour` and the rest in
// `colour`, as two transient geometries — the rest together, the selected one
// alone — so the pool needs VOE_3D_POINT_LIGHT_MARKER_VERTICES and _INDICES per
// light, two ranges and two objects; a pool too small draws none, as the
// outline does.
typedef struct {
	// False marks none, which is what a zeroed record is.
	bool shown;
	// The lamp drawn in `selected_colour`, zeroed for none.
	voe_ecs_entity selected;
	// The unlit record the quads wear — the outline's, voe_3d_shapes'.
	voe_3d_material material;
	// Linear: the gizmo's rest colour, for every lamp but the selected one.
	voe_math_float3 colour;
	// Linear: the outline's colour, for the selected lamp (0274's rule).
	voe_math_float3 selected_colour;
	// How wide a line is on the picture, in pixels, at any distance.
	float pixels;
	// The size of that picture, in pixels.
	voe_platform_size size;
} voe_3d_point_lights_marked;

// The collider a pass draws as lines (0253), voe_3d_collider_marker_quads'.
// It is drawn after the outline, behind the outline's depth clear, with the
// outline's material and colour (`frame.outlined`), so it shows through what
// stands in front. Only an editor's view sets one. The quads go into this
// frame's transient pool, sized from VOE_3D_COLLIDER_MARKER_VERTICES and
// _INDICES, one more range and one more object; a pool too small draws
// nothing, as the outline does.
typedef struct {
	// The entity whose collider is drawn, zeroed for none. A zeroed entity,
	// a dead one and one without a collider (voe_physics_shape_of) draw
	// nothing.
	voe_ecs_entity entity;
	// How wide a line is on the picture, in pixels, at any distance.
	float pixels;
	// The size of that picture, in pixels.
	voe_platform_size size;
} voe_3d_collider_marked;

// What one frame is drawn with, in the shape `render` takes it: the camera and
// the sun. Computed once by voe_3d_draw_system_frame, handed to
// voe_render_pass_begin by the loop and back to voe_3d_draw_system_run for its
// sort, so the camera is worked out exactly once per frame.
typedef struct {
	voe_render_view view;
	voe_render_light light;
	// Where the sun's shadow maps are; zero is none. _frame leaves it
	// zeroed, voe_3d_draw_system_shadows fills it, and the loop hands it
	// to the pass beside `view` and `light` (ADR-0258).
	voe_render_shadow shadow;
	// The point lights the pass is lit by (0320); zero is none. _frame
	// leaves it zeroed, voe_3d_draw_system_point_lights fills it before
	// voe_3d_draw_system_shadows reads its slots, and the loop hands it to
	// the pass camera beside `shadow`.
	voe_render_point_lights points;
	// The light blockers the pass's light is kept out of (0347); zero is
	// none. _frame leaves it zeroed, voe_3d_draw_system_light_blockers
	// fills it, and the loop hands it to the pass camera beside `points`.
	voe_render_light_blockers blockers;
	// The directional lights after the first (0357 point 1), `more_count`
	// of them; none is NULL and 0. _frame leaves them zeroed,
	// voe_3d_draw_system_lights fills them, and voe_3d_draw_system_shadows
	// sets their shadow records.
	voe_render_directional_light *more_lights;
	uint32_t more_count;
	// The camera's world position, in double, that `view` is about
	// (ADR-0250): every object's matrix, the sort and every mark is taken
	// about this point. _frame sets it; a frame built by hand sets it too.
	voe_math_double3 eye;
	// A PASS MAY HIDE ONE ENTITY, AND THE CASE IS A SURFACE SHOWING THIS
	// PASS'S OWN PICTURE (ADR-0158). A quad whose base colour texture is
	// the target being drawn into would be an image read while it is
	// written — undefined in Vulkan, and what voe_render_target_create's
	// debug check asserts on — so the pass that fills the target sets
	// `hidden` to that quad and the pass that shows it does not. One entity
	// and not a list, a mask or a layer: there is one such surface per
	// target, and a visibility system is a larger decision made when
	// something needs it (rule 10).
	//
	// The one entity this pass does not draw, zero for none — a zeroed
	// voe_ecs_entity is never a live one, because generation 0 is never
	// handed out (ecs/world.h), so a caller that never sets this loses
	// nothing. It is one entity and not a list or a mask on purpose: the
	// case is the surface showing this pass's own target and there is one
	// of those per target (ADR-0158, and the paragraph above).
	voe_ecs_entity hidden;
	// A PASS MAY OUTLINE ONE ENTITY, AND IT IS DRAWN AFTER EVERYTHING ELSE
	// (ADR-0203). Last means after the overlay's own two groups, and
	// therefore after the depth clear — and where there is no overlay to
	// clear for, the outline clears: it meets an empty depth buffer either
	// way, and nothing already drawn can cover it, which is the whole of
	// what makes it show through a thing standing in front of the entity.
	// The entity itself is not moved, not redrawn and not touched — it is
	// drawn where it really is, in its own layer, by the rules
	// voe_3d_draw_system_run states below, and the silhouette is a separate
	// set of quads around it.
	//
	// THE QUADS COME FROM voe_3d_outline_quads AND GO INTO THIS FRAME'S
	// TRANSIENT POOL. So a program that outlines anything sizes its
	// voe_render_capacities' three transient numbers from
	// VOE_3D_OUTLINE_VERTICES and VOE_3D_OUTLINE_INDICES, and counts one
	// more object per pass — the outline is one more draw and one more
	// record. The record the quads wear is the caller's unlit one
	// (voe_3d_shapes' `outline`) and the colour is the caller's too: whose
	// theme an outline is drawn in is the editor's business and not this
	// folder's.
	//
	// A REFUSED TRANSIENT RANGE DRAWS NO OUTLINE AND LEAVES THE FRAME
	// ALONE. A transient pool too small for this silhouette is a capacity
	// chosen too small: render says so on stderr and this draws nothing,
	// exactly as every other refused draw in here stops its own group and
	// no more.
	//
	// The one entity this pass draws a silhouette around, zeroed for none —
	// a zeroed entity is never a live one for the same reason `hidden`'s
	// is, so a caller that never sets this loses nothing. It is one entity
	// and not a list for the same reason `hidden` is one: what is outlined
	// is what is selected, there is one selection per view, and a set of
	// them is a larger decision made when something needs it (rule 10).
	voe_3d_outlined outlined;
	// The one entity this pass stands a move gizmo on, zeroed for none —
	// one entity and not a list for the same reason `outlined` is one, and
	// drawn last of all behind a clear of its own (ADR-0205, and
	// voe_3d_gizmoed above).
	voe_3d_gizmoed gizmo;
	// A PASS MAY MARK ONE SCENE CAMERA (0223). It is drawn in the world
	// layer inside the world's depth, not behind the outline's clear, so
	// what stands in front of the camera hides its lines. A zeroed record
	// draws nothing. One entity and not a list for the reason `outlined`
	// is one. The quads go into this frame's transient pool, sized from
	// VOE_3D_CAMERA_MARKER_VERTICES and _INDICES, one more range and one
	// more object; a pool too small is a capacity chosen too small and
	// draws nothing, as the outline does. A pass whose `view` is the marked
	// camera's own is the caller's to avoid.
	voe_3d_camera_marked marker;
	// A PASS MAY MARK ONE SUN (0274), drawn as the camera marker is: in the
	// world layer inside the world's depth. A zeroed record, a dead entity
	// and one without a light or a transform draw nothing. The quads go into
	// this frame's transient pool, sized from VOE_3D_SUN_MARKER_VERTICES and
	// _INDICES, one more range and one more object; a pool too small draws
	// nothing, as the outline does.
	voe_3d_sun_marked sun;
	// EVERY POINT LIGHT, MARKED (0320 point 7), drawn as the sun's marker is
	// and right after it (voe_3d_point_lights_marked above).
	voe_3d_point_lights_marked point_lights;
	// The one entity whose collider this pass draws as lines, zeroed for
	// none (voe_3d_collider_marked above).
	voe_3d_collider_marked collider;
	// THE SELECTED LIGHT BLOCKER'S BOX AS LINES (0347 point 5), the entity
	// zeroed for none. Only an editor's view sets it. The box is
	// voe_3d_light_blocker_shape's at the frame's lag, drawn as the
	// collider's lines are: after them, behind the outline's clear, in its
	// material and colour. A zeroed entity, a dead one and one with no
	// blocker or transform draw nothing. One more range and object, sized
	// from VOE_3D_COLLIDER_MARKER_VERTICES and _INDICES; a pool too small
	// draws nothing, as the outline does.
	voe_3d_collider_marked light_blocker;
	// TRUE WHEN THE CAMERA SEES NOTHING (0223): its transform has no
	// inverse, a scale of nought on an axis, so voe_3d_view refused it and
	// `view` is zeroed. _run draws no world for a blind frame. Zero means
	// seen, so a frame a caller builds by hand keeps drawing.
	bool blind;
	// How far back from the last step this frame is drawn, as a fraction of
	// a step (0254): 0 is now, 1 the step before. _frame sets it; _run
	// draws every mesh and panel from voe_scene_transform_between at it.
	float lag;
	// The model store the frame's model rows are drawn from, NULL for none
	// (0277 point 3): with NULL no model draws and the model table is not
	// read. _frame leaves it NULL; the caller sets it.
	const voe_3d_models *models;
	// The target this frame draws to, whose probe volume the shadows call
	// begins, captures and relights (0326 point 8). Zero, VOE_RENDER_TARGET_WINDOW, is the
	// window, so a caller that sets nothing is unchanged; _frame sets that.
	voe_render_target target;
} voe_3d_frame;

// The least metres about a moved caster whose probes in the volume are captured
// again (0326 point 4); on a coarser grid the radius is 3 of its cells (0332
// point 4).
#define VOE_3D_BOUNCE_REACH 6.0f

// The share of D, the 17th casting lamp's distance, where a point light's
// shadow starts fading (0325 point 5).
#define VOE_3D_POINT_SHADOW_FADE 0.75f

// Texels a side of one point-shadow cube face: the `point_shadow_size` a
// device is given (0325 point 1), 25 MiB a frame slot.
#define VOE_3D_POINT_SHADOW_TEXELS 256u


// The camera and the sun out of the tables, for the frame about to begin. `size`
// is the window's and gives the aspect ratio; a size with no area gets an aspect
// of one, because _begin is about to say there is nothing to draw into and the
// matrix is never read. `hidden`, `outlined`, `gizmo`, `marker`, `sun`,
// `point_lights`, `collider`, `light_blocker`, `points` and `blockers` all come
// back zeroed and `models` NULL — hiding, outlining, standing a gizmo, marking a
// camera, a sun or the point lights, drawing a collider or a blocker's box,
// lighting by point lights, keeping
// light out of blockers and drawing models are the caller's choice and it
// sets the field on the answer, and `more_lights` and `more_count` come back
// zeroed for voe_3d_draw_system_lights to fill. Asserts on a world
// without exactly one camera; with no light the
// frame's light is the zeroed one and lit surfaces draw black — see below.
//
// THE CAMERA AND THE SUN ARE COMPUTED ONCE, BEFORE THE PASS, AND HANDED BACK.
// The pass needs the view and the light and this system needs the view again for
// its sort, so voe_3d_draw_system_frame works both out from the tables and the
// loop passes the result to each. The camera is a lens and its pose is its
// entity's transform (0222), which the camera needs and this asserts on; the two
// become `view` through voe_3d_view (3d/projection.h), and a pose that sees
// nothing sets `blind` instead. Neither the camera nor the projection moves
// out of this folder for that: what the loop holds is an answer, not a way of
// computing one. Nothing in _run reads the camera table.
//
// IT NEEDS EXACTLY ONE CAMERA, AND MORE THAN ONE IS A BUG RATHER THAN A CHOICE.
// A second camera means a second target and a second frame, which is the card
// that introduces a viewport; until then a world with two of them has a mistake
// in it and this says so.
//
// THE FIRST LIGHT ROW IS `light` (0357 point 1). The ones after it, up to
// VOE_RENDER_DIRECTIONAL_LIGHTS in all, are voe_3d_draw_system_lights'. A
// world with none is
// still a choice and never asserts (ADR-0287): its light is the zeroed one, so
// lit surfaces draw black while unlit materials, text and panels draw as
// before, and it casts no shadow. The light comes from
// voe_3d_draw_system_light below. Registering the table is part of building a
// world that can be drawn, rows or none; see scene/light_system.h.
//
// THE WINDOW'S SIZE IS THE ASPECT RATIO'S SOURCE AND THE LOOP PASSES IT. Its
// type arrives from `render`'s public header, which is the folder that speaks to
// the window's answer; `3d` does not include `platform` and does not ask a window
// anything.
//
// THE FRAME IS DRAWN A LAG BEHIND THE LAST STEP (0254, ADR-0065 point 4). `lag`
// is 1 − banked time / step, between 0 and 1: the camera's pose, and every mesh
// and panel _run draws, are voe_scene_transform_between at it, so motion stepped
// at a fixed rate is smooth at any frame rate. A program that does not step —
// the editor, dev, a test — passes 0 and draws what is. The outline, the gizmo
// and the camera marker keep the current transform: they are the editor's, and
// it passes 0.
voe_3d_frame voe_3d_draw_system_frame(const voe_ecs_world *world,
				      voe_platform_size size, float lag);

// The world's first light row, in table order, in the shape `render` takes it;
// with none, the zeroed
// light, every field nought and `unshaded` too, which draws lit surfaces black
// (ADR-0287, 0290). The rows after it are voe_3d_draw_system_lights' (0357
// point 1). The direction is the light entity's
// transform rotation's -Z (scene's voe_scene_light_direction), -Z with no
// transform; the fill is fill_colour times fill_intensity (ADR-0273).
//
// IT IS PUBLIC SO EVERY PICTURE OF A SCENE MEANS THE SAME BY "NO LIGHT", AND
// SO A VIEW CAN CHOOSE ANOTHER. Dev's monitor lights its passes with it rather
// than reading the light table, so a scene with no light draws black there as
// in the game's frame; the editor lights such a world with its own preview
// light by handing its passes a different light (ADR-0287).
voe_render_light voe_3d_draw_system_light(const voe_ecs_world *world);

// Fills `frame->points` from every point light whose entity has a transform
// (0320 point 6): its world place at `frame->lag`, about `frame->eye` in float
// (ADR-0250); its range and its falloff as authored (0322); its colour times
// its intensity; its `bounces` and `bounce_strength` copied as authored (0326
// point 1). A light of intensity 0 is left out, and past
// VOE_RENDER_POINT_LIGHTS the rest are left out in table order. The array is in `arena`. A world with no point light table
// fills none and returns true. False with `points` zeroed when the arena is
// full — which base's arena never is, since its push aborts rather than fails
// (base/arena.h), so today it is always true.
//
// ONE CALL PER PASS'S FRAME, AND EVERY PICTURE OF A WORLD MAKES IT. The editor's
// views and the game both call it, so a lamp lights the same in both.
//
// THE NEAREST SIXTEEN CASTING LAMPS GET A SHADOW SLOT (0325 point 5). Among the
// kept lights whose row has `cast_shadows` (intensity above nought, as every
// kept one has), ranked by the eye's distance to the light's sphere,
// max(0, |position| − range), the nearest 16 get `shadow` 1..16 nearest first
// and `shadow_strength` 1 − smoothstep(FADE·D, D, distance), FADE being
// VOE_3D_POINT_SHADOW_FADE and D the 17th's distance, unbounded with 16 or
// fewer, so every one is at 1. One at strength 0 gets no slot; every other
// light's `shadow` is 0. It cannot pop: D moves continuously with the eye and
// the 16th's strength is 0 by the time the 17th overtakes it, so a shadow has
// faded out before its lamp loses its slot, with no state across frames.
[[nodiscard]] bool voe_3d_draw_system_point_lights(const voe_ecs_world *world,
						   voe_3d_frame *frame,
						   voe_base_arena *arena);

// Fills `frame->blockers` from every light blocker whose entity has a transform
// (0347 point 2), in table order, at most VOE_RENDER_LIGHT_BLOCKERS: its box
// (voe_3d_light_blocker_shape) at `frame->lag`, about `frame->eye` in float
// (ADR-0250), its rows by render/device.h's formula and its sphere the box's
// centre and |half|. A box with a half of nought on any axis is left out. The
// array is in `arena`.
//
// BLOCK AND THE SUN'S MASK (0350 point 2, 0353): bit i of `walls` or `indoors`
// is kept record i when its row's Block is Direct or Fill, an All in neither,
// so a box left out shifts the bits after it. `sun` is
// voe_render_light_blockers_mask over the kept records at the light row's
// entity's world place at `frame->lag`, about `frame->eye`; no light table, no
// light row or no transform on it is 0. A world with no light blocker table
// fills none and returns true; the arena's push never fails (base/arena.h), so
// today it is always true.
//
// CALLED BEFORE voe_3d_draw_system_shadows, which hands the same blockers to
// the bounce, and EVERY PICTURE OF A WORLD MAKES IT, so a house is dark inside
// in the editor's views as in the game.
[[nodiscard]] bool voe_3d_draw_system_light_blockers(const voe_ecs_world *world,
						     voe_3d_frame *frame,
						     voe_base_arena *arena);

// Fills `frame->more_lights` and `more_count` from the 2nd to the 4th light
// rows in table order (0357 point 1), each light made as
// voe_3d_draw_system_light makes the first. Each entry's `blockers` is the
// mask of `frame->blockers` holding its entity's world place at `frame->lag`,
// as `sun` is the first light's; no transform is 0. Its `bounces` and
// `bounce_strength` are its row's and its shadow is zeroed. Rows past the
// VOE_RENDER_DIRECTIONAL_LIGHTS-th are left out. The array is in `arena`; one
// light, none or no light table fills none.
//
// CALLED AFTER voe_3d_draw_system_light_blockers, whose masks it reads, AND
// BEFORE voe_3d_draw_system_shadows, which sets the shadows. EVERY PICTURE OF
// A WORLD MAKES IT, so a moon lights the editor's views as the game. The
// arena's push never fails (base/arena.h), so as the blockers' call it is
// always true today.
[[nodiscard]] bool voe_3d_draw_system_lights(const voe_ecs_world *world,
					     voe_3d_frame *frame,
					     voe_base_arena *arena);

// The pass camera of `frame`: its view, light, shadow, point lights, blockers
// and the lights after the first as `more`.
voe_render_pass_camera voe_3d_draw_system_camera(const voe_3d_frame *frame);

// The directional lights' shadow passes for `frame`, opened between the frame's
// begin and the view's pass (ADR-0258, 0357 point 3). Each casting light of the
// frame, `light` and each of `more_lights` — shaded and of some strength, the
// frame not blind, and its light row casting (`cast_shadows`, 0324) — casts:
// in table order each takes a slot, up to what voe_render_shadow_lights_ready
// holds, fits four cascades to `frame->view`, `frame->eye` and its direction at
// VOE_3D_SHADOW_TEXELS, opens one shadow pass per cascade at layer
// slot × 4 + cascade that draws every caster at the frame's lag, and sets its
// record (`shadow`, or the entry's) with that slot. A light that does not cast,
// or past what is ready, keeps a zeroed record and draws unshadowed; the
// device's array grows the first frame it is short, so the next frame has its
// slot. A world with no light row casts nothing, so neither does the editor's
// preview light. A caller that never calls it draws as before, with no shadow.
// False when a pass or a draw is refused, with render's line on stderr; every
// record is zeroed then, so the view draws unshadowed. Called with a pass
// open, it asserts.
//
// CASTERS ARE WORLD-LAYER, LIT, OPAQUE OR CUTOUT MESHES WITH A TRANSFORM, the
// frame's `hidden` left out; a cutout casts as solid, and panels and marks cast
// nothing. A model part in `frame->models` casts under the same rule, its
// material the part's. A mesh whose shape, or a model whose row, has
// `cast_shadows` false is no caster, though still drawn and shadowed; nor is a
// model row with `fade` at or above 1, and a fading one casts as ever (0336). The device needs `shadow_size` VOE_3D_SHADOW_TEXELS, and room for 4
// passes and 4 × the drawn objects more per casting light per view: every
// caster is drawn once into each of its cascades.
//
// THEN ONE POINT-SHADOW PASS (0325 point 6), whether or not the sun cast: when
// voe_render_point_shadows_ready and a light in `frame->points` has a slot, it
// opens the pass for those lights and draws the same casters into it, so
// `frame->points` is filled before this call. That is one more pass and at most
// one more object per caster, a device's `point_shadow_size`
// VOE_3D_POINT_SHADOW_TEXELS; false as before when render refuses.
//
// THEN THE PROBE BOUNCE (0326 point 8), whether or not the sun cast, when the
// frame is not blind and either a sun bounces — the world's first light row
// with `bounces` of 1 or more and the frame's light of some strength and not
// `unshaded`, or any of `more_lights` with `bounces` of 1 or more, some
// strength and shaded — or a light in `frame->points` has `bounces` of 1 or
// more. Every sun goes to the bounce (0357 point 1). It fits the probe volume to
// the still casters' box, not about `frame->eye`, so the camera never moves it
// (0331, 0332); begins `frame->target`'s bounce, opens capture passes while
// render has probes to capture, drawing the casters into each, and relights.
// That is up to VOE_RENDER_BOUNCE_CAPTURE_PASSES more passes and that many more
// objects per caster. On a frame that relights, for each sun that bounces and
// casts it opens one more pass, and one more object per caster: that sun's
// relight map (0357 point 4), fitted to the volume (voe_3d_bounce_grid_sun) and never the cascades,
// so the relight's sun shadow does not follow the view (0329). The first frame after a light starts bouncing builds the
// volume and shows none. Stale spheres are marked where a caster moved this
// step (lag 1 against lag 0), so a world without a previous table marks none.
// When nothing bounces nothing is begun, built or drawn (0316); false as before
// when any call fails. The bounce is handed `frame->blockers` (0347 point 4), so
// voe_3d_draw_system_light_blockers comes first, and a blocker that changes
// relights.
[[nodiscard]] bool voe_3d_draw_system_shadows(voe_ecs_world *world,
					      voe_render_device *device,
					      voe_3d_frame *frame);

// Draws every entity that has a mesh, a transform and a material — and every
// entity that has a panel, a transform and a range this frame submitted — into
// the pass that is open, with `frame` the answer voe_3d_draw_system_frame gave
// for it — the same camera the pass was opened with. `frame.hidden`, when it
// names a live entity, is the one thing left out, of either table and either
// layer, and `frame.outlined`, when it names one, is outlined after everything
// else is drawn — and `frame.gizmo`, when it names one with a transform, is the
// gizmo drawn after that, its arrows or its rings; `frame.marker`, when it names
// a live camera with a transform, and `frame.sun`, when it names a live light
// with a transform, are drawn with the world, and `frame.collider`, when it names one
// with a collider, as lines after the outline, and `frame.light_blocker`, when
// it names one with a blocker and a transform, as its box's lines after those. Calling it with no pass open is the caller's bug
// and asserts.
//
// TWO KINDS OF DRAWABLE, AND THEY ARE SORTED TOGETHER. A mesh is a range in
// render's geometry pools (3d/mesh_component.h); a panel is a range of this
// frame's element buffer and the size of the surface it is on
// (3d/panel_component.h). A panel is blended, tests depth and writes none, which
// is exactly the blended mesh pipeline's state — so a panel goes into the
// blended group of its layer and is sorted among the see-through meshes, and
// there is no panel pass. A pass of its own would hardcode "the interface draws
// last", and a panel bolted to a wall would then be visible through the wall.
//
// AN ENTITY IS DRAWN WHEN IT HAS ALL THREE OF MESH, TRANSFORM AND MATERIAL. A
// mesh with no transform has nowhere to be and a mesh with no material has no
// record to shade with, so both are skipped rather than guessed at — a guessed
// material would draw with somebody else's record, which looks like a bug in the
// importer. A PANEL NEEDS TWO OF THE THREE: a panel and a transform. It has no
// material and cannot have one — an element carries its own colour, which is the
// whole reason an interface is one draw command. A MODEL ROW WITH A TRANSFORM
// WHOSE ENTRY IN `frame.models` IS LOADED draws each part (0277 point 3): in the
// world layer, white in its object record, solid or blended by the part's
// material. An empty path, a path the store lacks, a failed entry and a NULL
// store draw nothing. A ROW'S `fade` (0336 point 3): at or above 1 the model
// is gone and not drawn; above 0 and below 1 every part is blended at 1 − fade
// with its part's twin, `faded`, sorted with the world's blended group; at or
// below 0, or not a number, it draws as above. THE ONE EXCEPTION IS THE FRAME'S `hidden`: the entity it
// names is not drawn however complete it is, mesh, panel, model or particle.
//
// EACH LIVE PARTICLE IS ONE BLENDED DRAW FROM `frame.models` (ADR-0298 point
// 6): its emitter's texture's picture entry, the soft dot at "" for an empty
// one, part 1 when it glows and part 0 otherwise, facing the camera, sized and
// coloured at its age, sorted with the rest of the world's blended group and
// casting no shadow. A NULL store, or no loaded picture, draws none of that
// emitter's; a world with no emitter table draws none. The device needs room
// for VOE_3D_EMITTER_PARTICLES objects per emitter per world pass.
//
// EACH WATER WITH A TRANSFORM AND A WAVES ROW IS ONE BLENDED DRAW (ADR-0305
// point 7): the store's quad and water record (voe_3d_models_water), turned to
// face +Y and scaled to the row's width and length, its colour, waves and sky in
// the object record, sorted with the world's blended group and casting no
// shadow. A NULL store, or one with no water record, draws none; one object per
// water per world pass. WHEN ANY WATER IS HELD, THE PASS'S DEPTH IS COPIED ONCE
// (voe_render_frame_copy_depth), after the world's solids and the markers and
// before its blended group, so the water fades against what stands under it; a
// refused copy stops that group as a refused draw does, said on stderr.
//
// A DRAWN OBJECT'S COLOUR IS ITS SHAPE'S, OR WHITE (ADR-0191). The per-object
// record carries a colour the shader multiplies into the material's base
// colour: the entity's shape colour with alpha 1 when it has a shape, and
// (1, 1, 1, 1) — the material as it is — when it has none or the world
// registered no shapes. A fading model's part is (1, 1, 1, 1 − fade), its
// row's fade as the alpha. It is in the object's record and not the material's
// because an object's record is written every frame and a material's once.
//
// EVERY DRAWN OBJECT CARRIES A NORMAL MATRIX AS WELL AS A WORLD MATRIX, and this
// loop is what computes it — one inverse per object per frame. See
// 3d/normal_matrix.h for why a normal cannot be carried by the world matrix, and
// what it looks like on screen when it is.
//
// TWO PASSES, AND WHICH ONE AN ENTITY IS IN IS ITS MATERIAL'S ALPHA MODE. Opaque
// and cutout go first, in table order, which is safe because the depth buffer
// resolves them per pixel. Blended goes second, sorted furthest away first,
// because blending is not commutative and the blended pipeline does not write
// depth — so the order these are issued in *is* the picture. See
// 3d/depth_sort.h for the sort and the sign it turns on, and
// render/include/render/device.h for why the depth write is off.
//
// THE SORT IS PER OBJECT AND NOT PER TRIANGLE. One key per entity: the
// view-space depth of its origin. Two see-through things that interpenetrate,
// and a long thin one seen end-on, come out wrong, and that is the trade taken
// rather than an oversight — per-fragment sorting and order-independent
// transparency are both refused by name.
//
// AND TWO LAYERS, WHICH RUN ACROSS THE TWO PASSES RATHER THAN INSIDE THEM. A
// drawable of either kind says whether it is in the world or above it
// (3d/mesh_component.h, 3d/panel_component.h);
// the whole world is drawn, its two passes in that order, then depth is cleared,
// then the overlay is drawn — its own two passes, in the same order, by the same
// rules. So there are four groups and one depth clear between the second and the
// third, and nothing about blending, the sort or shading differs between the
// halves. An overlay object keeps a real position in metres and is seen through
// the same camera: this layer is "always on top" and is not screen space. The one
// orthographic projection in this engine is the sun's cascades' (ADR-0258).
//
// THE OVERLAY STILL OCCLUDES ITSELF, WHICH IS WHY THIS IS A CLEAR AND NOT THE
// DEPTH TEST TURNED OFF. Two overlapping panels above the world have to hide
// each other the way two panels in the world do; a layer that stopped testing
// depth would draw them in table order and look correct until there were two of
// them. A depth range split was the other alternative and it spends the precision
// the reversed-depth convention exists to buy.
//
// THE LAYER SAYS WHEN, NEVER HOW. It does not imply unlit, does not imply
// blended and does not change a material — whether a surface is lit is `unlit`
// on its material, and an overlay object with a lit material is lit. Text is
// both unlit and in the overlay, and those are two decisions that happen to
// agree.
//
// A BEGIN THAT SAYS THERE IS NOTHING TO DRAW INTO IS THE LOOP'S CASE. A window
// with no area, or a swapchain that has just gone stale, comes back from _begin
// with `drawing` false; the loop then builds nothing, runs nothing here and does
// not call _end. Calling this with no pass open is the caller's bug and asserts,
// the same rule render applies to a draw outside a pass.
//
// NOTHING IN HERE FAILS IN A WAY THE LOOP SHOULD STOP FOR, WHICH IS WHY IT
// RETURNS NOTHING. A draw the device refuses — more objects than it was made
// for, an id naming nothing — stops its group, says so on stderr and leaves the
// frame to be ended and presented as usual; the device having stopped answering
// is _begin's and _end's to report, and both return false when it has.
//
// `arena` is scratch for this frame's sorts, the groups they order and the
// outline's, the collider's, the blocker's and the gizmo's quads, and nothing survives the call: it is rewound to the mark
// this took on the way in, on every path out.
//
// IT WANTS AN ARENA BECAUSE THE SORTS NEED SOMEWHERE TO WORK. Working memory is
// an arena passed in and there is no default one (rule 11), so the caller hands
// over scratch; this rewinds every frame to exactly what it was handed, keeps
// nothing, and a caller may pass the same arena it uses for anything else. What
// it takes is bounded by the number of drawables in the world, each loaded part
// of each model row, each live particle and each water counted as one — three groups' worth of it, because which group a drawable is in is not known until the walks
// have finished and each of them therefore has room for all of them.
void voe_3d_draw_system_run(voe_ecs_world *world, voe_render_device *device,
			    voe_base_arena *arena, voe_3d_frame frame);
