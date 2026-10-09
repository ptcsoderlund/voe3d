// The walk: a tree of nodes in, one frame of `ui` calls out, and what is on
// each panel. dock.c lays the tree out; this file draws what it laid out.
//
// EVERY CHILD IS GIVEN A FIXED SIZE IN MILLIMETRES AND NOTHING GROWS. The walk
// already knows how big the surface is, so it can divide it on the way down, and
// each child is told the number it came to. The alternative — weights and
// VOE_UI_SIZE_GROW — hands the division to `ui` and then a splitter has to read
// a rectangle back out to find out what it is dragging. voe_editor_dock_arrange
// is the one division both the walk and a drag read. A node it did not lay
// emits nothing, and a split with one laid child emits that child alone.
//
// A SEAM IS DRAWN IN THE BORDER COLOUR, so it reads like every other border;
// the reached one is drawn as the inverse pair, `inverse_ink` beside
// `inverse`, which shows over any picture in any theme (0194, 0196, 0231, 0232).
//
// THE TREE IS WRAPPED IN A ROW OF ITS OWN, WHICH IS ONE NODE AND IS NOT
// CEREMONY. `ui` needs a row or a column to hold a leaf's panel (see
// ui/layout.h) and a leaf emits a panel, so a tree that is a single panel
// filling what it was given would otherwise be the one shape the walk could
// not express. That row is a child like any other, which is why
// voe_editor_dock_walk takes `parent`: interface.c opens a column above it for
// the top bar, and a child's `size.along`/`.across` are read against its
// PARENT's flow, never its own, as walk_node's `parent` accounts for on every
// node below. `sizing_in()` is the one place that turns a size into the pair
// read correctly either way, the row's own included.
//
// EVERY CLOSABLE LEAF STARTS WITH A HEADER ROW (0363 point 2): its name at the
// left, an × at the right, recorded in the caller's voe_editor_dock_closes.
// The top view has none; it never closes.
//
// A SCENE VIEW'S PANEL HAS NO PADDING, AND ITS PICTURE TAKES ALL BUT THE
// HEADER. The rectangle the picture came to is what the view's target is sized
// by; padding would be a frame of panel colour round a picture that already
// has an edge. While the camera preview is shown, its picture sits over the
// view's bottom-right corner at 30% of the view's width.
//
// EVERY OTHER LEAF IS A SCROLL AREA UNDER ITS HEADER, AND THE PANEL AROUND IT
// IS NOT (ADR-0153 point 11). The panel keeps the background, the size the walk
// divided out and the padding; one voe_ui_scroll_begin grows to fill what is
// left and everything the leaf draws goes in there, so nothing a panel says
// reaches past the panel's edges and a column too small for its content
// scrolls instead of spilling over its neighbour. The area stretches its
// content across the flow, which is what gives a row that wraps a width to
// wrap against (ui/layout.h, A RUN THAT WRAPS). The header stays put while it
// scrolls. The Scene list's draw is handed the palette for its drag marks, and
// the Inspector's the open material.
//
// EVERY LEAF'S OWN SURFACE IS THE THEME'S ORDINARY SURFACE. The regions are
// told apart by the seam between them and by where their headings start, not
// by colour — a colour per panel would be this file deciding something nobody
// has decided, and there is no role for it besides (ui/theme.h, ADR-0171).
#include "dock.h"

#include "inspector.h"
#include "scene_list.h"
#include "themes.h"

#include <base/assert.h>

#include <math/float4.h>

#include <scene/camera_component.h>
#include <scene/identity_component.h>

#include <ui/colour.h>

// Inside a panel's four edges, and between the things on it. Millimetres.
#define PANEL_PAD (3.0f * VOE_EDITOR_SPACING)
#define PANEL_GAP (2.0f * VOE_EDITOR_SPACING)

// A child's size, in the axes its PARENT flows in. `ui` reads `size.along` and
// `size.across` against the container the child is in, so which of x and y is
// which depends on the parent and on nothing else — see ui/layout.h.
static voe_ui_sizing sizing_in(voe_editor_dock_axis parent,
			       voe_math_float2 size)
{
	if (parent == VOE_EDITOR_DOCK_ROW)
		return (voe_ui_sizing){
			.along = { VOE_UI_SIZE_FIXED, size.x },
			.across = { VOE_UI_SIZE_FIXED, size.y }
		};

	return (voe_ui_sizing){ .along = { VOE_UI_SIZE_FIXED, size.y },
				.across = { VOE_UI_SIZE_FIXED, size.x } };
}

