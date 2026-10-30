// The label: a string measured by the font into a box, and one glyph record per
// character that draws, inked by its role or by the control it sits in. See
// include/ui/widgets.h for the promises; widgets.c's emit calls
// voe_ui_push_label for every label node in paint order, and the clip, the
// theme and the record push are widgets.c's, through context.h.
//
// THE ONE PLACE A SIGN APPEARS ON THE ELEMENT PATH IS HERE, AND IT IS NOT A
// FLIP: a font measures up from a baseline and a surface measures down from a
// corner, so the top of a glyph's box is `baseline - high.y` and its bottom is
// `baseline - low.y`. That is the same expression dev/src/elements.c uses and
// text/include/text/font.h states the convention it comes from. Every other
// record is a copy of its node's rectangle — see widgets.c's header.
#include "context.h"

#include <base/assert.h>
#include <math/float2.h>
#include <math/float4.h>
#include <text/font.h>
#include <text/utf8.h>

// What a role multiplies the theme's text_size by: a heading is bigger, every
// other role is the theme's own size (ADR-0503).
static float role_scale(voe_ui_text_role role)
{
	return role == VOE_UI_TEXT_ROLE_HEADING ? VOE_UI_HEADING_SCALE : 1.0f;
}

voe_ui_node voe_ui_label_role(voe_ui_context *ui, const char *text,
			      voe_ui_text_role role)
{
	voe_text_measure measured;
	const voe_ui_theme *theme = voe_ui_theme_current(ui);
	float em;
	voe_ui_node node;

	VOE_BASE_ASSERT(ui != NULL, "adding a label to no context");
	VOE_BASE_ASSERT(text != NULL, "a label with no string");
	VOE_BASE_ASSERT(ui->font != NULL,
			"a label needs a font: measuring a string is the one "
			"thing it cannot do for itself — see voe_ui_font_set");
	VOE_BASE_ASSERT(theme != NULL,
			"a label needs a theme: its size and its ink both "
			"come from one — see voe_ui_theme_set/voe_ui_theme_push");

	// ADR-0090: the size is the font's measurement of this string, and
	// there is no table of advances and no assumed line height anywhere
	// above this call. The theme's own text_size is applied HERE, before
	// the number becomes a natural size, so that everything layout does
	// afterwards is ordinary layout in ordinary millimetres.
	em = theme->text_size * role_scale(role);
	measured = voe_text_font_measure(ui->font, text);

	node = voe_ui_box(ui,
			  (voe_math_float2){ measured.size.x * em,
					     measured.size.y * em },
			  (voe_ui_sizing){ 0 });
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_LABEL;
		ui->widgets[node].text = text;
		ui->widgets[node].baseline = measured.baseline * em;
		ui->widgets[node].theme = theme;
		ui->widgets[node].text_role = role;
	}

	return node;
}

voe_ui_node voe_ui_label(voe_ui_context *ui, const char *text)
{
	return voe_ui_label_role(ui, text, VOE_UI_TEXT_ROLE_NORMAL);
}

// One letter of a label.
//
// DEVIATION: task 2 of spec 001 says a record's clip is its rectangle intersected
// with the node's visible rectangle. A glyph's box reaches past its label's own
// rectangle — the field's margin, the half slack added above the baseline — so
// that reading would trim letters in a tree that clips nothing, and a zeroed
// container must behave as today. The narrowest reading that keeps both: a glyph
// is narrowed by what the label's clipping ancestors leave, which is the visible
// rectangle's own rule, and not by the label's rectangle.
static void push_glyph(voe_ui_context *ui, uint32_t node,
		       voe_math_float4 bounds, voe_math_float4 sheet,
		       voe_math_float4 ink)
{
	voe_math_float4 clip;

	if (!voe_ui_clip_of(ui, node, bounds, &clip))
		return;

	voe_ui_push_element(ui, (voe_render_element){
				 .bounds = bounds,
				 .clip = clip,
				 .colour = ink,
				 .kind = VOE_RENDER_ELEMENT_GLYPH,
				 .sheet_texture =
					 voe_text_font_atlas(ui->font).index,
				 // Straight across: voe_text_glyph already
				 // hands the sheet rectangle over in the
				 // record's own shape, its corner paired to
				 // the box's top-left.
				 .sheet = sheet,
			 });
}

