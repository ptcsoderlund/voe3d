// The two panels' content: the element exhibit — forty coloured rectangles and
// two lines of writing — and a small badge beside it. Each is submitted every
// frame and drawn by ONE draw command, so that a person can look at what the
// element path does and read the number that says it did it in one draw.
//
// THE WRITING IS THE POINT OF THE SECOND HALF. A letter here is not text drawn
// by a text system: it is a voe_render_element of kind GLYPH, the same eighty
// bytes as the rectangles beside it, in the same buffer and the same draw. The
// number printed beside the exhibit still says one, and that it still says one
// with letters in the picture is the whole claim.
//
// IT IS A CALL SITE AND NOT A SECOND ENGINE. Everything here is
// voe_render_frame_submit_element in the order a program would call it; the
// only thing this file decides is where the rectangles are and what colour each
// one is.
//
// NOTHING HERE DRAWS, AND THAT IS THE CHANGE CARD 032 MADE. Both calls submit
// records and hand the range back through voe_render_frame_elements_submitted;
// the draw is the draw system's, one per panel, in the layer and sort order
// everything else drawn goes through. main.c writes each range onto its panel
// component between voe_render_frame_begin and the walk, which is the phase the
// loop reserves for building what changes this frame.
//
// BOTH SURFACES ARE THINGS IN THE WORLD, MEASURED IN REAL MILLIMETRES. A GUI
// unit is 1 mm (ADR-0089) and a panel is an object: the exhibit is 240 by 135 mm,
// which is 0.24 by 0.135 metres, and the entity's own transform is what scales it
// up to something readable from the orbit. A resize does nothing to either of
// them — the window changes the camera's aspect and nothing else, and neither
// panel answers to ui_scale. The surface that does is src/surface.h, which is a
// different kind of surface and says so.
//
// THE EXHIBIT STANDS IN THE WORLD AND THE BADGE IS IN THE OVERLAY, which is the
// pair worth having. Something in the scene passes in front of the exhibit as
// the camera goes round, because a panel is occluded by what is in front of it;
// nothing ever covers the badge, because the overlay is drawn after the world's
// depth has been cleared. Both keep real positions in metres and neither is
// screen space.
//
// EVERY COLOUR IN HERE IS LINEAR, because every colour that crosses render's
// boundary is (see render/include/render/device.h). They are written as linear
// numbers rather than as the sRGB ones a colour picker would give, so the hues
// are wider apart on screen than the numbers look.
#pragma once

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

// And what the badge submits, counted the same way and for the same reason.
#define VOE_DEV_BADGE_ELEMENTS 5

// The panel the exhibit is laid out on, in millimetres, and 16:9 because the
// layout inside it was authored that way. It is 0.24 by 0.135 metres in the
// world; what makes it something a person can read from seven metres away is the
// scale on its entity's transform, in main.c.
#define VOE_DEV_ELEMENTS_PANEL_WIDE 240.0f
#define VOE_DEV_ELEMENTS_PANEL_HIGH 135.0f

// The badge's panel, in millimetres. Small, because its whole job is to be a
// second range of the one buffer with a second matrix — two panels is what says
// a range is a range rather than "the whole frame" spelled differently.
#define VOE_DEV_BADGE_WIDE 80.0f
#define VOE_DEV_BADGE_HIGH 50.0f

// Submits the exhibit's rectangles and letters into the open frame. It does not
// draw: the caller reads voe_render_frame_elements_submitted either side of this
// and writes the difference onto the exhibit's panel component.
//
// `font` is what the letters are measured from — their boxes, their sheet
// rectangles and their advances — and its atlas is the sheet every glyph record
// names. It is the same font the world's text is drawn with, because there is
// one and because a second sheet would prove nothing this one does not.
//
// False when a submit was refused, which for this program means the element
// capacity is smaller than everything the frame submits — this file's mistake
// and worth stopping over. The refusal has already said which numbers it was on
// stderr.
[[nodiscard]] bool voe_dev_elements_submit(voe_render_device *gpu,
					   const voe_text_font *font);

// Submits the badge's few rectangles, on the same terms. No font and no letters:
// what the badge is for is being a second range, and a second exhibit would say
// nothing the first one has not.
[[nodiscard]] bool voe_dev_elements_badge_submit(voe_render_device *gpu);
