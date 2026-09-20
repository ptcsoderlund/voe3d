// The single-line text field, and the number box once a click has opened it for
// typing: where a press takes the focus, what this frame's keys do to the text,
// and what a commit and a cancel leave behind. The number box's drag and its
// dead zone are `ui/tests/button.c`; what every widget shares is
// `ui/tests/widgets.c`.
//
// A FIELD FOCUSES ON THE PRESS ITSELF, NOT ON A RELEASE, and the cases below
// pin that down before anything about editing: a press elsewhere — nothing, a
// button, wherever — clears it on the very same edge a press on the field
// sets it, and a field not called this frame loses it exactly as a scroll
// area not called forgets its offset. TWO FIELDS IN ONE FRAME, ONLY ONE
// FOCUSED, is the case that would catch a shared buffer: typing must reach
// the focused one and leave the other's own text untouched. BACKSPACE ON AN
// EMPTY FIELD AND A CODE POINT TAKEN WHOLE both come from the same walk
// backward over continuation bytes, so both are pinned down rather than
// trusted to follow from one another. A TEXT AT CAPACITY REFUSING THE NEXT
// CODE POINT WHOLE is the one that would show a cut multi-byte character if
// the append ever stopped counting in bytes instead of code points. AND
// `changed` FALSE HANDS BACK THE CALLER'S OWN POINTER, not a copy of the same
// bytes, which is what lets a caller write the answer back every frame with no
// cost on the frames that changed nothing.
//
// THE SELECTED TEXT IS DRAWN INVERTED (ADR-0196), and that is one record of
// `inverse` behind the letters and the letters themselves in `inverse_ink`, so
// both halves of the pair are checked together: either one alone is text that
// cannot be read.
//
// A NUMBER BOX IS OPENED BY A CLICK AND NEVER BY A DRAG, and once it is open it
// is a field: a typed number is taken on Enter, an unchanged text changes
// nothing, a refused one stays open until Escape, and Tab commits and opens the
// next box.
//
// EVERY CASE IN THIS FILE NEEDS A DEVICE, AND IT IS DELIBERATE (ADR-0106). A
// FIELD COMPOSES A LABEL OF ITS OWN and so does an open number box, so proving
// any of this needs a real font, and a font uploads an atlas and so needs a
// device. These cases take a headless one — no window, no surface, no
// compositor — and where there is no driver at all they skip, saying which
// check did not run rather than only why.
//
// THE GEOMETRY IS WORKED OUT BY HAND AND WRITTEN AS NUMBERS. A field's
// rectangle comes out of layout, so a test that asked layout where the field
// was and then clicked there would pass with the arithmetic inverted. The
// numbers below say where the field and the number boxes are meant to be.
#include <ui/layout.h>
#include <ui/widgets.h>

#include <base/arena.h>
#include <math/float2.h>
#include <math/float4.h>
#include <render/device.h>
#include <text/font.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH 65536

// The box each number box below holds, and the y the pointer keeps on one: a
// number box round a 20 x 10 box is 25 x 15, so 7 is the middle of it.
#define BOX_WIDE 20.0f
#define BOX_HIGH 10.0f
#define ON_NUMBER_Y 7.0f

// Two units of value per millimetre of drag, so that a distance and a value
// cannot be confused for one another by coming out the same number — which they
// would at one.
#define PER_MM 2.0
// What the caller hands in every frame. The widget never keeps it, so every
// result below is this plus THIS frame's drag and never a running total.
#define START 100.0

// The theme every context in this file draws with, set once in main() — a
// panel, a field and a number box each need one in force to be built at all,
// and a field's ink, caret and selection all come out of its roles.
static voe_ui_theme TEST_THEME;

// A field 25 x 15 at the origin of a bare panel, so a point inside it and one
// well outside it are known without asking layout — the same premise the button
// cases rest on. A field always composes a label, which is why every case in
// this file runs inside the device group at its end.
#define FIELD_WIDE 25.0f
#define FIELD_HIGH 15.0f
#define IN_FIELD ((voe_math_float2){ 12.0f, 7.0f })
#define OUTSIDE_FIELD ((voe_math_float2){ 100.0f, 100.0f })

