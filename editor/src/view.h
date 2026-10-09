// A scene view: a camera's picture of the world, drawn into a target of its own
// and shown on a panel (ADR-0147). There can be several, and each is moved on its
// own by a middle-button drag that started over it.
//
// THE CAMERA IS THE EDITOR'S AND NEVER AN ENTITY IN THE WORLD. A view's camera is
// never authored and never saved (ADR-0125): it is where a person happens to be
// looking from, not a thing in the scene, and an entity would put it in the
// Scene list, in the Inspector and one day in the file. So it is a pose — an
// eye, a yaw and a pitch, beside an orbit focus and a distance — and a lens held
// in here, turned into a render view by voe_3d_view the way the world's camera
// is (0223), and not read out of the world by voe_3d_draw_system_frame.
//
// THE PICTURE'S SIZE LAGS THE LAYOUT BY ONE FRAME, AND CANNOT NOT. How big a
// panel is comes out of `ui` at voe_ui_frame_end, and the interface is built in
// the pass onto the window — after every view has already been drawn into its
// target. So a view is drawn at the rectangle the dock walk recorded last frame,
// and a resize shows one frame at the old size stretched to the new rectangle.
// The alternative is laying the interface out twice a frame.
//
// A VIEW WHOSE LEAF IS NOT IN THE TREE, OR IS CLOSED (0363), IS NOT DRAWN. Its
// target would be filled every frame and shown nowhere, which is a pass and a
// frame's worth of draws spent on nothing; and its rectangle is zeroed by the
// walk, so no click finds it. The loop asks the root
// (voe_editor_dock_shows_view) before it opens a pass.
//
// WHERE A CLICK LANDS IS LAST FRAME'S RECTANGLE, FOR THE SAME REASON THE
// PICTURE'S SIZE IS. A picture's rectangle comes out of `ui` only after the
// frame it was laid out in, so a click is read against the rectangle the dock
// walk recorded last frame — exactly what the middle-button drag is already
// measured against. The aspect ratio the ray is built with is the picture's own
// `width` and `height`, which is what it was actually drawn with, and not the
// rectangle's; see voe_editor_views_under.
//
// THE LIGHT IS THE OPPOSITE: THE WORLD'S AND NEVER THE VIEW'S OWN. Every view
// is lit the same way, by the lights the world holds — authored entities,
// selectable, editable and saved, the way `Cube` is (scene.h) — so a view keeps
// no light of its own: its pass is handed the world's first light as `light`
// (voe_editor_view_pass_camera) and the others come with the frame (0357
// point 1). A world with no directional light is shown by
// the editor's preview light instead (0287), never an entity; see
// voe_editor_view_light.
//
// A VIEW IS MOVED THREE WAYS: the middle-button drag and the right-button fly
// (0233), each holding the view it started over until its button comes up and
// neither starting while the other holds one, and the glide to a focus (0371).
// A drag or fly starting on a view ends its glide where it is; the views opening
// on the camera end every glide. None is saved — not to the world, not to undo,
// not to settings.
//
// THE PREVIEW IS WHAT THE WORLD'S CAMERA SEES, AT ONE FIXED SIZE (0223): a
// 16:9 target of VOE_EDITOR_PREVIEW_WIDTH by _HEIGHT pixels, made beside the
// views' and never resized, because the game window's aspect is not known to
// the editor and the marker's frustum is drawn at the same 16:9. Nothing but
// voe_editor_view_passes_preview draws into it, and only while the selected
// entity has a camera; `preview_shown` is that frame's answer. A view can be
// opened on that camera too. The colours a view is drawn with are here, a light
// blocker's faint one among them.
#pragma once

#include <3d/bounce_casters.h>

#include <base/error.h>

#include <ecs/world.h>

#include <math/double3.h>
#include <math/float2.h>
#include <math/float3.h>

#include <render/device.h>

#include <scene/camera_component.h>

#include <ui/layout.h>
#include <ui/theme.h>

#include <stdint.h>

// How many views the editor has room for, and how many it opens with. The room
// is what the device's target and pass capacities are sized from; the two in use
// are the two the default dock tree shows.
#define VOE_EDITOR_VIEWS 4
#define VOE_EDITOR_VIEWS_IN_USE 2

// What `captured` holds when no drag has a view.
#define VOE_EDITOR_VIEW_NONE UINT32_MAX

// The preview's size in pixels, named once (0223).
#define VOE_EDITOR_PREVIEW_WIDTH 480
#define VOE_EDITOR_PREVIEW_HEIGHT 270

