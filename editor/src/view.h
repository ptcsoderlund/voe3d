// A scene view: a camera's picture of the world, drawn into a target of its own
// and shown on a panel (ADR-0147). There can be several, and each is moved on its
// own by a middle-button drag that started over it.
//
// THE CAMERA IS THE EDITOR'S AND NEVER AN ENTITY IN THE WORLD. A view's camera is
// never authored and never saved (ADR-0125): it is where a person happens to be
// looking from, not a thing in the scene, and an entity would put it in the
// Scene list, in the Inspector and one day in the file. So it is a
// voe_scene_camera value held in here, beside an orbit focus and a distance, and
// the pass is handed a camera built from it — not voe_3d_draw_system_frame's,
// which reads a camera out of the world.
//
// THE PICTURE'S SIZE LAGS THE LAYOUT BY ONE FRAME, AND CANNOT NOT. How big a
// panel is comes out of `ui` at voe_ui_frame_end, and the interface is built in
// the pass onto the window — after every view has already been drawn into its
// target. So a view is drawn at the rectangle the dock walk recorded last frame,
// and a resize shows one frame at the old size stretched to the new rectangle.
// The alternative is laying the interface out twice a frame.
//
// A VIEW WHOSE LEAF IS NOT IN THE TREE IS NOT DRAWN. Its target would be filled
// every frame and shown nowhere, which is a pass and a frame's worth of draws
// spent on nothing; and its rectangle is whatever it was when it was last shown,
// which is nothing a picture should be sized by. The loop asks the tree
// (voe_editor_dock_shows_view) before it opens a pass.
//
// THE LIGHT IS THE OPPOSITE: THE WORLD'S AND NEVER THE VIEW'S OWN. Every view
// is lit the same way, by whichever light the world holds — an authored entity,
// selectable, editable and saved, the way `Cube` is (scene.h) — so a view keeps
// no light of its own and its pass is simply handed one; see
// voe_editor_view_pass_camera.
#pragma once

#include <base/error.h>

#include <math/float2.h>
#include <math/float3.h>

#include <render/device.h>

#include <scene/camera_component.h>

#include <ui/layout.h>

#include <stdint.h>

// How many views the editor has room for, and how many it opens with. The room
// is what the device's target and pass capacities are sized from; the two in use
// are the two the default dock tree shows.
#define VOE_EDITOR_VIEWS 4
#define VOE_EDITOR_VIEWS_IN_USE 2

// What `captured` holds when no drag has a view.
#define VOE_EDITOR_VIEW_NONE UINT32_MAX

// One view. `camera.eye`, `camera.yaw` and `camera.pitch` are worked out from
// the focus and the distance by the orbit and are never set on their own.
typedef struct {
	voe_scene_camera camera;
	// The point the camera orbits about and looks at, in metres, and how far
	// the eye stands from it. The distance is always above nought.
	voe_math_float3 focus;
	float distance;

	voe_render_target target;
	voe_render_texture texture;
	// The size the target was last asked to be, in pixels, which is the size
	// its picture is drawn at and the aspect ratio's source.
	uint32_t width;
	uint32_t height;

	// The picture's node this frame, VOE_UI_NODE_NONE when its panel was not
	// drawn, and where that picture came to sit — in the root surface's
	// millimetres, read after voe_ui_frame_end and used the frame after.
	voe_ui_node image;
	voe_ui_rect rect;
} voe_editor_view;

// Every view, and the middle-button drag. `captured` is the view the drag
// started over; `pointer` is where the pointer was last frame, in the root
// surface's millimetres, which is what a drag's travel is measured from.
typedef struct {
	voe_editor_view views[VOE_EDITOR_VIEWS];
	uint32_t count;
	uint32_t captured;
	bool middle_was_down;
	voe_math_float2 pointer;
} voe_editor_views;

// Sets every view in use to its initial camera — view 0 from the front and
// above, view 1 from the side, both looking at the origin — and makes each one a
// target. A startup operation, because making a target is.
//
// False when `render` refused a target, which has already said why on stderr.
[[nodiscard]] bool voe_editor_views_create(voe_editor_views *views,
					   voe_render_device *gpu,
					   voe_base_error *error);

// Asks for the view's target to be its recorded rectangle times
// `pixels_per_millimetre`, rounded, when that differs from what it is. A view
// with no rectangle yet — the first frame — keeps the size it was made with.
// Called before the frame's draw is opened, so the resize is applied by the very
// voe_render_frame_begin that draws it.
void voe_editor_view_fit(voe_editor_view *view, voe_render_device *gpu,
			 float pixels_per_millimetre);

// The camera and `light` the view's pass is opened with, the aspect ratio from
// the view's own size. `light` is the caller's to find — the world's first
// light row, or a light of zero intensity when the world has none — because a
// view knows nothing about the world it is shown (view.h's own header).
voe_render_pass_camera voe_editor_view_pass_camera(const voe_editor_view *view,
						   voe_render_light light);

// The middle-button drag, once per frame, handed the pointer in the root
// surface's millimetres and the three things it reads. A press that starts over a
// view's rectangle captures that view until the release, and the drag moves only
// that view even once the pointer has left it: plain is orbit, `shift` pans the
// focus, `control` dollies the distance. A press that starts over no view
// captures nothing.
void voe_editor_views_drag(voe_editor_views *views, voe_math_float2 pointer,
			   bool middle, bool shift, bool control);

// Forgets every picture's node. Called before the dock walk, because last
// frame's nodes named last frame's tree.
void voe_editor_views_images_clear(voe_editor_views *views);

// Reads where each picture drawn this frame came to sit. Called after
// voe_ui_frame_end and before the frame's arena is rewound, which is the one
// window a node's rectangle may be read in (ui/layout.h).
void voe_editor_views_rects_read(voe_editor_views *views,
				 const voe_ui_context *ui);
