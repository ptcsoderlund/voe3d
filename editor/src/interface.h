// The editor's whole interface, on the screen-filling surface: the dock's rows,
// columns and panels turned into element records and drawn onto the window in
// one command per root. The same shape `dev/src/interface.c` has, written out
// here rather than shared — `dev` is a leaf and so is this, and neither includes
// the other's files.
//
// IT IS HANDED THE POINTER AND NEVER ASKS FOR ONE (ADR-0141 point 4). Each root
// carries its own pointer, already in this surface's millimetres; the division
// that turns the window's pixels into them is in `main.c` and in nothing else,
// because the day a panel is a quad standing in the world that division is a ray
// against the quad and only the call site can know which it is.
//
// A ROOT IS A FRAME, WHICH IS WHY THE LOOP IS SHAPED THIS WAY. `ui` lays out one
// root container per frame (see ui/layout.h) and a root is its own surface with
// its own size, so it has its own element transform and its own range of the
// buffer. One root today, one frame, one draw command.
#pragma once

#include "dock.h"
#include "session.h"

#include <base/arena.h>
#include <math/float2.h>
#include <platform/window.h>
#include <render/device.h>
#include <text/font.h>
#include <ui/layout.h>

#include <stdint.h>

// THE ONLY CALIBRATION THIS PROGRAM HAS. Everything on the surface is drawn
// VOE_EDITOR_UI_SCALE times bigger, with that much less room to put it in.
// Nothing reads a display and there is no per-device logic behind it.
#define VOE_EDITOR_UI_SCALE 1.0f

// How tall the surface is in its own millimetres at a scale of one. The window's
// height divided by this is pixels per millimetre, which is ADR-0104's whole
// formula and the reason a window twice as tall shows the same thing twice as
// big rather than twice as much of it.
#define VOE_EDITOR_SURFACE_HIGH 135.0f

// What one frame of this interface may hold. All three are checked by `ui`, and
// a frame that wants more is refused with a line saying which number it was — so
// these are numbers to be honest about rather than careful with. A label is one
// element record per character that draws, which is what makes the second one
// much the larger of the three.
//
// THE SCROLL AREAS ARE ONE PER DOCK LEAF THAT IS NOT A SCENE VIEW, because that
// is what dock.c puts one round. The default tree has two — the Scene list and
// the Inspector — and a third panel is a third here.
//
// AND EACH OF THEM COSTS UP TO FOUR ELEMENT RECORDS OF ITS OWN, on top of
// everything the panel draws: a track and a thumb on each of the two axes, when
// both bars show. So the records below are the 512 the panels had before this
// feature plus 2 areas × 4 = 8 for their bars.
//
// THE TOP BAR (topbar.h) AND THE COLUMN THIS FILE OPENS OVER IT ADD ELEVEN
// NODES AT MOST: the column itself, one; topbar.h's own panel and the row
// inside it, two more; New, Open and Save as a button and a composed label
// each, six; and one label each for the project's name and the notice. That
// is 128 + 11 = 139 nodes.
//
// AND THEY ADD FOUR BACKGROUNDS, ELEVEN CHARACTERS AND ROOM FOR TWO HUNDRED
// MORE. The column and the row draw none of their own; the panel's
// background and each button's are four, the eleven characters "New", "Open"
// and "Save" draw between them are eleven, and two hundred more is generous
// for whatever a project's name and a notice's line come to — neither this
// file nor topbar.h puts a limit on how long either string is. That is
// 520 + 4 + 11 + 200 = 735 elements.
#define VOE_EDITOR_INTERFACE_NODES 139
#define VOE_EDITOR_INTERFACE_ELEMENTS 735
#define VOE_EDITOR_INTERFACE_SCROLLS 2

// Makes the context the interface is built in, once. It lives in `arena` and is
// freed with it; the font must outlive it.
voe_ui_context *voe_editor_interface_new(voe_base_arena *arena,
					 const voe_text_font *font);

// How big the surface is on a window of this size, and what one of its
// millimetres is worth in pixels. Both answers come out of one call because they
// come out of one division, and the caller needs each for a different reason:
// the size is what a root is laid out in, and the scale is what the pointer's
// pixels are divided by.
//
// A window with no area is the caller's bug and asserts — the reciprocal of
// nothing is what would otherwise reach a matrix.
void voe_editor_interface_surface(voe_platform_size target,
				  voe_math_float2 *millimetres,
				  float *pixels_per_millimetre);

// Builds, submits and draws every root's interface into the open frame, over the
// whole render target. Called when the draw is open and before it is closed; it
// is not in any world, so it has nothing to sort against and issues its own
// draws.
//
// False when a frame was refused — more nodes, more records or more scroll areas
// than the three numbers above — or when a submit or a draw was refused. All of those are this
// program's numbers being wrong, and whichever it was has already said so on
// stderr.
//
// `scene` is handed through to the panels and is also where THIS FRAME'S CLICKS
// LAND. The read has to happen in here and cannot be the caller's: a widget
// answers only between voe_ui_frame_end and the rewind of the arena its nodes
// were pushed out of (ui/widgets.h), and both of those are this function's.
// `views` is handed through the same way, and where each scene view's picture
// came to sit is read back into it in the same window, for the same reason.
//
// EACH ROOT GETS THE TOP BAR ABOVE ITS DOCK TREE, DRAWN FROM `session`, FOR
// THE SAME REASON AGAIN. The bar takes VOE_EDITOR_TOPBAR_HIGH off the top of
// the root's own height and hands the dock tree the rest, in a column this
// function opens as the frame's actual root; the tree itself is dock.c's
// unchanged, only nested one level deeper than it used to be, which is why
// voe_editor_dock_walk is called with VOE_EDITOR_DOCK_COLUMN (dock.h) — a
// child's declared size is read against its PARENT's flow and not its own
// (ui/layout.h), and the parent is now this column rather than the frame
// itself. A button that fired is carried out on `session` before this root's
// records are submitted, through voe_editor_session_do, which is the reason
// `session` and not just its notice and its project's name are handed in.
[[nodiscard]] bool voe_editor_interface_draw(voe_render_device *gpu,
					     voe_ui_context *ui,
					     voe_base_arena *arena,
					     const voe_editor_dock_root *roots,
					     uint32_t count,
					     voe_editor_scene *scene,
					     voe_editor_views *views,
					     voe_editor_session *session);
