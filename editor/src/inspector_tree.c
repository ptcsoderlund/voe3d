// The tree section's one frame of `ui` calls and the read of its ×s, sliders,
// number box and stepper afterwards. See the header for why the nodes live in
// the scene and what the read may change.
//
// The material rows are indexed bark, leaf; the sliders as SLIDERS lists them,
// which is assets/tree.h's order of the number keys, each slider's ends that
// key's _MIN and _MAX, so the file's ranges and the section's cannot drift.
#include "inspector_tree.h"

#include "scene.h"
#include "themes.h"

#include <base/assert.h>

#include <ui/slider.h>
#include <ui/widgets.h>

#include <stdio.h>
#include <string.h>

// As inspector_material.c's section: inside its edges, between its rows, and
// between the things on one row. Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// A slider's track (0498 point 3). Millimetres.
#define SLIDER_WIDE 40.0f

// What a millimetre of drag is worth on Variations: five to a variation.
#define VARIATIONS_PER_MM 0.2

static const char *const MATERIAL_NAMES[VOE_EDITOR_TREE_MATERIALS] = {
	"Bark", "Leaf"
};

// A slider's title, ends and unit, "" for none (0498 point 2).
typedef struct {
	const char *name;
	float low;
	float high;
	const char *unit;
} slider_kind;

static const slider_kind SLIDERS[VOE_EDITOR_TREE_SLIDERS] = {
	{ "Height", VOE_ASSETS_TREE_HEIGHT_MIN, VOE_ASSETS_TREE_HEIGHT_MAX, "m" },
	{ "Trunk", VOE_ASSETS_TREE_TRUNK_MIN, VOE_ASSETS_TREE_TRUNK_MAX, "m" },
	{ "Crown", VOE_ASSETS_TREE_CROWN_MIN, VOE_ASSETS_TREE_CROWN_MAX, "" },
	{ "Branching", VOE_ASSETS_TREE_BRANCHING_MIN,
	  VOE_ASSETS_TREE_BRANCHING_MAX, "" },
	{ "Reach", VOE_ASSETS_TREE_REACH_MIN, VOE_ASSETS_TREE_REACH_MAX, "" },
	{ "Angle", VOE_ASSETS_TREE_ANGLE_MIN, VOE_ASSETS_TREE_ANGLE_MAX, "°" },
	{ "Droop", VOE_ASSETS_TREE_DROOP_MIN, VOE_ASSETS_TREE_DROOP_MAX, "" },
	{ "Crooked", VOE_ASSETS_TREE_CROOKED_MIN, VOE_ASSETS_TREE_CROOKED_MAX,
	  "" },
	{ "Leaf size", VOE_ASSETS_TREE_LEAF_SIZE_MIN,
	  VOE_ASSETS_TREE_LEAF_SIZE_MAX, "m" },
	{ "Leaf density", VOE_ASSETS_TREE_LEAF_DENSITY_MIN,
	  VOE_ASSETS_TREE_LEAF_DENSITY_MAX, "" },
	{ "Leaf roll", VOE_ASSETS_TREE_LEAF_ROLL_MIN,
	  VOE_ASSETS_TREE_LEAF_ROLL_MAX, "" },
	{ "Leaf width", VOE_ASSETS_TREE_LEAF_WIDTH_MIN,
	  VOE_ASSETS_TREE_LEAF_WIDTH_MAX, "" },
};

// The number slider `i` writes, in SLIDERS' order.
static float *slider_value(voe_assets_tree *tree, uint32_t i)
{
	float *const values[VOE_EDITOR_TREE_SLIDERS] = {
		&tree->height,	  &tree->trunk,	  &tree->crown,
		&tree->branching, &tree->reach,	  &tree->angle,
		&tree->droop,	  &tree->crooked, &tree->leaf_size,
		&tree->leaf_density, &tree->leaf_roll, &tree->leaf_width
	};

	VOE_BASE_ASSERT(tree != NULL, "a slider of no tree");
	VOE_BASE_ASSERT(i < VOE_EDITOR_TREE_SLIDERS, "no such slider");
	return values[i];
}

// The path material row `i` holds: bark, leaf.
static char *material_path(voe_assets_tree *tree, uint32_t i)
{
	VOE_BASE_ASSERT(tree != NULL, "a material of no tree");
	VOE_BASE_ASSERT(i < VOE_EDITOR_TREE_MATERIALS, "no such material");
	return i == 0 ? tree->bark : tree->leaf;
}

// What follows the last `/` of `path`, `path` itself without one.
static const char *file_name(const char *path)
{
	const char *slash;

	VOE_BASE_ASSERT(path != NULL, "the name of no path");
	slash = strrchr(path, '/');
	return slash != NULL ? slash + 1 : path;
}