static const voe_ui_keyboard NO_KEYS = { 0 };

static voe_ui_sizing field_sizing(void)
{
	return (voe_ui_sizing){ { VOE_UI_SIZE_FIXED, FIELD_WIDE },
				{ VOE_UI_SIZE_FIXED, FIELD_HIGH } };
}

struct field_frame {
	voe_ui_node f;
	bool ok;
};

static struct field_frame build_field(voe_ui_context *ui,
				      voe_base_arena *arena,
				      voe_math_float2 at, bool over, bool down,
				      const char *text, voe_ui_keyboard keyboard)
{
	struct field_frame f = { VOE_UI_NODE_NONE, false };

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = over,
						 .down = down });
	voe_ui_keyboard_set(ui, keyboard);

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	f.f = voe_ui_field(ui, "name", 0, text, field_sizing());
	voe_ui_end(ui);

	f.ok = voe_ui_frame_end(ui);
	return f;
}

// A press inside it focuses it, on the press and not on the release that
// follows; a press outside it — nothing there, just empty panel — clears it
// on that same edge.
static void a_press_focuses_and_a_press_elsewhere_unfocuses(
	voe_ui_context *ui, voe_base_arena *arena)
{
	struct field_frame f =
		build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);

	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	// Let go: a release does not clear it, unlike a button's press.
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	// A press outside it, on empty panel, clears it.
	f = build_field(ui, arena, OUTSIDE_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);
}

static void typed_bytes_are_appended_in_order(voe_ui_context *ui,
					      voe_base_arena *arena)
{
	voe_ui_keyboard type_ab = { .text = "ab", .size = 2 };
	voe_ui_field_result r;
	struct field_frame f;

	// Nothing is focused before the press, so the press below is an edge
	// and not whatever the case before this left behind.
	f = build_field(ui, arena, IN_FIELD, true, false, "", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, true, true, "", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", type_ab);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "ab") == 0);
}

// Two fields, one focused: typing must reach the one focused — replacing
// its whole text, the focus having just arrived — and leave the other's own
// text exactly as it was handed in.
static void two_fields_only_the_focused_one_changes(voe_ui_context *ui,
						    voe_base_arena *arena)
{
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_node a;
	voe_ui_node b;
	voe_ui_field_result ra;
	voe_ui_field_result rb;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = IN_FIELD,
						 .over = true,
						 .down = true });
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	a = voe_ui_field(ui, "a", 0, "one", field_sizing());
	(void)voe_ui_field(ui, "b", 0, "two", field_sizing());
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	VOE_TEST_CHECK(voe_ui_field_action(ui, a).focused);

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ 0 });
	voe_ui_keyboard_set(ui, type_x);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	a = voe_ui_field(ui, "a", 0, "one", field_sizing());
	b = voe_ui_field(ui, "b", 0, "two", field_sizing());
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	ra = voe_ui_field_action(ui, a);
	rb = voe_ui_field_action(ui, b);
	VOE_TEST_CHECK(ra.focused);
	VOE_TEST_CHECK(ra.changed);
	VOE_TEST_CHECK(strcmp(ra.text, "x") == 0);
	VOE_TEST_CHECK(!rb.focused);
	VOE_TEST_CHECK(!rb.changed);
	VOE_TEST_CHECK(strcmp(rb.text, "two") == 0);
}

// Focuses the one field of build_field, text `text`, and lets go. A press
// outside first, so the focus arrives rather than staying where the case
// before left it.
static void focus_field(voe_ui_context *ui, voe_base_arena *arena,
			const char *text)
{
	(void)build_field(ui, arena, OUTSIDE_FIELD, true, true, text, NO_KEYS);
	(void)build_field(ui, arena, OUTSIDE_FIELD, true, false, text, NO_KEYS);
	(void)build_field(ui, arena, IN_FIELD, true, true, text, NO_KEYS);
	(void)build_field(ui, arena, IN_FIELD, true, false, text, NO_KEYS);
}

