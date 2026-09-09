// The screen-filling surface: a handful of rectangles mapped straight onto the
// window, and the only thing in this program that answers to the window's shape.
//
// IT IS A DIFFERENT KIND OF SURFACE FROM THE TWO PANELS AND THE DIFFERENCE IS
// THE WHOLE REASON IT IS HERE. A panel (src/elements.h) is an object: it has a
// position in metres, the draw system draws it in layer and sort order, and a
// cube can stand in front of it. This has no position at all. It is mapped onto
// the whole render target by voe_render_element_transform, it is drawn by a
// direct call after the draw system has walked, and nothing can be in front of
// it because it is not in the world. That is not "drawing the GUI last" — the
// thing ADR-0093 rejects is world content drawn last, and this is not world
// content.
//
// AND A MILLIMETRE HERE IS NOT A MILLIMETRE ON A PANEL. On a panel, an authored
// millimetre is a real millimetre: 240 mm is 0.24 m and a ruler would agree. On
// this surface it is a PROPORTION OF THE SURFACE'S AUTHORED HEIGHT — nothing
// physical, and nothing to do with any display. It becomes a size a person can
// measure only through the window's height and whatever ui_scale is set to
// below. One word, two meanings, and this paragraph is the answer to which is
// which.
//
// WHAT A RESIZE DOES: THE SAME RECTANGLES AT THE SAME SHAPE, WITH MORE OR LESS
// ROOM AROUND THEM. Both axes are divided by one number
// (voe_render_element_surface_size), so nothing is ever deformed — a square is a
// square in any window. What a window's shape changes is how many millimetres
// wide the surface is: drag the window narrow and there are fewer millimetres
// across to put things in. Nothing shrinks to fit.
//
// SO THE FAR CONTENT IS CUT OFF IN A NARROW WINDOW, AND THAT IS CORRECT. The
// marks along the top edge are at fixed millimetre positions from the top-left
// and deliberately reach further right than a narrow window has room for. Wrapping and scrolling are what answer that later (card 035); until then,
// content falling off the right edge is the decision working rather than the
// exhibit being broken.
#pragma once

#include <platform/window.h>
#include <render/device.h>

#include <stdint.h>

// THE ONLY CALIBRATION THIS ENGINE HAS, AND A PERSON CHANGES IT HERE. Everything
// on the screen-filling surface is drawn `VOE_DEV_UI_SCALE` times bigger, with
// that much less room to put it in: at 2.0 the surface holds half the
// millimetres it does at 1.0, so everything on it is twice the size and half of
// the marks along the top run off the edge. Nothing reads a display, on any
// platform, and there is no per-device logic anywhere behind this number.
//
// IT MOVES THIS SURFACE AND NOTHING ELSE. A panel standing in the world is an
// object in metres and does not answer to it; neither does the world, the text
// in it or the readout, which are all meshes. The knob is for the interface that
// is mapped onto the window, because that is the only thing whose size is a
// question about the person looking at it.
#define VOE_DEV_UI_SCALE 1.0f

// How tall the surface is in its own millimetres at a scale of one. The window's
// height is divided by this to get pixels per millimetre — which is ADR-0104's
// whole formula, and the reason a window twice as tall shows the same thing
// twice as big rather than twice as much of it.
#define VOE_DEV_SURFACE_HIGH 135.0f

// How many elements it submits, so that main.c can ask the device for exactly
// that many. Checked against what is actually submitted.
#define VOE_DEV_SURFACE_TICKS 6
#define VOE_DEV_SURFACE_ELEMENTS (VOE_DEV_SURFACE_TICKS + 3)

// Submits the surface's rectangles into the open frame and draws them, over the
// whole of `target`. Called after the draw system has walked and before the
// frame ends — it is not a panel, so it is not in the walk.
//
// IT DRAWS AND THE PANELS DO NOT, WHICH IS THE ONE ASYMMETRY IN THIS PROGRAM. A
// panel hands its range to a component and the draw system issues the draw in
// sort order; this has nothing to sort against, so it issues its own, over its
// own range of the same buffer.
//
// False when a submit or the draw was refused, which for this program means the
// element capacity is smaller than everything the frame submits.
[[nodiscard]] bool voe_dev_surface_draw(voe_render_device *gpu,
					voe_platform_size target);
