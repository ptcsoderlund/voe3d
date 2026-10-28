// The dock layout's lines in `editor_settings`: each line's words taken one at
// a time by a cursor, a node or float line kept only when every word parsed
// and nothing followed, and the root, nodes and floats written back through
// settings.h's keep-other-keys write. See the header for the line forms.
//
// Constraints: the lines are written into a stack buffer of LAYOUT_LINE_ROOM
// per line, room for any finite number `%.3f` or `%.6f` prints; each append
// asserts it fit, so a bound that is wrong stops rather than overruns.
#include "settings_layout.h"

#include "settings.h"

#include <base/assert.h>
#include <base/report.h>

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The three keys this file owns, in the order they are written.
static const char *const LAYOUT_KEYS[] = { "dock_root", "dock_node",
					   "dock_float" };
#define KEY_ROOT 0u
#define KEY_NODE 1u
#define LAYOUT_KEY_COUNT (sizeof LAYOUT_KEYS / sizeof LAYOUT_KEYS[0])

// Each panel's, axis's and hold's word, as 0491 point 7 spells them.
static const char *const PANEL_WORDS[VOE_EDITOR_PANEL_COUNT] = {
	[VOE_EDITOR_PANEL_SCENE] = "scene",
	[VOE_EDITOR_PANEL_INSPECTOR] = "inspector",
	[VOE_EDITOR_PANEL_SCENE_VIEW] = "view",
	[VOE_EDITOR_PANEL_ASSETS] = "assets",
	[VOE_EDITOR_PANEL_FRAME] = "frame",
	[VOE_EDITOR_PANEL_PROJECT] = "project",
	[VOE_EDITOR_PANEL_ERRORS] = "errors",
	[VOE_EDITOR_PANEL_LANDSCAPE] = "landscape",
};
static const char *const AXIS_WORDS[] = { [VOE_EDITOR_DOCK_ROW] = "row",
					  [VOE_EDITOR_DOCK_COLUMN] = "column" };
static const char *const HOLD_WORDS[] = {
	[VOE_EDITOR_DOCK_HOLD_FRACTION] = "fraction",
	[VOE_EDITOR_DOCK_HOLD_FIRST] = "first",
	[VOE_EDITOR_DOCK_HOLD_SECOND] = "second",
};
#define AXIS_COUNT (sizeof AXIS_WORDS / sizeof AXIS_WORDS[0])
#define HOLD_COUNT (sizeof HOLD_WORDS / sizeof HOLD_WORDS[0])

// Room for one written line: a key, three indices, three words and four
// numbers, any finite double `%.6f` printing under 330 characters and any
// float `%.3f` under 50.
#define LAYOUT_LINE_ROOM 512u
// The root, every node and one float per closable.
#define LAYOUT_LINES (1u + VOE_EDITOR_DOCK_NODES + VOE_EDITOR_CLOSABLE_COUNT)

// A line's text being read: where the next word starts, and false once any
// word failed to parse.
typedef struct {
	const char *at;
	bool ok;
} line_cursor;

// One word of a line: where it starts and how long it is.
typedef struct {
	const char *start;
	size_t length;
} line_word;

// What a read is filling: the tree and floats, whether a root line was read,
// and whether every node line parsed.
typedef struct {
	voe_editor_dock_tree *tree;
	voe_editor_dock_float *floats;
	bool rooted;
	bool parsed;
} layout_reading;

// Which of LAYOUT_KEYS a line starts with as a whole word, or
// LAYOUT_KEY_COUNT for none.
static size_t layout_key(const char *line)
{
	size_t length;

	VOE_BASE_ASSERT(line != NULL, "the key of no line");
	length = strcspn(line, " \t\r");
	for (size_t key = 0; key < LAYOUT_KEY_COUNT; key++)
		if (strlen(LAYOUT_KEYS[key]) == length &&
		    strncmp(line, LAYOUT_KEYS[key], length) == 0)
			return key;
	return LAYOUT_KEY_COUNT;
}

// Whether a line is the layout's, which the layout write replaces.
static bool layout_line(const char *line)
{
	return layout_key(line) < LAYOUT_KEY_COUNT;
}

// The next blank-separated word, the cursor moved past it.
static line_word take_word(line_cursor *c)
{
	line_word word;

	VOE_BASE_ASSERT(c != NULL && c->at != NULL, "taking a word from nothing");
	c->at += strspn(c->at, " \t");
	word = (line_word){ .start = c->at, .length = strcspn(c->at, " \t\r") };
	c->at += word.length;
	VOE_BASE_ASSERT(word.start + word.length == c->at, "a word not passed");
	return word;
}

static bool word_is(line_word word, const char *text)
{
	VOE_BASE_ASSERT(word.start != NULL && text != NULL, "comparing no word");
	return word.length == strlen(text) &&
	       strncmp(word.start, text, word.length) == 0;
}