// The key `ui` remembers a panel's widgets by. It is a name and the panel is
// still not a string: this is the one place the enumeration is spelled for
// `ui`'s benefit, and nothing outside this file may pass a panel by name.
static const char *panel_key(voe_editor_panel panel)
{
	switch (panel) {
	case VOE_EDITOR_PANEL_SCENE:
		return "scene";
	case VOE_EDITOR_PANEL_INSPECTOR:
		return "inspector";
	case VOE_EDITOR_PANEL_SCENE_VIEW:
		return "scene_view";
	case VOE_EDITOR_PANEL_ASSETS:
		return "assets";
	case VOE_EDITOR_PANEL_COUNT:
		break;
	}

	VOE_BASE_ASSERT(false, "a dock leaf naming no panel");
	return "";
}

// A split's seam, VOE_EDITOR_DOCK_SEAM along it and filling across. Not lit,
// one fill of `border`; lit, two stripes each half of it: the first child's
// side `inverse_ink`, the second's `inverse` (0231, 0232).
static void seam_draw(voe_ui_context *ui, voe_editor_dock_axis axis,
		      const voe_ui_theme *palette, bool lit)
{
	voe_ui_container box = {
		.size = { .along = { VOE_UI_SIZE_FIXED, VOE_EDITOR_DOCK_SEAM } },
		.across = VOE_UI_ACROSS_FILL
	};
	voe_ui_sizing stripe = { .along = { VOE_UI_SIZE_FIXED,
					    VOE_EDITOR_DOCK_SEAM / 2.0f } };
	voe_ui_sizing whole = { .along = { VOE_UI_SIZE_FIXED,
					   VOE_EDITOR_DOCK_SEAM } };
	voe_math_float4 dark = palette->inverse_ink;
	voe_math_float4 light = palette->inverse;
	voe_math_float4 rest = palette->border;

	VOE_BASE_ASSERT(ui != NULL, "drawing a seam into no interface");
	VOE_BASE_ASSERT(palette != NULL, "drawing a seam with no palette");

	if (axis == VOE_EDITOR_DOCK_ROW)
		voe_ui_row_begin(ui, box);
	else
		voe_ui_column_begin(ui, box);
	if (lit) {
		voe_ui_swatch(ui, (voe_math_float3){ dark.x, dark.y, dark.z },
			      stripe);
		voe_ui_swatch(ui, (voe_math_float3){ light.x, light.y, light.z },
			      stripe);
	} else {
		voe_ui_swatch(ui, (voe_math_float3){ rest.x, rest.y, rest.z },
			      whole);
	}
	voe_ui_end(ui);
}

// A closable leaf's header: its name at the left and its × at the right,
// spread apart along a row the panel stretches across. Over a picture, which
// has no padding of its own, the row carries the panel's.
static void header_draw(voe_ui_context *ui, voe_editor_closable which,
			bool picture, voe_editor_dock_closes *closes)
{
	float side = picture ? PANEL_PAD : 0.0f;

	VOE_BASE_ASSERT(which < VOE_EDITOR_CLOSABLE_DOCKED,
			"a header on a leaf that is not a closable dock panel");
	VOE_BASE_ASSERT(closes != NULL, "a header with nowhere to record its ×");

	voe_ui_row_begin(ui, (voe_ui_container){
				     .along = VOE_UI_ALONG_SPREAD,
				     .across = VOE_UI_ACROSS_CENTER,
				     .pad = { side, picture ? PANEL_GAP : 0.0f,
					      side, 0.0f } });
	voe_ui_label(ui, voe_editor_closable_name(which));
	closes->close[which] = voe_ui_button_begin(ui, "close", which);
	voe_ui_label(ui, "×");
	voe_ui_end(ui); // button
	voe_ui_end(ui); // row
}

