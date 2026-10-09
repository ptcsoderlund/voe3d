// The Inspector's material section, drawn in place of the entity's while a
// `.material` is open (scene.h's `material_open`, 0399 point 8): the file's
// name as heading, Lit and Unlit with the chosen one lit, Colour as a swatch
// inside a button, Roughness and Metal as sliders 0–1, Repeat as a number box
// clamped 0.01–1000, and the Colour, Normal and Roughness map rows, each the
// map's file name or "None" with × while set.
//
//     voe_editor_inspector_material_forget(&scene->material_controls);
//     voe_editor_inspector_material_draw(ui, &scene->material_controls,
//                                        scene->material_open, &scene->material);
//     ...voe_ui_frame_end...
//     voe_editor_inspector_material_read(ui, scene);
//
// THE NODES LIVE IN THE SCENE, which outlives the call that drew them, for
// the reason every Inspector control does (inspector.h): a widget answers only
// after voe_ui_frame_end. Each frame forgets them first, and a frame that does
// not draw the section reads nothing.
//
// THE READ CHANGES THE SHOWN COPY ONLY, and opens the picker. Whether the copy
// now differs from the table's row, and what the store does about it, is
// interface_read.c's. A fired Colour swatch opens scene.h's picker with
// `material` set, anchored at the section's left as the Inspector's swatch is
// at its column's; each map row's rectangle is kept for a picture dropped on
// it, which assets_drag.h reads before the next frame forgets the nodes.
//
// Constraints: draw, forget and read are called in one frame's window, read
// before the frame's arena is rewound; nothing here allocates. Every label the
// section draws points into the struct or into the strings it is handed, so
// those must stand until voe_ui_frame_end.
#pragma once

#include <assets/material.h>

#include <ui/layout.h>

// Lit and Unlit, indexed as voe_assets_material_shader.
#define VOE_EDITOR_MATERIAL_SHADERS 2

// Roughness and Metal: one slider each.
#define VOE_EDITOR_MATERIAL_SLIDERS 2

// The Colour, Normal and Roughness maps: one row each.
#define VOE_EDITOR_MATERIAL_MAPS 3

// Room for a shown figure.
#define VOE_EDITOR_MATERIAL_FIGURE 16

// The section's controls as drawn this frame, VOE_UI_NODE_NONE when not, and
// the figures beside the sliders and in the number box.
typedef struct {
	// The section's panel, whose left edge anchors the picker.
	voe_ui_node section;
	voe_ui_node shaders[VOE_EDITOR_MATERIAL_SHADERS];
	voe_ui_node colour;
	voe_ui_node sliders[VOE_EDITOR_MATERIAL_SLIDERS];
	voe_ui_node repeat;
	// Each map's row, for a drop on it, and its × while the map is set.
	voe_ui_node maps[VOE_EDITOR_MATERIAL_MAPS];
	voe_ui_node clears[VOE_EDITOR_MATERIAL_MAPS];
	// Where each map's row showed at the last read, for the drop the next
	// frame (assets_drag.h); no size for one not drawn. Forget keeps them.
	voe_ui_rect maps_seen[VOE_EDITOR_MATERIAL_MAPS];
	char figures[VOE_EDITOR_MATERIAL_SLIDERS][VOE_EDITOR_MATERIAL_FIGURE];
	char repeat_figure[VOE_EDITOR_MATERIAL_FIGURE];
} voe_editor_inspector_material;

// Forgets last frame's controls. Called when the frame opens.
void voe_editor_inspector_material_forget(
	voe_editor_inspector_material *controls);

// Puts the section for `material`, open at `path` (`Assets/...`), on the
// panel, recording its nodes in `controls`.
void voe_editor_inspector_material_draw(
	voe_ui_context *ui, voe_editor_inspector_material *controls,
	const char *path, const voe_assets_material_file *material);

struct voe_editor_scene;

// After voe_ui_frame_end, on `scene->material` through
// `scene->material_controls`: a fired shader button chooses it, each drawn
// slider's value taken, Repeat's taken clamped, a fired × empties its map, a
// fired swatch opens the picker on the material; each map row's rectangle kept.
void voe_editor_inspector_material_read(const voe_ui_context *ui,
					struct voe_editor_scene *scene);