// How long a glide takes, in seconds (0371 point 3).
#define VOE_EDITOR_VIEW_GLIDE_SECONDS 0.25f

// One view. `eye` is worked out from the focus, the distance, `yaw` and
// `pitch` by the orbit and is never set on its own; the fly moves it only by
// moving the focus with it, so the orbit's sum stays true. Zero yaw looks down −Z,
// positive yaw turns towards −X and positive pitch looks up; `lens` is what
// every view sees with. The eye and the focus are world positions, so double
// (ADR-0250); what moves them is float motion added to them.
typedef struct {
	voe_math_double3 eye;
	float yaw;
	float pitch;
	voe_scene_camera lens;
	// The point the camera orbits about and looks at, in metres, and how far
	// the eye stands from it. The distance is always above nought.
	voe_math_double3 focus;
	float distance;
	// The metres a second this view flies at before Shift, set by the wheel
	// while it flies and kept for its next fly (0396 point 7).
	float fly_speed;

	// The glide: whether one is under way, the focus and distance it left
	// and is going to, and the seconds gone since it began.
	bool gliding;
	voe_math_double3 glide_from_focus;
	float glide_from_distance;
	voe_math_double3 glide_to_focus;
	float glide_to_distance;
	float glide_seconds;

	voe_render_target target;
	voe_render_texture texture;
	// Where the casters of this view's last bounce stood, feeding the bounce
	// of `target` alone; only 3d writes it (0394).
	voe_3d_bounce_casters casters;
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

// The keys the fly moves by, each the level of whichever key the caller maps
// to it: along the look, across it level, along world ±Y, and three times as fast;
// and the wheel's notches this frame, positive turned away from the person (up).
typedef struct {
	bool forward;
	bool back;
	bool left;
	bool right;
	bool up;
	bool down;
	bool fast;
	float notches;
} voe_editor_fly_keys;

// Every view, the middle-button drag and the right-button fly. `captured` is
// the view the drag started over; `pointer` is where the pointer was last frame,
// in the root surface's millimetres, which is what a drag's travel is measured
// from. `flying` is the view the fly started over, VOE_EDITOR_VIEW_NONE when none.
// Each view and the preview hold remembered casters of about 20 KB (0394), so
// the struct is about 100 KB; main.c keeps it in a local, inside every
// platform's default main-thread stack.
typedef struct {
	voe_editor_view views[VOE_EDITOR_VIEWS];
	uint32_t count;
	uint32_t captured;
	bool middle_was_down;
	voe_math_float2 pointer;
	uint32_t flying;
	bool right_was_down;

	// The preview's target and its picture, and whether this frame drew it.
	voe_render_target preview_target;
	voe_render_texture preview_texture;
	bool preview_shown;
	// Where the casters of the preview's last bounce stood, feeding the
	// bounce of `preview_target` alone; only 3d writes it (0394).
	voe_3d_bounce_casters preview_casters;
} voe_editor_views;

// Sets every view in use to its initial camera — view 0 from the front and
// above, view 1 from the side, both looking at the origin until
// voe_editor_views_focus_camera moves them — and makes each one a target, then
// the preview's. A startup operation, because making a target is.
//
// False when `render` refused a target, which has already said why on stderr.
[[nodiscard]] bool voe_editor_views_create(voe_editor_views *views,
					   voe_render_device *gpu,
					   voe_base_error *error);

// Sets every view in use to orbit the world camera entity's position, the
// origin when the world has no camera or it has no transform, and puts each eye
// back by the orbit; yaw, pitch and distance are untouched (0255). Called once
// at startup and whenever a different project replaces the open one, so a scene
// 100 km out opens where it is; not on a Refresh or a Play, which keep the same
// scene and so the view a person flew to.
void voe_editor_views_focus_camera(voe_editor_views *views,
				   const voe_ecs_world *world);

// Asks for the view's target to be its recorded rectangle times
// `pixels_per_millimetre`, rounded, when that differs from what it is. A view
// with no rectangle yet — the first frame — keeps the size it was made with.
// Called before the frame's draw is opened, so the resize is applied by the very
// voe_render_frame_begin that draws it.
void voe_editor_view_fit(voe_editor_view *view, voe_render_device *gpu,
			 float pixels_per_millimetre);

// The light every view and the preview take as `light`, once a frame: the
// world's first directional light as `3d` gives it (voe_3d_draw_system_light);
// the others come with the frame (voe_3d_draw_system_lights). A
// world (level or open prefab) with no light is shown by the preview light
// (voe_editor_project_preview_light), which casts no shadows, and a light added
// replaces it at once; Play and the game draw such a world black (0287).
voe_render_light voe_editor_view_light(const voe_ecs_world *world);

// The colour every view's selection outline is drawn in: the lighter of the
// palette's `inverse` and `inverse_ink`, by relative luminance on the linear
// numbers the palette already holds (ui/theme.h). A scene view's background is
// the engine's near-black clear colour whatever the theme is, so the dark half
// of a light theme's inverse pair would be an outline nobody can see. Both
// roles carry the theme's one hue and neither is a colour of its own, so taking
// the lighter keeps ADR-0194's rule and keeps the outline visible in every
// theme (ADR-0203).
voe_math_float3 voe_editor_view_outline_colour(const voe_ui_theme *palette);

// The colour a gizmo handle is drawn in (ADR-0207): the outline's colour as it
// stands for a `marked` handle — hovered or held — and that colour multiplied
// by VOE_EDITOR_VIEW_GIZMO_REST for a handle at rest. Multiplying a linear
// colour is a lightness step on the theme's one hue rather than a new role, and
// the outline's colour is the one already known to show on a view's near-black
// ground in every theme, so both states stay visible and differ by lightness.
voe_math_float3 voe_editor_view_gizmo_colour(const voe_ui_theme *palette,
					     bool marked);

// The colour a light blocker's box is lined in while it is not selected (0365
// point 5): the outline's colour times VOE_EDITOR_VIEW_FAINT. A lightness step
// on the outline's colour, not a new role, for the gizmo's rest colour's
// reason: the theme keeps one hue, and the outline's colour is the one known
// to show on a view's near-black ground. Fainter than a handle at rest, so a
// level full of rooms stays readable and a box never reads as a handle.
voe_math_float3 voe_editor_view_faint_colour(const voe_ui_theme *palette);

// The camera and `light` the view's pass is opened with, the aspect ratio from
// the view's own size. `light` is the caller's to find — what
// voe_editor_view_light gives — because a
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

// The right-button fly, once per frame. The right button's down edge over a
// view (as voe_editor_views_under reads it), while no middle drag holds a view,
// makes that view the flying one until `right` goes up. While it flies, `turn`
// — the pointer's motion in the platform's units, +x right, +y down — turns it
// about its eye, and `keys` move it for `seconds` at the view's own speed. Each
// notch up multiplies that speed by 1.25 and each down divides it, clamped to
// 0.5 .. 1000 m/s and kept for the view's next fly. `pointer` is in the root
// surface's millimetres. Returns whether a view is flying after this call.
bool voe_editor_views_fly(voe_editor_views *views, voe_math_float2 pointer,
			  bool right, voe_math_float2 turn,
			  voe_editor_fly_keys keys, float seconds);

// Starts the view gliding from where it is now to orbit `focus` at `distance`,
// the distance clamped to the orbit's closest; yaw and pitch stay (0371).
void voe_editor_view_glide_to(voe_editor_view *view, voe_math_double3 focus,
			      float distance);

// Once a frame: every gliding view moves by smoothstep of the time gone over
// VOE_EDITOR_VIEW_GLIDE_SECONDS, the focus lerped in double and the eye put back
// by the orbit; at the end it lands exactly on the target and stops gliding.
void voe_editor_views_glide(voe_editor_views *views, float seconds);

// Which view the pointer is over and where in that view's picture, in the
// picture's own pixels — x right, y down from its top-left corner, which is
// what voe_3d_pick_ray takes. False when the pointer is over no view, and the
// two out-parameters are untouched then. `pointer` is in the root surface's
// millimetres, the same place the drag is handed.
[[nodiscard]] bool voe_editor_views_under(const voe_editor_views *views,
					  voe_math_float2 pointer,
					  uint32_t *view,
					  voe_math_float2 *point);

// Forgets every picture's node. Called before the dock walk, because last
// frame's nodes named last frame's tree.
void voe_editor_views_images_clear(voe_editor_views *views);

// Reads where each picture drawn this frame came to sit. Called after
// voe_ui_frame_end and before the frame's arena is rewound, which is the one
// window a node's rectangle may be read in (ui/layout.h).
void voe_editor_views_rects_read(voe_editor_views *views,
				 const voe_ui_context *ui);
