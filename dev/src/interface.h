// The first real interface: a semitransparent panel with a heading, two buttons
// and three number boxes on it, laid out by `ui`, hit tested against the mouse,
// and drawn in ONE draw command however many letters are on it.
//
// THE DRAW COUNT IS THE CLAIM AND IT IS WHY THIS IS A SEPARATE SURFACE. A panel,
// two buttons, three number boxes and six lines of writing is around fifty
// element records and one command — the panel and each button draw a hairline
// border now (ADR-0171), two records where one used to do, three more than
// before — and the readout on screen says how many commands the whole frame
// took, and adding this interface to the program moved that number by one. Not
// by one per widget and not by one per letter, which is what an interface built
// out of meshes would have cost — and dragging a number box changes how many
// records there are without ever changing how many commands.
//
// IT IS ON THE SCREEN-FILLING SURFACE AND NOT ON A PANEL IN THE WORLD, because
// this is the kind of interface a person points at: it is mapped onto the window
// by voe_render_element_transform, it answers to VOE_DEV_UI_SCALE the way
// everything on that surface does, and the mouse's pixels divide by the same one
// number to become the millimetres `ui` is given. A panel standing in the world
// would need a ray against a quad to be pointed at, which is a later card and
// not this one.
//
// THE ROOT IS THE WHOLE SURFACE AND THE PANEL IS PUSHED DOWN THE PAGE WITH A
// SPACER. That is what an interface with no anchoring looks like and it is
// deliberate rather than awkward: card 041 adds anchored children, and when it
// lands the spacer goes and the panel says where it wants to be. Until then the
// readout owns the top-left corner (card 040 found that out the hard way) and
// this sits below it.
//
// NOTHING HERE MEASURES A LETTER OR PLACES ONE. `ui` does both, out of the same
// font the world's text uses, and this file names a string and a colour and
// nothing else — which is the whole difference between this and src/elements.c,
// the exhibit that writes element records by hand.
#pragma once

#include <base/arena.h>
#include <platform/input.h>
#include <platform/window.h>
#include <render/device.h>
#include <text/font.h>
#include <ui/layout.h>

#include <stdint.h>

// What the interface may need in one frame. Both are checked by `ui` and a
// frame that wants more is refused with a line saying which number it was, so
// these are numbers to be honest about rather than careful with. The border a
// panel and a button now draw (ADR-0171) is a second element record on the
// same node, never a new node, so it moves VOE_DEV_INTERFACE_ELEMENTS by three
// — one for the panel, one for each of the two buttons — and leaves
// VOE_DEV_INTERFACE_NODES untouched.
#define VOE_DEV_INTERFACE_NODES 48
#define VOE_DEV_INTERFACE_ELEMENTS 131

// Makes the context the interface is built in, once. It lives in `arena` and is
// freed with it; the font must outlive it.
voe_ui_context *voe_dev_interface_new(voe_base_arena *arena,
				      const voe_text_font *font);

// Builds this frame's interface, submits its records and draws them, over the
// whole of `target`. Called after the draw system has walked and before the
// frame ends, exactly as src/surface.h's is and for the same reason: it is not
// in the world, so it has nothing to sort against and issues its own draw.
//
// `pointer` and `down` are the mouse as `platform` reports it, in the window's
// pixels; the one division that turns those into the surface's millimetres is in
// here, because it is the same division that decides how big the surface is.
//
// `fine` is the fine-drag modifier, worked out by the caller for the same reason
// `down` is: `ui` is handed values and never asks a window anything, so which key
// means "slower" is this program's choice and not that folder's.
//
// `elements` comes back with how many records the interface emitted, for the
// console line that says what one draw command was worth.
//
// False when the frame was refused — more nodes or more records than the two
// numbers above — or when a submit or the draw was refused. All of those are
// this program's numbers being wrong and are worth stopping over; whichever it
// was has already said so on stderr.
[[nodiscard]] bool voe_dev_interface_draw(voe_render_device *gpu,
					  voe_ui_context *ui,
					  voe_base_arena *arena,
					  voe_platform_size target,
					  voe_platform_pointer pointer,
					  bool down, bool fine,
					  uint32_t *elements);