static void walk_node(voe_ui_context *ui, const voe_editor_dock_tree *tree,
		      const voe_editor_dock_arrangement *places,
		      uint32_t index, voe_editor_dock_axis parent,
		      voe_math_float2 size, uint32_t depth, uint32_t lit,
		      const voe_ui_theme *palette, voe_editor_scene *scene,
		      voe_editor_views *views, voe_editor_dock_closes *closes)
{
	const voe_editor_dock_node *node;
	voe_math_float2 head;
	voe_math_float2 tail;

	VOE_BASE_ASSERT(depth < VOE_EDITOR_DOCK_DEPTH,
			"a dock tree deeper than VOE_EDITOR_DOCK_DEPTH — the child indices are a cycle, or this is not the shallow tree a dock is");
	VOE_BASE_ASSERT(index < tree->count,
			"a dock node naming a child past the end of the tree");

	node = &tree->nodes[index];
	if (!places->nodes[index].laid)
		return;

	if (node->kind == VOE_EDITOR_DOCK_LEAF) {
		bool picture = node->panel == VOE_EDITOR_PANEL_SCENE_VIEW;
		float pad = picture ? 0.0f : PANEL_PAD;
		voe_editor_closable which = voe_editor_dock_leaf_closable(node);

		// Two leaves may show the same kind of panel, so a scene view is
		// keyed by the view it shows as well as by its name.
		voe_ui_panel_begin(
			ui, panel_key(node->panel), picture ? node->view : 0,
			VOE_UI_SURFACE_SURFACE,
			(voe_ui_container){
				.size = sizing_in(parent, size),
				.across = VOE_UI_ACROSS_FILL,
				// Between the header and what is under it; the
				// things on a leaf are spaced by its scroll
				// area's own gap below.
				.gap = PANEL_GAP,
				.pad = { pad, pad, pad, pad } });

		if (which != VOE_EDITOR_CLOSABLE_COUNT)
			header_draw(ui, which, picture, closes);

		if (picture) {
			voe_editor_panel_draw(ui, node->panel, node->view,
					      palette, scene, views);
			voe_ui_end(ui);
			return;
		}

		// Keyed by the panel and the view, exactly as the panel above
		// is: the area is inside that panel, so its key is already
		// distinct from every other leaf's, and the view keeps two
		// leaves of one kind apart the day there are two.
		voe_ui_node area = voe_ui_scroll_begin(
			ui, panel_key(node->panel), node->view,
			(voe_ui_container){
				.size = { .along = { VOE_UI_SIZE_GROW, 1.0f } },
				.across = VOE_UI_ACROSS_FILL,
				.gap = PANEL_GAP },
			(voe_ui_scroll_axes){ .x = true, .y = true });

		// The Inspector's open list is drawn inside this area and
		// clipped by it, so this is the rectangle it fits itself into
		// (ADR-0200, inspector.h).
		if (node->panel == VOE_EDITOR_PANEL_INSPECTOR)
			voe_editor_inspector_area_set(&scene->inspector, area);
		// The Scene list's, which a reveal scrolls its row into (scene.h).
		if (node->panel == VOE_EDITOR_PANEL_SCENE)
			scene->list_area = area;

		voe_editor_panel_draw(ui, node->panel, node->view,
				      palette, scene, views);
		voe_ui_end(ui);
		voe_ui_end(ui);
		return;
	}

	// One laid child is emitted alone at the split's size, with no
	// container and no seam of the split's own around it.
	if (!places->nodes[index].seamed) {
		walk_node(ui, tree, places,
			  places->nodes[node->first].laid ? node->first :
							    node->second,
			  parent, size, depth + 1, lit, palette, scene, views,
			  closes);
		return;
	}

	// The children's sizes are the arrangement's, which is the one division
	// there is (see voe_editor_dock_arrange).
	head = places->nodes[node->first].rect.size;
	tail = places->nodes[node->second].rect.size;
	// The seam is a box of VOE_EDITOR_DOCK_SEAM between them, so the
	// lengths add up to the split's as the arrangement divided it.
	if (node->axis == VOE_EDITOR_DOCK_ROW) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .size = sizing_in(parent, size),
					     .across = VOE_UI_ACROSS_FILL });
	} else {
		voe_ui_column_begin(ui, (voe_ui_container){
						.size = sizing_in(parent, size),
						.across = VOE_UI_ACROSS_FILL });
	}

	walk_node(ui, tree, places, node->first, node->axis, head, depth + 1,
		  lit, palette, scene, views, closes);
	seam_draw(ui, node->axis, palette, index == lit);
	walk_node(ui, tree, places, node->second, node->axis, tail, depth + 1,
		  lit, palette, scene, views, closes);
	voe_ui_end(ui);
}

