// The material section's one frame of `ui` calls and the read of its buttons,
// swatches, sliders, number boxes and ×s afterwards; a swatch opens the picker
// at its field's offsetof in voe_assets_material_file. See the header for why the nodes live
// in the scene and what the read may change.
//
// The shader buttons are indexed as voe_assets_material_shader, so a button's
// index is its shader, a sides button's index is `double_sided`; the sliders
// roughness, metal, cutout; the map rows as the file lists them: colour,
// normal, ORM, emission.
//
// While ORMmap is set, Roughness and Metal are drawn under `controls->dim` with
// voe_ui_slider_shown and their nodes left VOE_UI_NODE_NONE, so the read takes
// nothing from them (0401, 0402).
#include "inspector_material.h"

#include "scene.h"
#include "themes.h"

#include <base/assert.h>

#include <math/float3.h>

#include <ui/colour.h>
#include <ui/slider.h>
#include <ui/widgets.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// As inspector_sculpt.c's section: inside its edges, between its rows, and
// between the things on one row. Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// A slider's track, and the swatch as inspector.c's. Millimetres.
#define SLIDER_WIDE 40.0f
#define SWATCH_WIDE 8.0f
#define SWATCH_HIGH 3.0f

// Repeat's range (0399 point 8), and what a millimetre of drag is worth.
#define REPEAT_MIN 0.01
#define REPEAT_MAX 1000.0
#define REPEAT_PER_MM 0.01

// The emission's Strength range (0420 point 2), and a millimetre of drag.
#define STRENGTH_MIN 0.0
#define STRENGTH_MAX 1000.0
#define STRENGTH_PER_MM 0.05

static const char *const SHADER_NAMES[VOE_EDITOR_MATERIAL_SHADERS] = {
	"Lit", "Unlit"
};

static const char *const SLIDER_NAMES[VOE_EDITOR_MATERIAL_SLIDERS] = {
	"Roughness", "Metal", "Cutout"
};

static const char *const SIDES_NAMES[VOE_EDITOR_MATERIAL_SIDES] = {
	"One side", "Both sides"
};

static const char *const MAP_NAMES[VOE_EDITOR_MATERIAL_MAPS] = {
	"Colormap", "Normalmap", "ORMmap", "Emissionmap"
};

// The number slider `i` writes: roughness, metal, cutout.
static float *slider_value(voe_assets_material_file *material, uint32_t i)
{
	VOE_BASE_ASSERT(material != NULL, "a slider of no material");
	VOE_BASE_ASSERT(i < VOE_EDITOR_MATERIAL_SLIDERS, "no such slider");
	return i == 0 ? &material->roughness :
	       i == 1 ? &material->metal :
			&material->cutout;
}

// The path map row `i` holds: colour, normal, ORM, emission.
static const char *map_path(const voe_assets_material_file *material,
			    uint32_t i)
{
	VOE_BASE_ASSERT(material != NULL, "a map of no material");
	VOE_BASE_ASSERT(i < VOE_EDITOR_MATERIAL_MAPS, "no such map");
	return i == 0 ? material->colormap :
	       i == 1 ? material->normalmap :
	       i == 2 ? material->ormmap :
			material->emissionmap;
}

// What follows the last `/` of `path`, `path` itself without one.
static const char *file_name(const char *path)
{
	const char *slash;

	VOE_BASE_ASSERT(path != NULL, "the name of no path");
	slash = strrchr(path, '/');
	return slash != NULL ? slash + 1 : path;
}

void voe_editor_inspector_material_dim(voe_editor_inspector_material *controls,
				       const voe_ui_theme *palette)
{
	VOE_BASE_ASSERT(controls != NULL && palette != NULL,
			"dimming no material section or from no palette");

	controls->dim = *palette;
	controls->dim.text_primary = palette->text_disabled;
	controls->dim.text_secondary = palette->text_disabled;
	controls->dim.inverse_ink = palette->text_disabled;
	controls->dim.control_hovered = palette->control;
	controls->dim.inverse = palette->control;
}

void voe_editor_inspector_material_forget(
	voe_editor_inspector_material *controls)
{
	VOE_BASE_ASSERT(controls != NULL, "forgetting no material section");

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SHADERS; i++)
		controls->shaders[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SLIDERS; i++)
		controls->sliders[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SIDES; i++)
		controls->sides[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++) {
		controls->maps[i] = VOE_UI_NODE_NONE;
		controls->clears[i] = VOE_UI_NODE_NONE;
	}
	controls->section = VOE_UI_NODE_NONE;
	controls->colour = VOE_UI_NODE_NONE;
	controls->emission = VOE_UI_NODE_NONE;
	controls->strength = VOE_UI_NODE_NONE;
	controls->repeat = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(controls->shaders[0] == VOE_UI_NODE_NONE,
			"a material section left remembered");
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

