// The swatch and the colour picker. See include/ui/colour.h for the promises;
// this file is how the picker is built, how a press or a hex commit becomes a
// colour at frame_end, and what the cells and markers emit.
//
// THE PICKER IS A PANEL WITH THREE KEYED CHILDREN: the square and the strip,
// leaves that take the pointer so a press there arms `held` as a button's does,
// and the hex field, which is widgets.c's field unchanged. So a drag begun in
// the square is simply the square still held, wherever the pointer has gone,
// and nothing here tracks a gesture of its own.
//
// ALL OF A FRAME'S ANSWER IS WORKED OUT IN voe_ui_colour_frame_end, after
// field_edit has settled the hex field's commit and after resolve has settled
// what is held: a hex commit first, then the square or strip held, so a press
// that also commits a half-typed field ends on where the press was. What it
// comes to is written onto the frame's picker record, read back by the action,
// and into the memory the next frame's call reads. The cells and markers draw
// what the picker was built showing, a frame behind a drag, as every widget's
// answer is.
//
// HSV IS OF THE sRGB-ENCODED COLOUR, hue as a fraction of a turn. sRGB <->
// linear is oklab.c's transfer function, the one this folder already has.
//
// CONSTRAINT: the memory is VOE_UI_COLOUR_PICKERS entries, found by a linear
// walk — a frame shows one picker today. A capacity in voe_ui_capacities lifts
// the ceiling; a hash would be wanted only past dozens.
#include "context.h"
#include "oklab.h"

#include <ui/colour.h>

#include <base/assert.h>
#include <base/report.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The picker's geometry, in millimetres: its padding and gap, the square's
// side, the strip's height, and the cells across each.
#define PICKER_PAD 2.0f
#define PICKER_GAP 2.0f
#define SQUARE_SIDE 32.0f
#define STRIP_HIGH 4.0f
#define SQUARE_CELLS 20
#define STRIP_CELLS 36

// The square's marker, an outer ring in text_primary around the current
// colour, and the strip's, a bar reaching past the strip on both edges.
#define MARKER_OUTER 1.6f
#define MARKER_INNER 1.0f
#define BAR_WIDE 0.6f
#define BAR_OVERHANG 0.5f

// How far a caller's colour may be from the one a picker handed back and still
// be that colour, per linear channel: a caller that stores it as floats hands
// back the same bits, and this forgives one that rounds.
#define SAME_COLOUR 1e-4f

static float clamp01(float v)
{
	return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

static voe_math_float3 srgb_of(voe_math_float3 linear)
{
	return (voe_math_float3){ voe_ui_linear_to_srgb(linear.x),
				  voe_ui_linear_to_srgb(linear.y),
				  voe_ui_linear_to_srgb(linear.z) };
}

static voe_math_float3 linear_of(voe_math_float3 srgb)
{
	return (voe_math_float3){ voe_ui_srgb_to_linear(srgb.x),
				  voe_ui_srgb_to_linear(srgb.y),
				  voe_ui_srgb_to_linear(srgb.z) };
}

// sRGB of hue, saturation and value, hue a fraction of a turn and 1 the same
// as 0.
static voe_math_float3 srgb_of_hsv(voe_math_float3 hsv)
{
	float h = (hsv.x - floorf(hsv.x)) * 6.0f;
	float s = clamp01(hsv.y);
	float v = clamp01(hsv.z);
	int sector = (int)h % 6;
	float f = h - floorf(h);
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (sector) {
	case 0:
		return (voe_math_float3){ v, t, p };
	case 1:
		return (voe_math_float3){ q, v, p };
	case 2:
		return (voe_math_float3){ p, v, t };
	case 3:
		return (voe_math_float3){ p, q, v };
	case 4:
		return (voe_math_float3){ t, p, v };
	default:
		return (voe_math_float3){ v, p, q };
	}
}

// HSV of an sRGB colour. A grey has no hue and black no saturation either, so
// those keep `previous`'s: that is the whole of how a hue survives a grey.
static voe_math_float3 hsv_of_srgb(voe_math_float3 c, voe_math_float3 previous)
{
	float high = fmaxf(c.x, fmaxf(c.y, c.z));
	float low = fminf(c.x, fminf(c.y, c.z));
	float d = high - low;
	voe_math_float3 hsv = { previous.x, previous.y, high };

	if (high <= 0.0f)
		return hsv;
	hsv.y = d / high;
	if (d <= 0.0f)
		return hsv;
	if (high == c.x)
		hsv.x = (c.y - c.z) / d;
	else if (high == c.y)
		hsv.x = 2.0f + (c.z - c.x) / d;
	else
		hsv.x = 4.0f + (c.x - c.y) / d;
	hsv.x /= 6.0f;
	if (hsv.x < 0.0f)
		hsv.x += 1.0f;
	return hsv;
}

static bool same_colour(voe_math_float3 a, voe_math_float3 b)
{
	return fabsf(a.x - b.x) <= SAME_COLOUR &&
	       fabsf(a.y - b.y) <= SAME_COLOUR && fabsf(a.z - b.z) <= SAME_COLOUR;
}

static unsigned byte_of(float c)
{
	return (unsigned)lroundf(clamp01(c) * 255.0f);
}

// `#RRGGBB` or `RRGGBB`, any case, and nothing else — no blanks, no three-digit
// form. The three bytes go to `out` as sRGB 0..1.
static bool hex_parse(const char *text, voe_math_float3 *out)
{
	unsigned byte[3];

	if (text[0] == '#')
		text++;
	if (strlen(text) != 6)
		return false;
	for (int i = 0; i < 6; i++)
		if (!isxdigit((unsigned char)text[i]))
			return false;
	for (int i = 0; i < 3; i++) {
		char pair[3] = { text[2 * i], text[2 * i + 1], '\0' };

		byte[i] = (unsigned)strtoul(pair, NULL, 16);
	}
	*out = (voe_math_float3){ byte[0] / 255.0f, byte[1] / 255.0f,
				  byte[2] / 255.0f };
	return true;
}

static const struct voe_ui_colour_memory *memory_of(const voe_ui_context *ui,
						    uint64_t key)
{
	for (uint32_t i = 0; i < ui->picker_remembered; i++)
		if (ui->picker_memory[i].key == key)
			return &ui->picker_memory[i];
	return NULL;
}

voe_ui_node voe_ui_swatch(voe_ui_context *ui, voe_math_float3 linear,
			  voe_ui_sizing sizing)
{
	voe_ui_node node;

	VOE_BASE_ASSERT(ui != NULL, "adding a swatch to no context");

	node = voe_ui_box(ui, (voe_math_float2){ 0.0f, 0.0f }, sizing);
	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_SWATCH;
		ui->widgets[node].colour = linear;
	}
	return node;
}

