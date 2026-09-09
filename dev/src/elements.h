// The element exhibit: forty coloured rectangles and two lines of writing,
// submitted every frame and drawn by ONE draw command, so that a person can
// look at what the element path does and read the number that says it did it in
// one draw.
//
// THE WRITING IS THE POINT OF THE SECOND HALF. A letter here is not text drawn
// by a text system: it is a voe_render_element of kind GLYPH, the same eighty
// bytes as the rectangles beside it, in the same buffer and the same draw. The
// number printed beside the exhibit still says one, and that it still says one
// with letters in the picture is the whole claim.
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
#include <text/font.h>

#include <stdint.h>

// How many elements the exhibit submits, so that main.c can ask the device for
// exactly that many and no "to be safe" headroom. It is checked against what is
// actually submitted, in voe_dev_elements_submit.
//
// IT IS TWO NUMBERS NOW BECAUSE THE SECOND ONE DEPENDS ON WHAT THE WRITING
// SAYS. A character that draws costs one element and a space costs none, so the
// glyph number below is the drawn characters of the two lines in elements.c,
// counted — seventeen and twenty-three. Changing a string without changing it is
// a refusal on the last submit, which would be a letter quietly missing from the
// picture; the assert at the bottom of voe_dev_elements_submit is what turns
// that into a stop instead. That arrangement is the same one the rectangles have
// always had and it is deliberately not relaxed to "at most".
#define VOE_DEV_ELEMENTS_RECTANGLES 40
#define VOE_DEV_ELEMENTS_GLYPHS 40
#define VOE_DEV_ELEMENTS \
	(VOE_DEV_ELEMENTS_RECTANGLES + VOE_DEV_ELEMENTS_GLYPHS)

// The panel the exhibit is laid out on, in millimetres. 16:9, which is the
// window's default aspect.
#define VOE_DEV_ELEMENTS_PANEL_WIDE 240.0f
#define VOE_DEV_ELEMENTS_PANEL_HIGH 135.0f

// Submits the exhibit's rectangles and letters into the open frame and draws
// them. Called between the draw system's walk and voe_render_frame_end, which is
// the provisional place the header explains.
//
// `font` is what the letters are measured from — their boxes, their sheet
// rectangles and their advances — and its atlas is the sheet every glyph record
// names. It is the same font the world's text is drawn with, because there is
// one and because a second sheet would prove nothing this one does not.
//
// False when a submit or the draw was refused, which for this program means the
// element capacity is smaller than VOE_DEV_ELEMENTS — this file's mistake and
// worth stopping over. The refusal has already said which numbers it was on
// stderr.
[[nodiscard]] bool voe_dev_elements_submit(voe_render_device *gpu,
					   const voe_text_font *font);

// The transform the exhibit is drawn with: the panel above, filling the whole
// target. Separate from the submit so that main.c can say what it drew with.
voe_math_float4x4 voe_dev_elements_transform(void);