// The next word as a panel, VOE_EDITOR_PANEL_COUNT and not ok for none.
static voe_editor_panel take_panel(line_cursor *c)
{
	const line_word word = take_word(c);

	VOE_BASE_ASSERT(c != NULL, "taking a panel from nothing");
	for (uint32_t p = 0; p < VOE_EDITOR_PANEL_COUNT; p++)
		if (word_is(word, PANEL_WORDS[p]))
			return (voe_editor_panel)p;
	c->ok = false;
	return VOE_EDITOR_PANEL_COUNT;
}

// The next word an index of digits up to UINT32_MAX, nought and not ok
// otherwise.
static uint32_t take_index(line_cursor *c)
{
	char *end;
	unsigned long value;

	VOE_BASE_ASSERT(c != NULL && c->at != NULL, "taking an index from nothing");
	c->at += strspn(c->at, " \t");
	if (!isdigit((unsigned char)*c->at)) {
		c->ok = false;
		return 0;
	}
	value = strtoul(c->at, &end, 10);
	c->at = end;
	if (value > UINT32_MAX || strchr(" \t\r", *end) == NULL) {
		c->ok = false;
		return 0;
	}
	return (uint32_t)value;
}

// The next word a number finite as a double, nought and not ok otherwise.
static double take_number(line_cursor *c)
{
	char *end;
	double value;

	VOE_BASE_ASSERT(c != NULL && c->at != NULL, "taking a number from nothing");
	value = strtod(c->at, &end);
	if (end == c->at || strchr(" \t\r", *end) == NULL || !isfinite(value)) {
		c->ok = false;
		return 0.0;
	}
	c->at = end;
	return value;
}

// The next word a number finite as a float, nought and not ok otherwise.
static float take_millimetres(line_cursor *c)
{
	const float value = (float)take_number(c);

	VOE_BASE_ASSERT(c != NULL, "taking millimetres from nothing");
	if (isfinite(value))
		return value;
	c->ok = false;
	return 0.0f;
}

// Whether every word parsed and only blanks are left.
static bool line_ended(line_cursor *c)
{
	VOE_BASE_ASSERT(c != NULL && c->at != NULL, "ending no line");
	c->at += strspn(c->at, " \t\r");
	return c->ok && *c->at == '\0';
}

// A split's words after `split`: axis, hold, length, fraction, first, second.
static void split_take(line_cursor *c, voe_editor_dock_node *node)
{
	const line_word axis = take_word(c);
	const line_word hold = take_word(c);
	uint32_t a = 0;
	uint32_t h = 0;

	VOE_BASE_ASSERT(node != NULL, "taking a split into nothing");
	while (a < AXIS_COUNT && !word_is(axis, AXIS_WORDS[a]))
		a++;
	while (h < HOLD_COUNT && !word_is(hold, HOLD_WORDS[h]))
		h++;
	if (a == AXIS_COUNT || h == HOLD_COUNT)
		c->ok = false;
	node->kind = VOE_EDITOR_DOCK_SPLIT;
	node->axis = (voe_editor_dock_axis)(a % AXIS_COUNT);
	node->hold = (voe_editor_dock_hold)(h % HOLD_COUNT);
	node->length = take_millimetres(c);
	node->fraction = take_number(c);
	node->first = take_index(c);
	node->second = take_index(c);
	VOE_BASE_ASSERT(node->axis < AXIS_COUNT, "a split's axis read past its words");
}

// A `dock_node` line's words into its node, the count grown to hold it. False,
// and nothing written, when it does not parse or its index is past the room.
static bool node_take(voe_editor_dock_tree *tree, line_cursor *c)
{
	voe_editor_dock_node node = { 0 };
	const uint32_t index = take_index(c);
	const line_word kind = take_word(c);

	VOE_BASE_ASSERT(tree != NULL, "taking a node into no tree");
	if (word_is(kind, "leaf")) {
		node.kind = VOE_EDITOR_DOCK_LEAF;
		node.panel = take_panel(c);
		if (node.panel == VOE_EDITOR_PANEL_SCENE_VIEW)
			node.view = take_index(c);
	} else if (word_is(kind, "split")) {
		split_take(c, &node);
	} else {
		c->ok = false;
	}
	if (!line_ended(c) || index >= VOE_EDITOR_DOCK_NODES)
		return false;
	tree->nodes[index] = node;
	if (index >= tree->count)
		tree->count = index + 1;
	VOE_BASE_ASSERT(tree->count <= VOE_EDITOR_DOCK_NODES, "a tree read past its room");
	return true;
}

// A `dock_float` line's words over its moving panel's entry; nothing when it
// does not parse or names no moving panel.
static void float_take(voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT],
		       line_cursor *c)
{
	const voe_editor_panel panel = take_panel(c);
	const uint32_t floating = take_index(c);
	voe_ui_rect box;

	VOE_BASE_ASSERT(floats != NULL, "taking a float into nothing");
	box.min.x = take_millimetres(c);
	box.min.y = take_millimetres(c);
	box.size.x = take_millimetres(c);
	box.size.y = take_millimetres(c);
	if (!line_ended(c) || floating > 1)
		return;
	for (uint32_t w = 0; w < VOE_EDITOR_CLOSABLE_COUNT; w++)
		if (voe_editor_closable_panel((voe_editor_closable)w) == panel) {
			floats[w].floating = floating == 1;
			floats[w].box = box;
		}
	VOE_BASE_ASSERT(!floats[VOE_EDITOR_CLOSABLE_BOTTOM_VIEW].floating,
			"a float line floated the bottom view");
}