// A keyed leaf of the picker that takes the pointer: the square or the strip.
static uint32_t picker_part(voe_ui_context *ui, const char *name,
			    enum voe_ui_widget kind, float high,
			    voe_math_float3 hsv)
{
	uint64_t key = voe_ui_widget_claim(ui, name, 0);
	voe_ui_node node = voe_ui_box(
		ui, (voe_math_float2){ 0.0f, 0.0f },
		(voe_ui_sizing){ .along = { VOE_UI_SIZE_FIXED, high },
				 .across = { VOE_UI_SIZE_FIXED, SQUARE_SIDE } });

	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = kind;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
		ui->widgets[node].theme = voe_ui_theme_current(ui);
		ui->widgets[node].colour = hsv;
	}
	return node;
}

voe_ui_node voe_ui_colour_picker(voe_ui_context *ui, const char *name,
				 uint32_t index, voe_math_float3 linear)
{
	const struct voe_ui_colour_memory *memory;
	struct voe_ui_colour_picker *p;
	voe_math_float3 srgb;
	voe_ui_node node;
	const voe_ui_theme *theme = voe_ui_theme_current(ui);
	float pad;

	VOE_BASE_ASSERT(ui != NULL, "opening a colour picker on no context");
	VOE_BASE_ASSERT(name != NULL,
			"a colour picker with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a colour picker needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	// The pad and gap follow the theme's spacing; the square, the strip,
	// the markers and the cells do not.
	pad = PICKER_PAD * theme->spacing;
	node = voe_ui_panel_begin(
		ui, name, index, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){ .gap = PICKER_GAP * theme->spacing,
				    .pad = { pad, pad, pad, pad } });
	// A panel refused for want of a node refuses the frame; nothing inside
	// it is built, and the memory is left as it was.
	if (node == VOE_UI_NODE_NONE) {
		voe_ui_end(ui);
		return node;
	}
	ui->widgets[node].kind = VOE_UI_WIDGET_COLOUR_PICKER;
	ui->widgets[node].colour = linear;

	if (ui->picker_count == VOE_UI_COLOUR_PICKERS) {
		if (!ui->picker_overrun)
			VOE_BASE_ERROR("ui",
				       "colour picker %u refused, a frame has "
				       "room for %u",
				       ui->picker_count + 1,
				       VOE_UI_COLOUR_PICKERS);
		ui->picker_overrun = true;
		voe_ui_end(ui);
		return node;
	}

	p = &ui->pickers[ui->picker_count++];
	*p = (struct voe_ui_colour_picker){ .key = ui->widgets[node].key,
					    .node = node,
					    .linear = linear,
					    .value = linear };
	memory = memory_of(ui, p->key);
	if (memory != NULL && same_colour(memory->linear, linear))
		p->hsv = memory->hsv;
	else
		p->hsv = hsv_of_srgb(srgb_of(linear),
				     memory != NULL
					     ? memory->hsv
					     : (voe_math_float3){ 0.0f, 0.0f, 0.0f });
	p->refused_showing = memory != NULL && memory->refused;

	srgb = srgb_of_hsv(p->hsv);
	snprintf(p->hex, sizeof p->hex, "#%02X%02X%02X", byte_of(srgb.x),
		 byte_of(srgb.y), byte_of(srgb.z));

	p->square = picker_part(ui, "square", VOE_UI_WIDGET_COLOUR_SQUARE,
				SQUARE_SIDE, p->hsv);
	p->strip = picker_part(ui, "hue", VOE_UI_WIDGET_COLOUR_HUE, STRIP_HIGH,
			       p->hsv);
	p->field = voe_ui_field(
		ui, "hex", 0, p->hex,
		(voe_ui_sizing){ .across = { VOE_UI_SIZE_FIXED, SQUARE_SIDE } });
	if (p->refused_showing)
		voe_ui_label_role(ui, "not #RRGGBB", VOE_UI_TEXT_ROLE_SECONDARY);
	voe_ui_end(ui);

	return node;
}