// The trailing continuation byte and the byte before it both go: "aö" loses
// the whole of "ö" (0xC3 0xB6) and not just its last byte.
static void backspace_takes_a_two_byte_code_point_whole(voe_ui_context *ui,
							voe_base_arena *arena)
{
	voe_ui_keyboard erase = { .backspace = true };
	voe_ui_keyboard type_it = { .text = "a\xc3" "\xb6", .size = 3 };
	struct field_frame f;
	voe_ui_field_result r;

	// Typed rather than handed in, so no selection is left for the
	// Backspace to empty.
	focus_field(ui, arena, "");
	f = build_field(ui, arena, IN_FIELD, true, false, "", type_it);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", erase);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "a") == 0);
}

static void backspace_on_an_empty_text_does_nothing(voe_ui_context *ui,
						    voe_base_arena *arena)
{
	voe_ui_keyboard erase = { .backspace = true };
	struct field_frame f;

	focus_field(ui, arena, "");
	f = build_field(ui, arena, IN_FIELD, true, false, "", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", erase);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).changed);
}

// A text already at capacity refuses the next code point whole rather than
// cutting it at a byte the capacity happens to allow.
static void a_text_at_capacity_refuses_the_next_code_point_whole(
	voe_ui_context *ui, voe_base_arena *arena)
{
	static char full[VOE_UI_FIELD_CAPACITY + 1];
	voe_ui_keyboard type_one = { .text = "x", .size = 1 };
	struct field_frame f;
	voe_ui_field_result r;

	memset(full, 'a', VOE_UI_FIELD_CAPACITY);
	full[VOE_UI_FIELD_CAPACITY] = '\0';

	voe_ui_keyboard type_full = { .text = full,
				      .size = VOE_UI_FIELD_CAPACITY };

	// Typed to capacity rather than handed in, so no selection is left
	// for the next letter to replace.
	focus_field(ui, arena, "");
	f = build_field(ui, arena, IN_FIELD, true, false, "", type_full);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "", type_one);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(strlen(r.text) == VOE_UI_FIELD_CAPACITY);
}

// Enter is true for exactly the frame it arrived on, and only while focused;
// it changes no text, commits, and drops the focus.
static void entered_is_true_only_on_the_frame_enter_arrived(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard press_enter = { .enter = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hi");
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).entered);

	f = build_field(ui, arena, IN_FIELD, false, false, "hi", press_enter);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.entered);
	VOE_TEST_CHECK(r.committed);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.focused);

	f = build_field(ui, arena, IN_FIELD, false, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).entered);
}

// A field not called this frame is no longer the focus, exactly as a scroll
// area not called forgets its offset.
static void a_field_not_called_loses_focus(voe_ui_context *ui,
					   voe_base_arena *arena)
{
	struct field_frame f;
	bool ok;

	(void)build_field(ui, arena, IN_FIELD, true, true, "hi", NO_KEYS);
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_field_action(ui, f.f).focused);

	// A frame with no field in it at all.
	voe_ui_frame_begin(ui, arena);
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	voe_ui_end(ui);
	ok = voe_ui_frame_end(ui);
	VOE_TEST_CHECK(ok);

	f = build_field(ui, arena, OUTSIDE_FIELD, false, false, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_field_action(ui, f.f).focused);
}

// voe_ui_field_focus takes the keyboard to a field with no press at all —
// the frame a panel holding the one field first opens.
static void voe_ui_field_focus_takes_it(voe_ui_context *ui,
					voe_base_arena *arena)
{
	voe_ui_node f;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ 0 });
	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	f = voe_ui_field(ui, "name", 0, "hi", field_sizing());
	voe_ui_field_focus(ui, f);
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));

	VOE_TEST_CHECK(voe_ui_field_action(ui, f).focused);
}