// One layout line into the layout_reading at into; other keys' are not its.
static void layout_take(void *into, const char *line)
{
	layout_reading *reading = into;
	const size_t key = layout_key(line);
	line_cursor c;
	uint32_t root;

	VOE_BASE_ASSERT(reading != NULL && reading->tree != NULL &&
				reading->floats != NULL,
			"taking a layout line into nothing");
	if (key == LAYOUT_KEY_COUNT)
		return;
	c = (line_cursor){ .at = line + strlen(LAYOUT_KEYS[key]), .ok = true };
	if (key == KEY_ROOT) {
		root = take_index(&c);
		if (line_ended(&c)) {
			reading->tree->root = root;
			reading->rooted = true;
		}
	} else if (key == KEY_NODE) {
		if (!node_take(reading->tree, &c))
			reading->parsed = false;
	} else {
		float_take(reading->floats, &c);
	}
}

bool voe_editor_settings_layout_read(
	voe_editor_dock_tree *tree,
	voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT])
{
	layout_reading reading = {
		.tree = tree, .floats = floats, .rooted = false, .parsed = true
	};

	VOE_BASE_ASSERT(tree != NULL, "reading a layout into no tree");
	VOE_BASE_ASSERT(floats != NULL, "reading a layout into no floats");

	*tree = (voe_editor_dock_tree){ 0 };
	voe_editor_settings_lines_read(layout_take, &reading);
	return reading.rooted && reading.parsed;
}

// One node's line at `out`, of `room`; its length.
static size_t node_line(char *out, size_t room, uint32_t index,
			const voe_editor_dock_node *node)
{
	int length;

	VOE_BASE_ASSERT(out != NULL && node != NULL, "writing no node's line");
	if (node->kind == VOE_EDITOR_DOCK_SPLIT)
		length = snprintf(out, room, "dock_node %u split %s %s %.3f %.6f %u %u\n",
				  (unsigned)index, AXIS_WORDS[node->axis],
				  HOLD_WORDS[node->hold], (double)node->length,
				  node->fraction, (unsigned)node->first,
				  (unsigned)node->second);
	else if (node->panel == VOE_EDITOR_PANEL_SCENE_VIEW)
		length = snprintf(out, room, "dock_node %u leaf view %u\n",
				  (unsigned)index, (unsigned)node->view);
	else
		length = snprintf(out, room, "dock_node %u leaf %s\n",
				  (unsigned)index, PANEL_WORDS[node->panel]);
	VOE_BASE_ASSERT(length > 0 && (size_t)length < room,
			"a layout line longer than its room");
	return (size_t)length;
}

// One moving panel's float line at `out`, of `room`; its length.
static size_t float_line(char *out, size_t room, voe_editor_panel panel,
			 const voe_editor_dock_float *f)
{
	const int length = snprintf(out, room,
				    "dock_float %s %d %.3f %.3f %.3f %.3f\n",
				    PANEL_WORDS[panel], f->floating ? 1 : 0,
				    (double)f->box.min.x, (double)f->box.min.y,
				    (double)f->box.size.x, (double)f->box.size.y);

	VOE_BASE_ASSERT(panel < VOE_EDITOR_PANEL_COUNT, "a float line of no panel");
	VOE_BASE_ASSERT(length > 0 && (size_t)length < room,
			"a float line longer than its room");
	return (size_t)length;
}

bool voe_editor_settings_layout_write(
	const voe_editor_dock_tree *tree,
	const voe_editor_dock_float floats[VOE_EDITOR_CLOSABLE_COUNT])
{
	char own[LAYOUT_LINES * LAYOUT_LINE_ROOM];
	size_t used;
	bool ok;

	VOE_BASE_ASSERT(tree != NULL && floats != NULL, "writing no layout");
	VOE_BASE_ASSERT(tree->count > 0 && tree->count <= VOE_EDITOR_DOCK_NODES,
			"writing a tree of no nodes or past its room");

	used = (size_t)snprintf(own, sizeof own, "dock_root %u\n",
				(unsigned)tree->root);
	for (uint32_t i = 0; i < tree->count; i++)
		used += node_line(own + used, sizeof own - used, i,
				  &tree->nodes[i]);
	for (uint32_t w = 0; w < VOE_EDITOR_CLOSABLE_COUNT; w++) {
		const voe_editor_panel panel =
			voe_editor_closable_panel((voe_editor_closable)w);

		if (panel != VOE_EDITOR_PANEL_COUNT)
			used += float_line(own + used, sizeof own - used, panel,
					   &floats[w]);
	}
	ok = voe_editor_settings_lines_write(layout_line, own, used);

	if (!ok)
		VOE_BASE_ERROR("editor",
			       "the dock layout could not be written to "
			       "<settings>/voe3d/editor_settings");
	return ok;
}