// A row of `name` and a swatch of `colour` inside a button keyed `key`, which
// it returns.
static voe_ui_node swatch_draw(voe_ui_context *ui, const char *name,
			       const char *key, const float colour[3])
{
	const voe_ui_sizing swatch = {
		.along = { VOE_UI_SIZE_FIXED, SWATCH_WIDE },
		.across = { VOE_UI_SIZE_FIXED, SWATCH_HIGH }
	};
	voe_ui_node button;

	VOE_BASE_ASSERT(ui != NULL && name != NULL && key != NULL,
			"drawing a swatch nowhere or unnamed");
	VOE_BASE_ASSERT(colour != NULL, "drawing a swatch of no colour");
	row_open(ui);
	voe_ui_label(ui, name);
	button = voe_ui_button_begin(ui, key, 0);
	voe_ui_swatch(ui, (voe_math_float3){ colour[0], colour[1], colour[2] },
		      swatch);
	voe_ui_end(ui);
	voe_ui_end(ui);
	return button;
}

// A row of `name`, a number box keyed `key` and its figure written into
// `figure`; returns the box.
static voe_ui_node number_draw(voe_ui_context *ui, const char *name,
			       const char *key, float value, double per_mm,
			       char figure[VOE_EDITOR_MATERIAL_FIGURE])
{
	voe_ui_node box;

	VOE_BASE_ASSERT(ui != NULL && name != NULL && key != NULL,
			"drawing a number box nowhere or unnamed");
	VOE_BASE_ASSERT(figure != NULL, "a number box with no figure");
	row_open(ui);
	voe_ui_label(ui, name);
	box = voe_ui_number_begin(ui, key, 0, (double)value, per_mm);
	snprintf(figure, VOE_EDITOR_MATERIAL_FIGURE, "%.2f", (double)value);
	voe_ui_label(ui, figure);
	voe_ui_end(ui);
	voe_ui_end(ui);
	return box;
}

// Slider `i`'s row: name, track and figure; greyed, under `controls->dim` and
// shown only, so its node stays VOE_UI_NODE_NONE.
static void slider_draw(voe_ui_context *ui,
			voe_editor_inspector_material *controls, uint32_t i,
			float value, bool greyed)
{
	VOE_BASE_ASSERT(ui != NULL && controls != NULL, "drawing no slider");
	VOE_BASE_ASSERT(i < VOE_EDITOR_MATERIAL_SLIDERS, "no such slider");

	if (greyed)
		voe_ui_theme_push(ui, &controls->dim);
	row_open(ui);
	voe_ui_label(ui, SLIDER_NAMES[i]);
	if (greyed)
		voe_ui_slider_shown(ui, "material slider", i, value, 0.0f, 1.0f,
				    SLIDER_WIDE);
	else
		controls->sliders[i] = voe_ui_slider(ui, "material slider", i,
						     value, 0.0f, 1.0f,
						     SLIDER_WIDE);
	snprintf(controls->figures[i], sizeof controls->figures[i], "%.2f",
		 (double)value);
	voe_ui_label(ui, controls->figures[i]);
	voe_ui_end(ui);
	if (greyed)
		voe_ui_theme_pop(ui);
}

// The map rows: name, file name or "None", and × while set.
static void maps_draw(voe_ui_context *ui,
		      voe_editor_inspector_material *controls,
		      const voe_assets_material_file *material)
{
	VOE_BASE_ASSERT(ui != NULL && controls != NULL, "drawing no map rows");
	VOE_BASE_ASSERT(material != NULL, "drawing the maps of no material");

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++) {
		const char *path = map_path(material, i);

		controls->maps[i] = row_open(ui);
		voe_ui_label(ui, MAP_NAMES[i]);
		voe_ui_label(ui, path[0] != '\0' ? file_name(path) : "None");
		if (path[0] != '\0') {
			controls->clears[i] = voe_ui_button_begin(ui, "clear map",
								  i);
			voe_ui_label(ui, "×");
			voe_ui_end(ui);
		}
		voe_ui_end(ui);
	}
}

