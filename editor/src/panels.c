// The panels' start and remember: the default tree's sizes filled into a
// voe_editor_settings, the file read over them and set back, and the same
// fields written back through settings.h.
#include "panels.h"

#include "settings.h"

#include <base/assert.h>

void voe_editor_panels_start(voe_editor_dock_root *root, voe_editor_topbar *bar)
{
	voe_editor_settings sizes;

	VOE_BASE_ASSERT(root != NULL, "starting the panels of no root");
	VOE_BASE_ASSERT(bar != NULL, "starting the panels with no bar");

	root->tree = voe_editor_dock_default();
	sizes = (voe_editor_settings){
		.scene_wide = voe_editor_dock_panel_length(
			&root->tree, VOE_EDITOR_PANEL_SCENE),
		.inspector_wide = voe_editor_dock_panel_length(
			&root->tree, VOE_EDITOR_PANEL_INSPECTOR),
		.view_share = voe_editor_dock_view_share(&root->tree),
		.assets_tall = VOE_EDITOR_DOCK_ASSETS_TALL,
	};
	voe_editor_settings_read(&sizes);
	voe_editor_dock_panel_length_set(&root->tree, VOE_EDITOR_PANEL_SCENE,
					 sizes.scene_wide);
	voe_editor_dock_panel_length_set(&root->tree, VOE_EDITOR_PANEL_INSPECTOR,
					 sizes.inspector_wide);
	voe_editor_dock_panel_length_set(&root->tree, VOE_EDITOR_PANEL_ASSETS,
					 sizes.assets_tall);
	voe_editor_dock_view_share_set(&root->tree, sizes.view_share);
	bar->wanted = sizes.topbar_high;

	VOE_BASE_ASSERT(bar->wanted >= 0.0f, "a remembered bar height below nought");
}

bool voe_editor_panels_remember(const voe_editor_dock_root *root,
				const voe_editor_topbar *bar)
{
	const voe_editor_dock_tree *tree;

	VOE_BASE_ASSERT(root != NULL, "remembering the panels of no root");
	VOE_BASE_ASSERT(bar != NULL, "remembering the panels with no bar");

	tree = &root->tree;
	return voe_editor_settings_write(&(voe_editor_settings){
		.scene_wide = voe_editor_dock_panel_length(
			tree, VOE_EDITOR_PANEL_SCENE),
		.inspector_wide = voe_editor_dock_panel_length(
			tree, VOE_EDITOR_PANEL_INSPECTOR),
		.assets_tall = voe_editor_dock_panel_length(
			tree, VOE_EDITOR_PANEL_ASSETS),
		.topbar_high = bar->wanted,
		.view_share = voe_editor_dock_view_share(tree) });
}