void voe_editor_inspector_tree_forget(voe_editor_inspector_tree *controls)
{
	VOE_BASE_ASSERT(controls != NULL, "forgetting no tree section");

	for (uint32_t i = 0; i < VOE_EDITOR_TREE_MATERIALS; i++) {
		controls->materials[i] = VOE_UI_NODE_NONE;
		controls->clears[i] = VOE_UI_NODE_NONE;
	}
	for (uint32_t i = 0; i < VOE_EDITOR_TREE_SLIDERS; i++)
		controls->sliders[i] = VOE_UI_NODE_NONE;
	controls->variations = VOE_UI_NODE_NONE;
	controls->previous = VOE_UI_NODE_NONE;
	controls->next = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(controls->sliders[0] == VOE_UI_NODE_NONE,
			"a tree section left remembered");
}

// A row centred across, wrapping, as every Inspector row is.
static voe_ui_node row_open(voe_ui_context *ui)
{
	VOE_BASE_ASSERT(ui != NULL, "a row in no interface");
	return voe_ui_row_begin(ui, (voe_ui_container){
					    .across = VOE_UI_ACROSS_CENTER,
					    .gap = ROW_GAP,
					    .wrap = true });
}

// A button keyed `key` holding `text`, which it returns.
static voe_ui_node button_draw(voe_ui_context *ui, const char *key,
			       uint32_t index, const char *text)
{
	voe_ui_node button;

	VOE_BASE_ASSERT(ui != NULL && key != NULL, "a button nowhere or unkeyed");
	VOE_BASE_ASSERT(text != NULL, "a button saying nothing");
	button = voe_ui_button_begin(ui, key, index);
	voe_ui_label(ui, text);
	voe_ui_end(ui);
	return button;
}

// The Bark and Leaf rows: name, path or "None", and × while set.
static void materials_draw(voe_ui_context *ui,
			   voe_editor_inspector_tree *controls,
			   const voe_assets_tree *tree)
{
	VOE_BASE_ASSERT(ui != NULL && controls != NULL,
			"drawing no material rows");
	VOE_BASE_ASSERT(tree != NULL, "drawing the materials of no tree");

	for (uint32_t i = 0; i < VOE_EDITOR_TREE_MATERIALS; i++) {
		const char *path = i == 0 ? tree->bark : tree->leaf;

		controls->materials[i] = row_open(ui);
		voe_ui_label(ui, MATERIAL_NAMES[i]);
		voe_ui_label(ui, path[0] != '\0' ? path : "None");
		if (path[0] != '\0')
			controls->clears[i] = button_draw(ui, "clear tree material",
							  i, "×");
		voe_ui_end(ui);
	}
}

// Slider `i`'s row: title, track and figure with its unit.
static void slider_draw(voe_ui_context *ui, voe_editor_inspector_tree *controls,
			uint32_t i, float value)
{
	const slider_kind *kind = &SLIDERS[i < VOE_EDITOR_TREE_SLIDERS ? i : 0];

	VOE_BASE_ASSERT(ui != NULL && controls != NULL, "drawing no slider");
	VOE_BASE_ASSERT(i < VOE_EDITOR_TREE_SLIDERS, "no such slider");

	row_open(ui);
	voe_ui_label(ui, kind->name);
	controls->sliders[i] = voe_ui_slider(ui, "tree slider", i, value,
					     kind->low, kind->high, SLIDER_WIDE);
	snprintf(controls->figures[i], sizeof controls->figures[i], "%.2f%s%s",
		 (double)value, kind->unit[0] != '\0' ? " " : "", kind->unit);
	voe_ui_label(ui, controls->figures[i]);
	voe_ui_end(ui);
}

// Variations' number box, and the stepper: ◀, "v / count", ▶; a variation
// past the count, not yet clamped by the read, shows as the last.
static void variations_draw(voe_ui_context *ui,
			    voe_editor_inspector_tree *controls,
			    uint32_t variations, uint32_t variation)
{
	VOE_BASE_ASSERT(ui != NULL && controls != NULL,
			"drawing no variations");
	VOE_BASE_ASSERT(variations >= VOE_ASSETS_TREE_VARIATIONS_MIN,
			"showing a species of no variations");

	row_open(ui);
	voe_ui_label(ui, "Variations");
	controls->variations = voe_ui_number_begin(ui, "tree variations", 0,
						   (double)variations,
						   VARIATIONS_PER_MM);
	snprintf(controls->variations_figure,
		 sizeof controls->variations_figure, "%u", variations);
	voe_ui_label(ui, controls->variations_figure);
	voe_ui_end(ui);
	voe_ui_end(ui);

	row_open(ui);
	voe_ui_label(ui, "Variation");
	controls->previous = button_draw(ui, "tree variation", 0, "◀");
	snprintf(controls->variation_figure, sizeof controls->variation_figure,
		 "%u / %u", (variation < variations ? variation : variations - 1) + 1,
		 variations);
	voe_ui_label(ui, controls->variation_figure);
	controls->next = button_draw(ui, "tree variation", 1, "▶");
	voe_ui_end(ui);
}