void voe_editor_inspector_material_draw(
	voe_ui_context *ui, voe_editor_inspector_material *controls,
	const char *path, const voe_assets_material_file *material)
{
	VOE_BASE_ASSERT(ui != NULL && controls != NULL,
			"drawing no material section or into no interface");
	VOE_BASE_ASSERT(path != NULL && material != NULL,
			"drawing a material section of no material");

	controls->section = voe_ui_panel_begin(ui, "material", 0,
					       VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, file_name(path));

	row_open(ui);
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SHADERS; i++) {
		controls->shaders[i] = voe_ui_choice_begin(
			ui, "shader", i, (uint32_t)material->shader == i);
		voe_ui_label(ui, SHADER_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);

	controls->colour = swatch_draw(ui, "Colour", "colour", material->colour);
	controls->emission = swatch_draw(ui, "Emission", "emission",
					 material->emission);
	controls->strength = number_draw(ui, "Strength", "strength",
					 material->emission_strength,
					 STRENGTH_PER_MM,
					 controls->strength_figure);

	const bool greyed = material->ormmap[0] != '\0';

	slider_draw(ui, controls, 0, material->roughness, greyed);
	slider_draw(ui, controls, 1, material->metal, greyed);

	controls->repeat = number_draw(ui, "Repeat", "repeat", material->repeat,
				       REPEAT_PER_MM, controls->repeat_figure);

	slider_draw(ui, controls, 2, material->cutout, false);

	row_open(ui);
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SIDES; i++) {
		controls->sides[i] = voe_ui_choice_begin(
			ui, "sides", i, material->double_sided == (i == 1));
		voe_ui_label(ui, SIDES_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);

	maps_draw(ui, controls, material);

	voe_ui_end(ui);
}

// A changed number box's value into `*value`, clamped to `low`..`high`; a box
// not drawn takes nothing.
static void number_read(const voe_ui_context *ui, voe_ui_node box, double low,
			double high, float *value)
{
	voe_ui_number_result result;

	VOE_BASE_ASSERT(ui != NULL && value != NULL,
			"reading a number box of no interface or into nothing");
	VOE_BASE_ASSERT(low <= high, "a number box's range upside down");
	if (box == VOE_UI_NODE_NONE)
		return;
	result = voe_ui_number_action(ui, box);
	if (result.changed)
		*value = (float)(result.value < low  ? low :
				 result.value > high ? high :
						       result.value);
}

// A fired swatch opens the picker on the material's three floats at `offset`,
// beside the section's left edge, as the Inspector's opens it beside its
// content column's.
static void swatch_read(const voe_ui_context *ui,
			struct voe_editor_scene *scene, voe_ui_node swatch,
			size_t offset)
{
	const voe_editor_inspector_material *controls;

	VOE_BASE_ASSERT(ui != NULL && scene != NULL,
			"reading a swatch of no interface or no scene");
	VOE_BASE_ASSERT(offset + 3 * sizeof(float) <= sizeof scene->material,
			"a swatch past the material file's end");
	controls = &scene->material_controls;
	if (swatch == VOE_UI_NODE_NONE ||
	    !voe_ui_button_action(ui, swatch).fired)
		return;
	voe_editor_scene_picker_open(
		scene, (voe_editor_picking){
			       .material = true,
			       .offset = offset,
			       .left = controls->section != VOE_UI_NODE_NONE ?
					       voe_ui_node_rect(ui, controls->section)
						       .min.x :
					       0.0f });
}

void voe_editor_inspector_material_read(const voe_ui_context *ui,
					struct voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(ui != NULL && scene != NULL,
			"reading no material section or of no interface");

	const voe_editor_inspector_material *controls =
		&scene->material_controls;
	voe_assets_material_file *material = &scene->material;

	// Indexed as map_path's.
	char *const maps[VOE_EDITOR_MATERIAL_MAPS] = {
		material->colormap, material->normalmap,
		material->ormmap, material->emissionmap
	};

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SHADERS; i++)
		if (controls->shaders[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, controls->shaders[i]).fired)
			material->shader = (voe_assets_material_shader)i;

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SIDES; i++)
		if (controls->sides[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, controls->sides[i]).fired)
			material->double_sided = i == 1;

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SLIDERS; i++)
		if (controls->sliders[i] != VOE_UI_NODE_NONE)
			*slider_value(material, i) = (float)voe_ui_slider_action(
							     ui, controls->sliders[i],
							     0.0f, 1.0f)
							     .value;

	number_read(ui, controls->repeat, REPEAT_MIN, REPEAT_MAX,
		    &material->repeat);
	number_read(ui, controls->strength, STRENGTH_MIN, STRENGTH_MAX,
		    &material->emission_strength);

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++)
		scene->material_controls.maps_seen[i] =
			controls->maps[i] == VOE_UI_NODE_NONE ?
				(voe_ui_rect){ 0 } :
				voe_ui_node_visible(ui, controls->maps[i]);

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++)
		if (controls->clears[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, controls->clears[i]).fired)
			maps[i][0] = '\0';

	swatch_read(ui, scene, controls->colour,
		    offsetof(voe_assets_material_file, colour));
	swatch_read(ui, scene, controls->emission,
		    offsetof(voe_assets_material_file, emission));
}
