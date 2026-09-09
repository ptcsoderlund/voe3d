// The element exhibit: forty coloured rectangles submitted every frame and
// drawn by one draw command, so that a person can look at what card 030's
// element path does and read the number that says it did it in one draw.
//
// IT IS A CALL SITE AND NOT A SECOND ENGINE. Everything here is
// voe_render_frame_submit_element and voe_render_frame_draw_elements in the
// order a program would call them; the only thing this file decides is where
// the rectangles are and what colour each one is.
//
// WHERE IT IS DRAWN IN THE FRAME IS PROVISIONAL. It is submitted and drawn after
// the draw system has walked the world and before the frame ends, so it lands
// over everything as a full-screen overlay. That is a place from which it can be
// seen and nothing more: card 032 makes an element surface an entity the draw
// system draws in layer order, and this call moves there when it does.
//
// THE SURFACE IS A PANEL OF A FIXED SIZE IN MILLIMETRES AND NOT THE WINDOW IN
// PIXELS. A GUI unit is 1 mm (ADR-0089), so the exhibit is laid out in
// millimetres on a panel that is stretched to fill the window. Its aspect is the
// window's default aspect, so at that size nothing is stretched; drag the window
// narrow and the rectangles stretch with it, which is what a panel filling a
// window does and not a bug.
//
// EVERY COLOUR IN HERE IS LINEAR, because every colour that crosses render's
// boundary is (see render/include/render/device.h). They are written as linear
// numbers rather than as the sRGB ones a colour picker would give, so the hues
// are wider apart on screen than the numbers look.
#pragma once

#include <math/float2.h>
#include <render/device.h>

#include <stdint.h>

// How many elements the exhibit submits, so that main.c can ask the device for
// exactly that many and no "to be safe" headroom. It is checked against what is
// actually submitted, in voe_dev_elements_submit.
#define VOE_DEV_ELEMENTS 40

// The panel the exhibit is laid out on, in millimetres. 16:9, which is the
// window's default aspect.
#define VOE_DEV_ELEMENTS_PANEL_WIDE 240.0f
#define VOE_DEV_ELEMENTS_PANEL_HIGH 135.0f

// Submits the exhibit's rectangles into the open frame and draws them. Called
// between the draw system's walk and voe_render_frame_end, which is the
// provisional place the header explains.
//
// False when a submit or the draw was refused, which for this program means the
// element capacity is smaller than VOE_DEV_ELEMENTS — this file's mistake and
// worth stopping over. The refusal has already said which numbers it was on
// stderr.
[[nodiscard]] bool voe_dev_elements_submit(voe_render_device *gpu);

// The transform the exhibit is drawn with: the panel above, filling the whole
// target. Separate from the submit so that main.c can say what it drew with.
voe_math_float4x4 voe_dev_elements_transform(void);