// The nearest button or number box above `node`, or VOE_UI_NODE_NONE: the
// control a label inside one is drawn by. The walk stops at the first of the
// two, a label belonging to the control it was composed into and not to
// whatever that control sits in.
static uint32_t control_above(const voe_ui_context *ui, uint32_t node)
{
	for (uint32_t at = ui->nodes[node].parent; at != VOE_UI_NODE_NONE;
	     at = ui->nodes[at].parent) {
		enum voe_ui_widget kind = ui->widgets[at].kind;

		if (kind == VOE_UI_WIDGET_BUTTON ||
		    kind == VOE_UI_WIDGET_NUMBER)
			return at;
	}

	return VOE_UI_NODE_NONE;
}

// The ink of a label's letters: the one its role asks for, and `inverse_ink`
// over that wherever the label is drawn inverted (ADR-0196) — inside a
// control that is held, dragged or selected, or as the selected text of a
// field or an open number box, which field.c has already pushed `inverse`
// behind.
//
// THE CONTROL DECIDES THE INK AND NOT THE CALLER, WHATEVER ROLE WAS ASKED
// FOR, because a caller composing a button cannot know on that frame whether
// it is about to be held. The role still decides everything else, so a
// secondary label goes back to reading as one the moment the press ends.
static voe_math_float4 label_ink(const voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	uint32_t control = control_above(ui, node);

	if ((control != VOE_UI_NODE_NONE &&
	     voe_ui_control_inverted(ui, control)) ||
	    voe_ui_label_selected(ui, node))
		return w->theme->inverse_ink;

	return w->text_role == VOE_UI_TEXT_ROLE_SECONDARY
		       ? w->theme->text_secondary
		       : w->theme->text_primary;
}

// One record per character that draws, in reading order. A space advances the
// pen and costs nothing, which on this path is a whole element of the frame's
// capacity saved for every one of them.
void voe_ui_push_label(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_ui_rect rect = ui->nodes[node].rect;
	// The SAME theme this label read at the call that made it, so what is
	// drawn here can never disagree with what was measured then — see
	// context.h on why the theme is copied onto the node rather than
	// looked up again. Scaled by the role, as it was at the measure.
	float em = w->theme->text_size * role_scale(w->text_role);
	voe_math_float4 ink = label_ink(ui, node);
	float line = voe_text_font_line_height(ui->font) * em;
	float pen = rect.min.x;
	// The slack under the last line: the descender and the line gap, which
	// is everything of the line box the letters of an ordinary word do not
	// reach into. It is `line` and not `rect.size.y` because a label of
	// three lines has three line heights in its box and only the last one's
	// slack is under the text.
	float slack = line - w->baseline;
	// The font measures up from a baseline and this surface measures down
	// from a corner, so the baseline is BELOW the top edge by the ascender
	// the measurement handed back. Adding it is the whole of the conversion
	// and it is not a flip — see this file's header.
	//
	// AND HALF THE SLACK IS ADDED WITH IT, WHICH IS WHY A WORD SITS IN THE
	// MIDDLE OF A BUTTON RATHER THAN HIGH IN IT. A label's box is a whole
	// line — ascender, descender and line gap — and `click: 0` inks only the
	// part above the baseline, so a container centring that box centres the
	// space and not the writing: every button came out with more room under
	// its word than over it. Halving the slack puts as much of it above the
	// letters as below.
	//
	// IT MOVES THE TEXT AND NOT THE BOX. The rectangle layout measured is
	// untouched, so nothing about sizes, hit rectangles or where the next
	// widget goes changes — this is the last step before the letters become
	// records, and it is the only place in this folder that adjusts one.
	//
	// IT IS METRIC CENTRING AND NOT OPTICAL CENTRING. A capital is shorter
	// than the ascender, so a word of capitals still sits a little high; the
	// ink's own extent is a question for `text` and this folder cannot ask
	// it. The descender is the large half of the error and this is the half
	// that can be fixed from here.
	//
	// AND IT IS A DEFAULT AND NOT A POLICY. When styling arrives it decides
	// where a label sits in its box and this line goes with it.
	float baseline = rect.min.y + w->baseline + slack * 0.5f;

	for (const char *at = w->text; *at != '\0';) {
		uint32_t codepoint;
		voe_text_glyph g;

		at += voe_text_utf8_next(at, &codepoint);

		if (codepoint == '\n') {
			pen = rect.min.x;
			baseline += line;
			continue;
		}

		g = voe_text_font_glyph(ui->font, codepoint);
		if (g.drawn) {
			// The box whole and not trimmed: the field carries on
			// past the outline and a rectangle cut back to what the
			// letter looks like clips its own stems.
			voe_math_float4 bounds = {
				pen + g.low.x * em, baseline - g.high.y * em,
				(g.high.x - g.low.x) * em,
				(g.high.y - g.low.y) * em
			};

			push_glyph(ui, node, bounds, g.sheet, ink);
		}

		pen += g.advance * em;
	}
}