// A field that is not focused hands back the exact pointer the call was
// given, not a copy of the same bytes — so a caller may write the answer back
// every frame at no cost.
static void unfocused_hands_back_the_callers_own_pointer(
	voe_ui_context *ui, voe_base_arena *arena)
{
	static const char hello[] = "hello";
	struct field_frame f;
	voe_ui_field_result r;

	build_field(ui, arena, OUTSIDE_FIELD, true, true, hello, NO_KEYS);
	f = build_field(ui, arena, OUTSIDE_FIELD, false, false, hello, NO_KEYS);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(r.text == hello);
}

// The focus arrives with the whole text selected: the first typed text
// replaces it, and the next is appended as before.
static void typing_into_a_newly_focused_field_replaces_its_text(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard type_y = { .text = "y", .size = 1 };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hello");
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", type_x);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "x") == 0);

	// The caller's "hello" is not read while focused.
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", type_y);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(strcmp(r.text, "xy") == 0);
}

static void backspace_on_a_newly_focused_field_empties_it(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard erase = { .backspace = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hello");
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", erase);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "") == 0);
}

// Two colours are the same role, to the tolerance every other check here uses.
// Alpha is not compared: every record below is opaque.
static bool same_colour(voe_math_float4 a, voe_math_float4 b)
{
	return fabsf(a.x - b.x) < 0.001f && fabsf(a.y - b.y) < 0.001f &&
	       fabsf(a.z - b.z) < 0.001f;
}

// THE WHOLE TEXT SELECTED IS DRAWN INVERTED (ADR-0196). A field just focused
// arrives selected, so that frame carries one record of `inverse` behind the
// letters and every letter of "hi" in `inverse_ink`. The panel's border and
// fill and the field's own fill are the other records before them.
//
// ONLY THE RECORDS BEFORE THE FIRST LETTER ARE COUNTED, because `inverse` is a
// fill AT text_primary's lightness and in the same hue, so the caret — drawn in
// text_primary after the letters — is the very same colour and counting the
// whole list would find two.
static void a_selected_text_is_inverted(voe_ui_context *ui,
					voe_base_arena *arena)
{
	uint32_t count;
	uint32_t glyphs = 0;
	uint32_t first_glyph;
	uint32_t behind = 0;

	focus_field(ui, arena, "hi");
	count = voe_ui_element_count(ui);
	first_glyph = count;

	for (uint32_t i = 0; i < count; i++) {
		voe_render_element e = voe_ui_element(ui, i);

		if (e.kind != VOE_RENDER_ELEMENT_GLYPH)
			continue;
		if (glyphs == 0)
			first_glyph = i;
		glyphs++;
		VOE_TEST_CHECK(same_colour(e.colour, TEST_THEME.inverse_ink));
	}

	for (uint32_t i = 0; i < first_glyph; i++)
		if (same_colour(voe_ui_element(ui, i).colour,
				TEST_THEME.inverse))
			behind++;

	VOE_TEST_CHECK_INT((int)glyphs, 2);
	VOE_TEST_CHECK_INT((int)behind, 1);
}

// Escape cancels: the focus drops, `text` is the caller's own that frame,
// and the next frame shows the caller's text again.
static void escape_cancels_and_the_callers_text_stands(voe_ui_context *ui,
						       voe_base_arena *arena)
{
	static const char hello[] = "hello";
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard escape = { .escape = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, hello);
	(void)build_field(ui, arena, IN_FIELD, true, false, hello, type_x);
	f = build_field(ui, arena, IN_FIELD, true, false, hello, escape);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.cancelled);
	VOE_TEST_CHECK(!r.committed);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(r.text == hello);

	f = build_field(ui, arena, IN_FIELD, true, false, hello, NO_KEYS);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(!r.cancelled);
	VOE_TEST_CHECK(r.text == hello);
}

