// The material section's one frame of `ui` calls and the read of its buttons,
// sliders, number box and ×s afterwards. See the header for why the nodes live
// in the scene and what the read may change.
//
// The shader buttons are indexed as voe_assets_material_shader, so a button's
// index is its shader; the map rows as the file lists them: colour, normal,
// roughness.
#include "inspector_material.h"

#include "scene.h"
#include "themes.h"

#include <base/assert.h>

#include <math/float3.h>

#include <ui/colour.h>
#include <ui/slider.h>
#include <ui/widgets.h>

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

static const char *const SHADER_NAMES[VOE_EDITOR_MATERIAL_SHADERS] = {
	"Lit", "Unlit"
};

static const char *const SLIDER_NAMES[VOE_EDITOR_MATERIAL_SLIDERS] = {
	"Roughness", "Metal"
};

static const char *const MAP_NAMES[VOE_EDITOR_MATERIAL_MAPS] = {
	"Colormap", "Normalmap", "ORMmap"
};

// The number slider `i` writes: roughness, metal.
static float *slider_value(voe_assets_material_file *material, uint32_t i)
{
	VOE_BASE_ASSERT(material != NULL, "a slider of no material");
	VOE_BASE_ASSERT(i < VOE_EDITOR_MATERIAL_SLIDERS, "no such slider");
	return i == 0 ? &material->roughness : &material->metal;
}

// The path map row `i` holds: colour, normal, roughness.
static const char *map_path(const voe_assets_material_file *material,
			    uint32_t i)
{
	VOE_BASE_ASSERT(material != NULL, "a map of no material");
	VOE_BASE_ASSERT(i < VOE_EDITOR_MATERIAL_MAPS, "no such map");
	return i == 0 ? material->colormap :
	       i == 1 ? material->normalmap :
			material->ormmap;
}

// What follows the last `/` of `path`, `path` itself without one.
static const char *file_name(const char *path)
{
	const char *slash;

	VOE_BASE_ASSERT(path != NULL, "the name of no path");
	slash = strrchr(path, '/');
	return slash != NULL ? slash + 1 : path;
}

void voe_editor_inspector_material_forget(
	voe_editor_inspector_material *controls)
{
	VOE_BASE_ASSERT(controls != NULL, "forgetting no material section");

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SHADERS; i++)
		controls->shaders[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SLIDERS; i++)
		controls->sliders[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++) {
		controls->maps[i] = VOE_UI_NODE_NONE;
		controls->clears[i] = VOE_UI_NODE_NONE;
	}
	controls->section = VOE_UI_NODE_NONE;
	controls->colour = VOE_UI_NODE_NONE;
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

// The three map rows: name, file name or "None", and × while set.
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
	voe_ui_sizing swatch = { .along = { VOE_UI_SIZE_FIXED, SWATCH_WIDE },
				 .across = { VOE_UI_SIZE_FIXED, SWATCH_HIGH } };

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

	row_open(ui);
	voe_ui_label(ui, "Colour");
	controls->colour = voe_ui_button_begin(ui, "colour", 0);
	voe_ui_swatch(ui,
		      (voe_math_float3){ material->colour[0], material->colour[1],
					 material->colour[2] },
		      swatch);
	voe_ui_end(ui);
	voe_ui_end(ui);

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SLIDERS; i++) {
		const float value =
			i == 0 ? material->roughness : material->metal;

		row_open(ui);
		voe_ui_label(ui, SLIDER_NAMES[i]);
		controls->sliders[i] = voe_ui_slider(ui, "material slider", i,
						     value, 0.0f, 1.0f,
						     SLIDER_WIDE);
		snprintf(controls->figures[i], sizeof controls->figures[i],
			 "%.2f", (double)value);
		voe_ui_label(ui, controls->figures[i]);
		voe_ui_end(ui);
	}

	row_open(ui);
	voe_ui_label(ui, "Repeat");
	controls->repeat = voe_ui_number_begin(ui, "repeat", 0,
					       (double)material->repeat,
					       REPEAT_PER_MM);
	snprintf(controls->repeat_figure, sizeof controls->repeat_figure,
		 "%.2f", (double)material->repeat);
	voe_ui_label(ui, controls->repeat_figure);
	voe_ui_end(ui);
	voe_ui_end(ui);

	maps_draw(ui, controls, material);

	voe_ui_end(ui);
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
		material->ormmap
	};

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SHADERS; i++)
		if (controls->shaders[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, controls->shaders[i]).fired)
			material->shader = (voe_assets_material_shader)i;

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_SLIDERS; i++)
		if (controls->sliders[i] != VOE_UI_NODE_NONE)
			*slider_value(material, i) = (float)voe_ui_slider_action(
							     ui, controls->sliders[i],
							     0.0f, 1.0f)
							     .value;

	if (controls->repeat != VOE_UI_NODE_NONE) {
		voe_ui_number_result result =
			voe_ui_number_action(ui, controls->repeat);

		if (result.changed)
			material->repeat = (float)(result.value < REPEAT_MIN ?
							   REPEAT_MIN :
						   result.value > REPEAT_MAX ?
							   REPEAT_MAX :
							   result.value);
	}

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++)
		scene->material_controls.maps_seen[i] =
			controls->maps[i] == VOE_UI_NODE_NONE ?
				(voe_ui_rect){ 0 } :
				voe_ui_node_visible(ui, controls->maps[i]);

	for (uint32_t i = 0; i < VOE_EDITOR_MATERIAL_MAPS; i++)
		if (controls->clears[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, controls->clears[i]).fired)
			maps[i][0] = '\0';

	// The swatch opens the picker beside the section's left edge, as the
	// Inspector's opens it beside its content column's.
	if (controls->colour != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, controls->colour).fired)
		voe_editor_scene_picker_open(
			scene, (voe_editor_picking){
				       .material = true,
				       .left = controls->section != VOE_UI_NODE_NONE
						       ? voe_ui_node_rect(ui,
									  controls->section)
								 .min.x
						       : 0.0f });
}