// ------------------------------------------------------------- the frame

void voe_ui_colour_frame_begin(voe_ui_context *ui)
{
	ui->picker_count = 0;
	ui->picker_overrun = false;
}

// Where the pointer is across `rect`, 0 at its left or top and 1 at its right
// or bottom, clamped: a drag off the edge holds the edge's value.
static float across(voe_ui_rect rect, float at, bool y)
{
	float min = y ? rect.min.y : rect.min.x;
	float size = y ? rect.size.y : rect.size.x;

	return size > 0.0f ? clamp01((at - min) / size) : 0.0f;
}

static bool held_now(const voe_ui_context *ui, uint32_t part)
{
	return part != VOE_UI_NODE_NONE && ui->pointer.down && ui->held_set &&
	       ui->held == ui->widgets[part].key;
}

// One picker's frame: a hex commit, then a press or drag in the square or
// strip, then whether the press was outside.
static void picker_resolve(voe_ui_context *ui, struct voe_ui_colour_picker *p,
			   bool pressed)
{
	voe_math_float3 hsv = p->hsv;
	voe_math_float3 value = p->linear;
	bool moved = false;
	voe_math_float2 at = ui->pointer.at;
	voe_ui_rect whole = ui->nodes[p->node].rect;

	if (p->field != VOE_UI_NODE_NONE) {
		voe_math_float3 typed;
		voe_math_float3 shown;

		if (ui->widgets[p->field].changed)
			p->refused_showing = false;
		if (ui->widgets[p->field].committed) {
			const char *text = ui->widgets[p->field + 1].text;

			if (!hex_parse(text, &typed)) {
				p->refused = true;
				p->refused_showing = true;
			} else {
				p->refused_showing = false;
				// A commit of what it already showed moves
				// nothing, so a grey's hue is not lost to it.
				(void)hex_parse(p->hex, &shown);
				if (typed.x != shown.x || typed.y != shown.y ||
				    typed.z != shown.z) {
					hsv = hsv_of_srgb(typed, hsv);
					value = linear_of(typed);
					moved = true;
				}
			}
		}
	}

	if (held_now(ui, p->square)) {
		voe_ui_rect r = ui->nodes[p->square].rect;

		hsv.y = across(r, at.x, false);
		hsv.z = 1.0f - across(r, at.y, true);
		value = linear_of(srgb_of_hsv(hsv));
		moved = true;
	} else if (held_now(ui, p->strip)) {
		hsv.x = across(ui->nodes[p->strip].rect, at.x, false);
		value = linear_of(srgb_of_hsv(hsv));
		moved = true;
	}

	if (moved) {
		p->hsv = hsv;
		p->value = value;
		p->changed = value.x != p->linear.x ||
			     value.y != p->linear.y || value.z != p->linear.z;
	}
	p->outside = pressed &&
		     !(at.x >= whole.min.x && at.x < whole.min.x + whole.size.x &&
		       at.y >= whole.min.y && at.y < whole.min.y + whole.size.y);
}

void voe_ui_colour_frame_end(voe_ui_context *ui, bool laid_out, bool pressed)
{
	// A refused frame has no rectangles to press in and leaves the memory
	// as it was; every picker answers with the colour it was handed.
	if (!laid_out)
		return;

	for (uint32_t i = 0; i < ui->picker_count; i++) {
		struct voe_ui_colour_picker *p = &ui->pickers[i];

		picker_resolve(ui, p, pressed);
		ui->picker_memory[i] = (struct voe_ui_colour_memory){
			.key = p->key,
			.hsv = p->hsv,
			.linear = p->value,
			.refused = p->refused_showing,
		};
	}
	ui->picker_remembered = ui->picker_count;
}