// Enter and a press elsewhere each commit with the typed text.
static void enter_and_a_press_elsewhere_commit_the_typed_text(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard press_enter = { .enter = true };
	struct field_frame f;
	voe_ui_field_result r;

	focus_field(ui, arena, "hello");
	(void)build_field(ui, arena, IN_FIELD, true, false, "hello", type_x);
	f = build_field(ui, arena, IN_FIELD, true, false, "hello", press_enter);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.committed);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(strcmp(r.text, "x") == 0);

	focus_field(ui, arena, "hello");
	(void)build_field(ui, arena, IN_FIELD, true, false, "hello", type_x);
	f = build_field(ui, arena, OUTSIDE_FIELD, true, true, "hello", NO_KEYS);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(r.committed);
	VOE_TEST_CHECK(!r.entered);
	VOE_TEST_CHECK(!r.focused);
	VOE_TEST_CHECK(strcmp(r.text, "x") == 0);
}

struct two_fields {
	voe_ui_field_result a;
	voe_ui_field_result b;
};

// Fields "a" and "b" in a column, "a" at the origin, with no pointer.
static struct two_fields build_two_fields(voe_ui_context *ui,
					  voe_base_arena *arena,
					  voe_ui_pointer pointer,
					  voe_ui_keyboard keyboard)
{
	struct two_fields r;
	voe_ui_node a;
	voe_ui_node b;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, pointer);
	voe_ui_keyboard_set(ui, keyboard);
	voe_ui_column_begin(ui, (voe_ui_container){ 0 });
	a = voe_ui_field(ui, "a", 0, "one", field_sizing());
	b = voe_ui_field(ui, "b", 0, "two", field_sizing());
	voe_ui_end(ui);
	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	r.a = voe_ui_field_action(ui, a);
	r.b = voe_ui_field_action(ui, b);
	return r;
}

// Tab from the first commits it and focuses the second; Tab from the second
// wraps to the first.
static void tab_moves_the_focus_to_the_next_field_and_wraps(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_pointer press = { .at = IN_FIELD, .over = true, .down = true };
	voe_ui_pointer none = { 0 };
	voe_ui_keyboard type_x = { .text = "x", .size = 1 };
	voe_ui_keyboard tab = { .tab = true };
	struct two_fields r;

	(void)build_two_fields(ui, arena, none, NO_KEYS);
	r = build_two_fields(ui, arena, press, NO_KEYS);
	VOE_TEST_CHECK(r.a.focused);
	(void)build_two_fields(ui, arena, none, type_x);

	r = build_two_fields(ui, arena, none, tab);
	VOE_TEST_CHECK(r.a.committed);
	VOE_TEST_CHECK(strcmp(r.a.text, "x") == 0);
	VOE_TEST_CHECK(!r.a.focused);
	VOE_TEST_CHECK(r.b.focused);

	// The second opens the next frame, its whole text selected.
	r = build_two_fields(ui, arena, none, type_x);
	VOE_TEST_CHECK(r.b.focused);
	VOE_TEST_CHECK(strcmp(r.b.text, "x") == 0);

	r = build_two_fields(ui, arena, none, tab);
	VOE_TEST_CHECK(r.b.committed);
	VOE_TEST_CHECK(r.a.focused);
	VOE_TEST_CHECK(!r.b.focused);
}

// A 0x09 or 0x7F byte in `text` is not appended; the bytes around it are.
static void control_bytes_are_not_appended(voe_ui_context *ui,
					   voe_base_arena *arena)
{
	voe_ui_keyboard typed = { .text = "a\tb\x7f" "c", .size = 5 };
	voe_ui_keyboard only_tab = { .text = "\t", .size = 1 };
	struct field_frame f;
	voe_ui_field_result r;

	// A control byte alone does not replace the selection either.
	focus_field(ui, arena, "hi");
	f = build_field(ui, arena, IN_FIELD, true, false, "hi", only_tab);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(strcmp(r.text, "hi") == 0);

	f = build_field(ui, arena, IN_FIELD, true, false, "hi", typed);
	r = voe_ui_field_action(ui, f.f);
	VOE_TEST_CHECK(strcmp(r.text, "abc") == 0);
}

