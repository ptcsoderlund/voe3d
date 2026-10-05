// The panels' start and remember: the default tree's sizes and every panel
// open filled into a voe_editor_settings, the file read over them and set back
// into the tree and `root->closed`, and the same fields written back through
// settings.h; a toggle flips one flag and remembers at once.
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
		.scene_open = true,
		.assets_open = true,
		.inspector_open = true,
		.view_open = true,
	};
	voe_editor_settings_read(&sizes);
	root->closed[VOE_EDITOR_CLOSABLE_SCENE_LIST] = !sizes.scene_open;
	root->closed[VOE_EDITOR_CLOSABLE_ASSETS] = !sizes.assets_open;
	root->closed[VOE_EDITOR_CLOSABLE_INSPECTOR] = !sizes.inspector_open;
	root->closed[VOE_EDITOR_CLOSABLE_BOTTOM_VIEW] = !sizes.view_open;
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
		.view_share = voe_editor_dock_view_share(tree),
		.scene_open = !root->closed[VOE_EDITOR_CLOSABLE_SCENE_LIST],
		.assets_open = !root->closed[VOE_EDITOR_CLOSABLE_ASSETS],
		.inspector_open = !root->closed[VOE_EDITOR_CLOSABLE_INSPECTOR],
		.view_open = !root->closed[VOE_EDITOR_CLOSABLE_BOTTOM_VIEW] });
}

bool voe_editor_panels_toggle(voe_editor_closable which,
			      voe_editor_dock_root *root,
			      const voe_editor_topbar *bar)
{
	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_DOCKED,
			"toggling a panel that is not a dock panel");
	VOE_BASE_ASSERT(root != NULL, "toggling a panel of no root");

	root->closed[which] = !root->closed[which];
	return voe_editor_panels_remember(root, bar);
}