// ------------------------------------------------------------- emission

static voe_math_float4 opaque(voe_math_float3 linear)
{
	return (voe_math_float4){ linear.x, linear.y, linear.z, 1.0f };
}

static voe_math_float4 cell_colour(voe_math_float3 hsv)
{
	return opaque(linear_of(srgb_of_hsv(hsv)));
}

// A square of side `side` centred on `at`.
static voe_ui_rect centred(voe_math_float2 at, float side)
{
	return (voe_ui_rect){ .min = { at.x - side * 0.5f, at.y - side * 0.5f },
			      .size = { side, side } };
}

static void emit_square(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_ui_rect r = ui->nodes[node].rect;
	float wide = r.size.x / SQUARE_CELLS;
	float high = r.size.y / SQUARE_CELLS;
	voe_math_float2 mark = { r.min.x + w->colour.y * r.size.x,
				 r.min.y + (1.0f - w->colour.z) * r.size.y };

	for (int row = 0; row < SQUARE_CELLS; row++)
		for (int col = 0; col < SQUARE_CELLS; col++) {
			voe_math_float3 hsv = {
				w->colour.x, (col + 0.5f) / SQUARE_CELLS,
				1.0f - (row + 0.5f) / SQUARE_CELLS
			};

			voe_ui_push_solid(
				ui, node,
				(voe_ui_rect){ .min = { r.min.x + col * wide,
							r.min.y + row * high },
					       .size = { wide, high } },
				cell_colour(hsv));
		}
	voe_ui_push_solid(ui, node, centred(mark, MARKER_OUTER),
			  w->theme->text_primary);
	voe_ui_push_solid(ui, node, centred(mark, MARKER_INNER),
			  cell_colour(w->colour));
}

static void emit_strip(voe_ui_context *ui, uint32_t node)
{
	const struct voe_ui_widget_record *w = &ui->widgets[node];
	voe_ui_rect r = ui->nodes[node].rect;
	float wide = r.size.x / STRIP_CELLS;
	float mark = r.min.x + (w->colour.x - floorf(w->colour.x)) * r.size.x;

	for (int col = 0; col < STRIP_CELLS; col++)
		voe_ui_push_solid(
			ui, node,
			(voe_ui_rect){ .min = { r.min.x + col * wide, r.min.y },
				       .size = { wide, r.size.y } },
			cell_colour((voe_math_float3){
				(col + 0.5f) / STRIP_CELLS, 1.0f, 1.0f }));
	voe_ui_push_solid(
		ui, node,
		(voe_ui_rect){ .min = { mark - BAR_WIDE * 0.5f,
					r.min.y - BAR_OVERHANG },
			       .size = { BAR_WIDE,
					 r.size.y + 2.0f * BAR_OVERHANG } },
		w->theme->text_primary);
}

void voe_ui_colour_emit(voe_ui_context *ui, uint32_t node)
{
	switch (ui->widgets[node].kind) {
	case VOE_UI_WIDGET_SWATCH:
		voe_ui_push_solid(ui, node, ui->nodes[node].rect,
				  opaque(ui->widgets[node].colour));
		break;
	case VOE_UI_WIDGET_COLOUR_SQUARE:
		emit_square(ui, node);
		break;
	case VOE_UI_WIDGET_COLOUR_HUE:
		emit_strip(ui, node);
		break;
	default:
		VOE_BASE_ASSERT(false, "emitting a node colour.c did not make");
	}
}

// ------------------------------------------------------------- read back

voe_ui_colour_result voe_ui_colour_picker_action(const voe_ui_context *ui,
						 voe_ui_node picker)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a colour picker on no context");
	VOE_BASE_ASSERT(ui->state == VOE_UI_LAID_OUT,
			"reading a colour picker before the frame has ended");
	VOE_BASE_ASSERT(picker != VOE_UI_NODE_NONE,
			"reading a colour picker the frame had no room for");
	VOE_BASE_ASSERT(picker < ui->count,
			"reading a colour picker this frame never made");
	VOE_BASE_ASSERT(ui->widgets[picker].kind == VOE_UI_WIDGET_COLOUR_PICKER,
			"reading a colour picker action from a node that is not "
			"a colour picker");

	for (uint32_t i = 0; i < ui->picker_count; i++) {
		const struct voe_ui_colour_picker *p = &ui->pickers[i];

		if (p->node == picker)
			return (voe_ui_colour_result){ .changed = p->changed,
						       .value = p->value,
						       .outside = p->outside,
						       .refused = p->refused };
	}
	// One past the frame's ceiling: built empty, and it answers with what
	// it was handed.
	return (voe_ui_colour_result){ .value = ui->widgets[picker].colour };
}
