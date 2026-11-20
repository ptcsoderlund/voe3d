// The Inspector's tree section, drawn in place of the entity's while a species
// is open (scene.h's `tree_open`, 0515 point 6): the file's name as heading,
// the Bark and Leaf rows, each its `.material` path or "None" with × while
// set, a slider per number key (Height, Trunk, Crown, Branching, Reach, Angle,
// Droop, Crooked, Leaf size, Leaf density) over assets/tree.h's _MIN.._MAX with
// its figure at two decimals and unit (0498 point 3), Variations as a number
// box clamped VOE_ASSETS_TREE_VARIATIONS_MIN–_MAX, and the Variation stepper,
// ◀ v / count ▶, counting the shown variation from one.
//
//     voe_editor_inspector_tree_forget(&scene->tree_controls);
//     voe_editor_inspector_tree_draw(ui, &scene->tree_controls,
//                                    scene->tree_open, &scene->tree,
//                                    scene->tree_variation);
//     ...voe_ui_frame_end...
//     voe_editor_inspector_tree_read(ui, scene);
//
// THE NODES LIVE IN THE SCENE, which outlives the call that drew them, for
// the reason every Inspector control does (inspector.h): a widget answers only
// after voe_ui_frame_end. Each frame forgets them first, and a frame that does
// not draw the section reads nothing.
//
// THE READ CHANGES THE SHOWN COPY AND THE STEPPED VARIATION ONLY. Whether the
// copy now differs from the table's row, what the store does about it, and
// stepping the world's tree to the variation, are interface_read.c's. Each
// material row's rectangle is kept for a `.material` dropped on it, which
// assets_drag.h reads before the next frame forgets the nodes.
//
// Constraints: draw, forget and read are called in one frame's window, read
// before the frame's arena is rewound; nothing here allocates. Every label the
// section draws points into the struct or into the strings it is handed, so
// those must stand until voe_ui_frame_end.
#pragma once

#include <assets/tree.h>

#include <ui/layout.h>

#include <stdint.h>

// Bark and Leaf: one row each.
#define VOE_EDITOR_TREE_MATERIALS 2

// Height, Trunk, Crown, Branching, Reach, Angle, Droop, Crooked, Leaf size and
// Leaf density: one slider each.
#define VOE_EDITOR_TREE_SLIDERS 10

// Room for a shown figure, "v / count" among them.
#define VOE_EDITOR_TREE_FIGURE 16

// The section's controls as drawn this frame, VOE_UI_NODE_NONE when not, and
// the figures beside the sliders, in the number box and between the steps.
typedef struct {
	// Each material row, for a drop on it, and its × while its path is set.
	voe_ui_node materials[VOE_EDITOR_TREE_MATERIALS];
	voe_ui_node clears[VOE_EDITOR_TREE_MATERIALS];
	// Where each material row showed at the last read, for the drop the
	// next frame (assets_drag.h); no size for one not drawn. Forget keeps
	// them.
	voe_ui_rect materials_seen[VOE_EDITOR_TREE_MATERIALS];
	voe_ui_node sliders[VOE_EDITOR_TREE_SLIDERS];
	voe_ui_node variations;
	// The stepper's ◀ and ▶.
	voe_ui_node previous;
	voe_ui_node next;
	char figures[VOE_EDITOR_TREE_SLIDERS][VOE_EDITOR_TREE_FIGURE];
	char variations_figure[VOE_EDITOR_TREE_FIGURE];
	char variation_figure[VOE_EDITOR_TREE_FIGURE];
} voe_editor_inspector_tree;

// Forgets last frame's controls. Called when the frame opens.
void voe_editor_inspector_tree_forget(voe_editor_inspector_tree *controls);

// Puts the section for `tree`, open at `path` (`Assets/...`) and showing
// variation `variation` (from 0; one past the count, not yet clamped, shows
// as the last one), on the panel, recording its nodes in `controls`.
void voe_editor_inspector_tree_draw(voe_ui_context *ui,
				    voe_editor_inspector_tree *controls,
				    const char *path, const voe_assets_tree *tree,
				    uint32_t variation);

struct voe_editor_scene;

// After voe_ui_frame_end, on `scene->tree` through `scene->tree_controls`:
// each drawn slider's value taken, Variations taken rounded and clamped, a
// fired × empties its path, and a fired ◀ or ▶ steps `scene->tree_variation`
// one down or up, wrapping within the variations as they now are; each
// material row's rectangle kept. Clamping the variation when Variations falls
// is scene.h's voe_editor_scene_tree_clamp.
void voe_editor_inspector_tree_read(const voe_ui_context *ui,
				    struct voe_editor_scene *scene);