void voe_editor_dock_walk(const voe_editor_dock_root *root,
			  voe_editor_dock_axis parent, voe_ui_context *ui,
			  const voe_ui_theme *palette, voe_editor_scene *scene,
			  voe_editor_views *views, voe_editor_dock_closes *closes)
{
	voe_editor_dock_arrangement places;

	VOE_BASE_ASSERT(root != NULL, "walking no dock root");
	VOE_BASE_ASSERT(ui != NULL, "walking a dock root into no interface");
	VOE_BASE_ASSERT(root->tree.count > 0, "walking an empty dock tree");
	VOE_BASE_ASSERT(root->size.x > 0.0f && root->size.y > 0.0f,
			"walking a dock root onto a surface with no area");
	VOE_BASE_ASSERT(palette != NULL, "walking a dock root with no palette");
	VOE_BASE_ASSERT(scene != NULL, "walking a dock root with no scene");
	VOE_BASE_ASSERT(views != NULL, "walking a dock root with no views");
	VOE_BASE_ASSERT(closes != NULL, "walking a dock root with no closes");

	// A leaf the walk does not reach draws no ×.
	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_DOCKED; c++)
		closes->close[c] = VOE_UI_NODE_NONE;

	// Last frame's rows and pictures named last frame's nodes and the arena
	// they were in has gone. The panels record this frame's as they draw.
	voe_editor_scene_rows_clear(scene);
	voe_editor_views_images_clear(views);

	// sizing_in(parent, root->size) IS root->size READ IN WHATEVER AXES
	// `parent` FLOWS IN — see the header on why this row needs that now
	// that it is not always the frame's own root.
	voe_editor_dock_arrange(&root->tree, root->closed,
				(voe_ui_rect){ .size = root->size }, &places);
	// A view the walk does not show keeps no rectangle, so
	// voe_editor_views_under (view.h) cannot find it under the pointer.
	for (uint32_t v = 0; v < views->count; v++)
		if (!voe_editor_dock_shows_view(root, v))
			views->views[v].rect = (voe_ui_rect){ 0 };
	voe_ui_row_begin(ui, (voe_ui_container){ .size = sizing_in(
							 parent, root->size),
						 .across = VOE_UI_ACROSS_FILL });
	walk_node(ui, &root->tree, &places, root->tree.root,
		  VOE_EDITOR_DOCK_ROW, root->size, 0, root->lit, palette, scene,
		  views, closes);
	voe_ui_end(ui);
}

voe_editor_closable
voe_editor_dock_closes_read(const voe_ui_context *ui,
			    const voe_editor_dock_closes *closes)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the closes of no interface");
	VOE_BASE_ASSERT(closes != NULL, "reading no closes");

	for (uint32_t c = 0; c < VOE_EDITOR_CLOSABLE_DOCKED; c++)
		if (closes->close[c] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, closes->close[c]).fired)
			return (voe_editor_closable)c;

	return VOE_EDITOR_CLOSABLE_COUNT;
}

// WHAT IS ON THE INSPECTOR IS ONE CALL AND NOT A SECOND SCENE PANEL. It is
// handed the world, the selection and the types it keeps and nothing else,
// because what it lists is the world's own component types and not anything this
// folder knows the name of — see inspector.h. The identity and the camera are
// the ones it is told of, never removable: the Scene list is built from the
// first, and the scene's one camera keeps its component (ADR-0218). The
// transform is not among them: it is an ordinary component, removable when no
// other row of the entity needs it (0300, 0302), which the Inspector decides.
static void inspector_panel(voe_ui_context *ui, voe_editor_scene *scene)
{
	const voe_ecs_type kept[] = {
		voe_ecs_component_type(scene->world, &voe_scene_identity_key),
		voe_ecs_component_type(scene->world, &voe_scene_camera_key),
	};

	// An open material is drawn in place of the entity (scene.h).
	voe_editor_inspector_draw(ui, &scene->inspector, scene->world,
				  voe_editor_scene_selected(scene), kept,
				  sizeof(kept) / sizeof(kept[0]), &scene->sculpt,
				  scene->material_open, &scene->material,
				  &scene->material_controls);
}