void voe_editor_inspector_tree_draw(voe_ui_context *ui,
				    voe_editor_inspector_tree *controls,
				    const char *path, const voe_assets_tree *tree,
				    uint32_t variation)
{
	VOE_BASE_ASSERT(ui != NULL && controls != NULL,
			"drawing no tree section or into no interface");
	VOE_BASE_ASSERT(path != NULL && tree != NULL,
			"drawing a tree section of no species");
	// Copied, so the sliders read through slider_value as the read writes.
	voe_assets_tree shown = *tree;

	voe_ui_panel_begin(ui, "tree", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, file_name(path));
	materials_draw(ui, controls, tree);
	for (uint32_t i = 0; i < VOE_EDITOR_TREE_SLIDERS; i++)
		slider_draw(ui, controls, i,
			    *slider_value(&shown, i));
	variations_draw(ui, controls, tree->variations, variation);
	voe_ui_end(ui);
}

// A changed Variations into `*variations`, rounded and clamped; a box not
// drawn takes nothing.
static void variations_read(const voe_ui_context *ui, voe_ui_node box,
			    uint32_t *variations)
{
	const double low = VOE_ASSETS_TREE_VARIATIONS_MIN;
	const double high = VOE_ASSETS_TREE_VARIATIONS_MAX;
	voe_ui_number_result result;

	VOE_BASE_ASSERT(ui != NULL && variations != NULL,
			"reading Variations of no interface or into nothing");
	if (box == VOE_UI_NODE_NONE)
		return;
	result = voe_ui_number_action(ui, box);
	if (result.changed)
		*variations = (uint32_t)((result.value < low  ? low :
					  result.value > high ? high :
								result.value) +
					 0.5);
	VOE_BASE_ASSERT(*variations >= VOE_ASSETS_TREE_VARIATIONS_MIN &&
				*variations <= VOE_ASSETS_TREE_VARIATIONS_MAX,
			"Variations read past its range");
}

// Whether `button` was drawn and fired.
static bool fired(const voe_ui_context *ui, voe_ui_node button)
{
	VOE_BASE_ASSERT(ui != NULL, "asking a button of no interface");
	return button != VOE_UI_NODE_NONE &&
	       voe_ui_button_action(ui, button).fired;
}

void voe_editor_inspector_tree_read(const voe_ui_context *ui,
				    struct voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(ui != NULL && scene != NULL,
			"reading no tree section or of no interface");

	const voe_editor_inspector_tree *controls = &scene->tree_controls;
	voe_assets_tree *tree = &scene->tree;
	uint32_t count;

	for (uint32_t i = 0; i < VOE_EDITOR_TREE_SLIDERS; i++)
		if (controls->sliders[i] != VOE_UI_NODE_NONE)
			*slider_value(tree, i) = (float)voe_ui_slider_action(
							 ui, controls->sliders[i],
							 SLIDERS[i].low,
							 SLIDERS[i].high)
							 .value;

	for (uint32_t i = 0; i < VOE_EDITOR_TREE_MATERIALS; i++)
		scene->tree_controls.materials_seen[i] =
			controls->materials[i] == VOE_UI_NODE_NONE ?
				(voe_ui_rect){ 0 } :
				voe_ui_node_visible(ui, controls->materials[i]);

	// No species open: the section was not drawn, and the shown copy is
	// still the zeroed one with no variations to step through.
	if (scene->tree_open[0] == '\0')
		return;

	for (uint32_t i = 0; i < VOE_EDITOR_TREE_MATERIALS; i++)
		if (fired(ui, controls->clears[i]))
			material_path(tree, i)[0] = '\0';

	variations_read(ui, controls->variations, &tree->variations);

	count = tree->variations;
	if (fired(ui, controls->previous))
		scene->tree_variation = scene->tree_variation == 0 ||
						scene->tree_variation >= count ?
						count - 1 :
						scene->tree_variation - 1;
	if (fired(ui, controls->next))
		scene->tree_variation = scene->tree_variation + 1 >= count ?
						0 :
						scene->tree_variation + 1;
	VOE_BASE_ASSERT(count >= VOE_ASSETS_TREE_VARIATIONS_MIN,
			"a tree read to no variations");
}