// voe_ui_typing is true exactly while a field held the focus at the end of
// the last frame.
static void typing_follows_the_focus(voe_ui_context *ui,
				     voe_base_arena *arena)
{
	voe_ui_keyboard press_enter = { .enter = true };

	(void)build_field(ui, arena, OUTSIDE_FIELD, true, false, "hi", NO_KEYS);
	(void)build_field(ui, arena, OUTSIDE_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_typing(ui));
	(void)build_field(ui, arena, IN_FIELD, true, false, "hi", NO_KEYS);
	(void)build_field(ui, arena, IN_FIELD, true, true, "hi", NO_KEYS);
	VOE_TEST_CHECK(voe_ui_typing(ui));
	(void)build_field(ui, arena, IN_FIELD, true, false, "hi", press_enter);
	VOE_TEST_CHECK(!voe_ui_typing(ui));
}

// Two number boxes, "n" handed START over "m" handed M_START, each holding one
// 20 x 10 box. "n" is at the origin, so the point ON_N is inside it whether it
// is open or not — open, it only grows. A click is all these cases do with the
// pointer; the rest is the keyboard. Open, a number box composes a label, which
// is why these run in the device group with the fields.
#define M_START 7.0
#define ON_N ((voe_math_float2){ 10.0f, ON_NUMBER_Y })

struct numbers_frame {
	voe_ui_node n;
	voe_ui_node m;
};

static struct numbers_frame build_numbers(voe_ui_context *ui,
					  voe_base_arena *arena,
					  voe_math_float2 at, bool down,
					  voe_ui_keyboard keyboard)
{
	struct numbers_frame f;

	voe_ui_frame_begin(ui, arena);
	voe_ui_pointer_set(ui, (voe_ui_pointer){ .at = at,
						 .over = true,
						 .down = down });
	voe_ui_keyboard_set(ui, keyboard);

	voe_ui_panel_begin(ui, "panel", 0, VOE_UI_SURFACE_SURFACE,
			   (voe_ui_container){ 0 });
	f.n = voe_ui_number_begin(ui, "n", 0, START, PER_MM);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	f.m = voe_ui_number_begin(ui, "m", 0, M_START, PER_MM);
	voe_ui_box(ui, (voe_math_float2){ BOX_WIDE, BOX_HIGH },
		   (voe_ui_sizing){ 0 });
	voe_ui_end(ui);
	voe_ui_end(ui);

	VOE_TEST_CHECK(voe_ui_frame_end(ui));
	return f;
}

// A press and a release far away, which closes whatever an earlier case left
// open, then a click on "n" — which opens it.
static void open_n(voe_ui_context *ui, voe_base_arena *arena)
{
	(void)build_numbers(ui, arena, OUTSIDE_FIELD, true, NO_KEYS);
	(void)build_numbers(ui, arena, OUTSIDE_FIELD, false, NO_KEYS);
	(void)build_numbers(ui, arena, ON_N, true, NO_KEYS);
	(void)build_numbers(ui, arena, ON_N, false, NO_KEYS);
}

static voe_ui_keyboard typing(const char *text)
{
	return (voe_ui_keyboard){ .text = text,
				  .size = (uint32_t)strlen(text) };
}

// A CLICK OPENS IT AND A DRAG NEVER DOES. Clicked, it is typing on the frame
// after the release, draws open with its caller's content hidden, and Escape
// closes it; pressed and dragged 5 mm, nothing on any frame opens it.
static void a_click_opens_the_box_and_a_drag_never_does(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard escape = { .escape = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
	VOE_TEST_CHECK(voe_ui_typing(ui));
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.m).typing);

	f = build_numbers(ui, arena, ON_N, false, escape);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!voe_ui_typing(ui));

	(void)build_numbers(ui, arena, ON_N, true, NO_KEYS);
	(void)build_numbers(ui, arena, (voe_math_float2){ 15.0f, ON_NUMBER_Y },
			    true, NO_KEYS);
	f = build_numbers(ui, arena, (voe_math_float2){ 15.0f, ON_NUMBER_Y },
			  false, NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).typing);
	f = build_numbers(ui, arena, (voe_math_float2){ 15.0f, ON_NUMBER_Y },
			  false, NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).typing);
}