// ONE PICTURE, THE WHOLE OF THE VIEW'S TEXTURE, GROWING TO FILL THE PANEL. The
// panel stretches it across and it grows along, so its rectangle is the panel's;
// the node is handed to the view to be asked where it sat once the frame has
// ended, which is the size the view's target is drawn at next frame (view.h).
//
// The preview over it is sized from last frame's rectangle, as the view's own
// target is, so a view that has not sat anywhere yet shows none; its node is not
// kept, so a click on it is a click on the view (0223).
#define PREVIEW_SHARE 0.3f
#define PREVIEW_INSET (1.0f * VOE_EDITOR_SPACING)

static void scene_view_panel(voe_ui_context *ui, uint32_t view,
			     voe_editor_views *views)
{
	VOE_BASE_ASSERT(view < views->count,
			"a dock leaf showing a scene view the editor does not have");

	views->views[view].image = voe_ui_image(
		ui, views->views[view].texture,
		(voe_math_float4){ 0.0f, 0.0f, 1.0f, 1.0f },
		(voe_math_float2){ 0.0f, 0.0f },
		(voe_ui_sizing){ .along = { VOE_UI_SIZE_GROW, 1.0f } });

	float width = views->views[view].rect.size.x * PREVIEW_SHARE;

	if (!views->preview_shown || width <= 0.0f)
		return;

	// Anchored, so it is out of the panel's run and paints after the view's
	// picture; a row, because an anchor is a container's and not a leaf's.
	voe_ui_row_begin(
		ui, (voe_ui_container){
			    .size = { .along = { VOE_UI_SIZE_FIXED, width },
				      .across = { VOE_UI_SIZE_FIXED,
						  width * VOE_EDITOR_PREVIEW_HEIGHT /
							  VOE_EDITOR_PREVIEW_WIDTH } },
			    .across = VOE_UI_ACROSS_FILL,
			    .anchor = { .anchored = true,
					.x = { VOE_UI_ACROSS_END, PREVIEW_INSET },
					.y = { VOE_UI_ACROSS_END,
					       PREVIEW_INSET } } });
	voe_ui_image(ui, views->preview_texture,
		     (voe_math_float4){ 0.0f, 0.0f, 1.0f, 1.0f },
		     (voe_math_float2){ 0.0f, 0.0f },
		     (voe_ui_sizing){ .along = { VOE_UI_SIZE_GROW, 1.0f } });
	voe_ui_end(ui);
}

void voe_editor_panel_draw(voe_ui_context *ui, voe_editor_panel panel,
			   uint32_t view, const voe_ui_theme *palette,
			   voe_editor_scene *scene, voe_editor_views *views)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a panel into no interface");
	VOE_BASE_ASSERT(palette != NULL, "drawing a panel with no palette");
	VOE_BASE_ASSERT(scene != NULL, "drawing a panel with no scene");
	VOE_BASE_ASSERT(scene->world != NULL,
			"drawing a panel onto a scene with no world");
	VOE_BASE_ASSERT(views != NULL, "drawing a panel with no views");

	switch (panel) {
	case VOE_EDITOR_PANEL_SCENE:
		voe_editor_scene_list_draw(ui, palette, scene);
		return;
	case VOE_EDITOR_PANEL_INSPECTOR:
		inspector_panel(ui, scene);
		return;
	case VOE_EDITOR_PANEL_SCENE_VIEW:
		scene_view_panel(ui, view, views);
		return;
	case VOE_EDITOR_PANEL_ASSETS:
		voe_ui_label(ui, "Assets");
		voe_editor_assets_draw(ui, &scene->assets);
		return;
	case VOE_EDITOR_PANEL_COUNT:
		break;
	}

	VOE_BASE_ASSERT(false, "drawing a panel that is not one");
}