// Typed, then Enter: the typed number comes back as `value` with `changed`,
// on the Enter frame and not on the typing one, and the box closes.
static void typing_a_number_then_enter_changes_the_value(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard enter = { .enter = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, typing("0.1"));
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(!r.changed);

	f = build_numbers(ui, arena, ON_N, false, enter);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(r.value == 0.1);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(!r.refused);

	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	VOE_TEST_CHECK(!voe_ui_number_action(ui, f.n).changed);
}

// Opened and entered with nothing typed: the text is what it opened with, so
// nothing changes — not even the value going through `%.6g` and back.
static void enter_with_nothing_typed_changes_nothing(voe_ui_context *ui,
						     voe_base_arena *arena)
{
	voe_ui_keyboard enter = { .enter = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, enter);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.refused);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
}

// Not a number, then Enter: refused, nothing changed, still open — and the
// next frame says so with one more label's worth of letters. Escape then
// closes it with the caller's value standing.
static void a_refused_enter_stays_open_and_escape_closes(
	voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_keyboard enter = { .enter = true };
	voe_ui_keyboard escape = { .escape = true };
	struct numbers_frame f;
	voe_ui_number_result r;
	uint32_t before;

	open_n(ui, arena);
	build_numbers(ui, arena, ON_N, false, typing("abc"));
	before = voe_ui_element_count(ui);
	f = build_numbers(ui, arena, ON_N, false, enter);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(voe_ui_typing(ui));

	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.typing);
	VOE_TEST_CHECK(!r.refused);
	VOE_TEST_CHECK(voe_ui_element_count(ui) > before);

	f = build_numbers(ui, arena, ON_N, false, escape);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)START, 0.001f);
}

// `3` then Tab: "n" takes 3 and the focus goes to "m", which opens; Escape
// there leaves "m"'s value as it was handed in.
static void tab_commits_and_opens_the_next_number_box(voe_ui_context *ui,
						      voe_base_arena *arena)
{
	voe_ui_keyboard tab = { .text = "3", .size = 1, .tab = true };
	voe_ui_keyboard escape = { .escape = true };
	struct numbers_frame f;
	voe_ui_number_result r;

	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, tab);
	r = voe_ui_number_action(ui, f.n);
	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(r.value == 3.0);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.m).typing);

	f = build_numbers(ui, arena, ON_N, false, NO_KEYS);
	VOE_TEST_CHECK(voe_ui_number_action(ui, f.m).typing);

	f = build_numbers(ui, arena, ON_N, false, escape);
	r = voe_ui_number_action(ui, f.m);
	VOE_TEST_CHECK(!r.changed);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK_FLOAT((float)r.value, (float)M_START, 0.001f);
}

// Opens "n", types `text` and presses Enter, and hands back that frame's
// answer; a box left open by a refusal is closed by the next open_n.
static voe_ui_number_result enter_typed(voe_ui_context *ui,
					voe_base_arena *arena,
					const char *text)
{
	voe_ui_keyboard enter = typing(text);
	struct numbers_frame f;

	enter.enter = true;
	open_n(ui, arena);
	f = build_numbers(ui, arena, ON_N, false, enter);
	return voe_ui_number_action(ui, f.n);
}

// Blanks around a number are allowed; an infinity, a NaN and trailing
// letters are not one finite number, and are refused.
static void what_counts_as_a_number(voe_ui_context *ui, voe_base_arena *arena)
{
	voe_ui_number_result r = enter_typed(ui, arena, " 2.5 ");

	VOE_TEST_CHECK(r.changed);
	VOE_TEST_CHECK(!r.refused);
	VOE_TEST_CHECK(r.value == 2.5);

	r = enter_typed(ui, arena, "1e999");
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	r = enter_typed(ui, arena, "nan");
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);
	r = enter_typed(ui, arena, "2x");
	VOE_TEST_CHECK(r.refused);
	VOE_TEST_CHECK(!r.changed);

	// And a press elsewhere closes a refused box, changing nothing.
	r = voe_ui_number_action(
		ui, build_numbers(ui, arena, OUTSIDE_FIELD, true, NO_KEYS).n);
	VOE_TEST_CHECK(!r.typing);
	VOE_TEST_CHECK(!r.changed);
}

// The device and the font every case in this file needs, and the skip that
// stands in for them where there is no driver.
static int the_field(voe_base_arena *arena)
{
	voe_platform_size size = { 64, 64 };
	voe_render_capacities capacities = {
		.vertices = 4,
		.indices = 6,
		.geometries = 1,
		.objects = 1,
		.shadings = 1,
		.passes = 1,
	};
	voe_render_device *device;
	voe_text_font *font;
	voe_ui_context *ui;
	voe_base_error error = VOE_BASE_OK;

	device = voe_render_device_new_headless(arena, size, capacities,
						&error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: no graphics driver — the field checks "
			       "did not run\n");
			return 0;
		}
		VOE_TEST_CHECK(device != NULL);
		return 0;
	}

	// Oxanium: nothing here cares which face, so the engine's default.
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	if (font == NULL) {
		VOE_TEST_CHECK(font != NULL);
		voe_render_device_destroy(device);
		return 0;
	}

	// Elements enough for the capacity case's 256 glyphs plus a few
	// backgrounds and carets; nodes are two per field plus a panel.
	ui = voe_ui_context_new(arena, (voe_ui_capacities){ .nodes = 32,
							    .elements = 512 });
	voe_ui_font_set(ui, font);
	voe_ui_theme_set(ui, &TEST_THEME);

	a_press_focuses_and_a_press_elsewhere_unfocuses(ui, arena);
	typed_bytes_are_appended_in_order(ui, arena);
	two_fields_only_the_focused_one_changes(ui, arena);
	backspace_takes_a_two_byte_code_point_whole(ui, arena);
	backspace_on_an_empty_text_does_nothing(ui, arena);
	a_text_at_capacity_refuses_the_next_code_point_whole(ui, arena);
	entered_is_true_only_on_the_frame_enter_arrived(ui, arena);
	a_field_not_called_loses_focus(ui, arena);
	voe_ui_field_focus_takes_it(ui, arena);
	unfocused_hands_back_the_callers_own_pointer(ui, arena);
	typing_into_a_newly_focused_field_replaces_its_text(ui, arena);
	backspace_on_a_newly_focused_field_empties_it(ui, arena);
	a_selected_text_is_inverted(ui, arena);
	escape_cancels_and_the_callers_text_stands(ui, arena);
	enter_and_a_press_elsewhere_commit_the_typed_text(ui, arena);
	tab_moves_the_focus_to_the_next_field_and_wraps(ui, arena);
	control_bytes_are_not_appended(ui, arena);
	typing_follows_the_focus(ui, arena);
	a_click_opens_the_box_and_a_drag_never_does(ui, arena);
	typing_a_number_then_enter_changes_the_value(ui, arena);
	enter_with_nothing_typed_changes_nothing(ui, arena);
	a_refused_enter_stays_open_and_escape_closes(ui, arena);
	tab_commits_and_opens_the_next_number_box(ui, arena);
	what_counts_as_a_number(ui, arena);

	voe_text_font_destroy(font);
	voe_render_device_destroy(device);
	return 0;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();

	TEST_THEME = voe_ui_theme_derive(&inputs, NULL);

	(void)the_field(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
