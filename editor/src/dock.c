// The walk: a tree of nodes in, one frame of `ui` calls out. See the header for
// why the layout is data rather than call order, and for what a root is.
//
// EVERY CHILD IS GIVEN A FIXED SIZE IN MILLIMETRES AND NOTHING GROWS. The walk
// already knows how big the surface is, so it can divide it on the way down, and
// each child is told the number it came to. The alternative — weights and
// VOE_UI_SIZE_GROW — hands the division to `ui` and then a splitter has to read
// a rectangle back out to find out what it is dragging. A fraction of a known
// length is the number a drag will write to.
//
// THE SEAM IS A GAP AND NOT A DRAWN DIVIDER. A millimetre is taken off the
// split's length before the fraction divides it, and neither child fills it, so
// two panels read as two regions rather than as one. It is also exactly where a
// splitter goes when there is one; nothing draws anything in it today and
// nothing hit-tests it.
//
// THE TREE IS WRAPPED IN A ROW OF ITS OWN, WHICH IS ONE NODE AND IS NOT
// CEREMONY. `ui` needs a row or a column to hold a leaf's panel (see
// ui/layout.h) and a leaf emits a panel, so a tree that is a single panel
// filling what it was given would otherwise be the one shape the walk could
// not express.
//
// THAT ROW IS A CHILD LIKE ANY OTHER HERE, WHICH IS WHY voe_editor_dock_walk
// TAKES `parent`. It used to be the frame's actual root, where
// ui/layout.h says a size is read against its own flow; interface.c now opens
// a column above it for the top bar, which makes this row an ordinary child
// again — and a child's `voe_ui_container.size.along`/`.across` are read
// against its PARENT's flow, never its own, exactly as walk_node's `parent`
// argument already accounts for on every node below this one. `sizing_in()`
// is the one place that turns root->size into the pair read correctly either
// way, and voe_editor_dock_walk uses it for its own row now instead of
// writing `.along`/`.across` out by hand.
//
// A SCENE VIEW'S PANEL HAS NO PADDING, AND ITS PICTURE IS ITS WHOLE CHILD. The
// picture fills the panel edge to edge, so the rectangle the picture came to is
// the panel's own and is what the view's target is sized by; padding would be a
// frame of panel colour round a picture that already has an edge.
//
// EVERY LEAF THAT IS NOT A PICTURE IS A SCROLL AREA, AND THE PANEL AROUND IT IS
// NOT (ADR-0153 point 11). The panel keeps the background, the size the walk
// divided out and the padding; inside it one voe_ui_scroll_begin grows to fill
// what the padding leaves and everything the leaf draws goes in there, so
// nothing a panel says can reach past the panel's own edges and a column too
// small for its content scrolls instead of spilling over its neighbour. The area
// stretches its content across the flow, which is what gives a row that wraps a
// width to wrap against (ui/layout.h, A RUN THAT WRAPS); the panel's gap moves
// onto it, the panel being one child now. A scene view is left alone: its
// picture is already sized to its panel, and a bar over it would be a bar over
// the scene.
#include "dock.h"

#include "inspector.h"

#include <base/assert.h>

#include <math/float4.h>

#include <scene/identity_component.h>

// Every leaf's own surface is the theme's ordinary SURFACE, the same role for
// all of them. The two regions are told apart by the seam between them and by
// where their headings start, not by being different colours — a colour per
// panel would be this file deciding something nobody has decided, and there
// is no role for it besides (ui/theme.h, ADR-0169).

// Inside a panel's four edges, and between the things on it. Millimetres.
#define PANEL_PAD 3.0f
#define PANEL_GAP 2.0f

// The space between the two children of a split. See the header note above.
#define SEAM 1.0f

// What goes in front of the selected row's name. IT IS A SECOND LABEL IN THE
// BUTTON AND NOT A COMPOSED STRING, which is the shape ui/widgets.h asks for —
// a button is a row and a second thing in one goes across. The alternative is a
// buffer holding the name with the marker glued on, and that buffer would have
// to outlive the call that filled it: a label's text is read at
// voe_ui_frame_end, long after this function has returned. A literal and a
// pointer into the identity table are both still there then.
#define SELECTED_MARK "> "

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
	case VOE_EDITOR_PANEL_COUNT:
		break;
	}

	VOE_BASE_ASSERT(false, "a dock leaf naming no panel");
	return "";
}

static void walk_node(voe_ui_context *ui, const voe_editor_dock_tree *tree,
		      uint32_t index, voe_editor_dock_axis parent,
		      voe_math_float2 size, uint32_t depth,
		      voe_editor_scene *scene, voe_editor_views *views)
{
	const voe_editor_dock_node *node;
	voe_math_float2 head = size;
	voe_math_float2 tail = size;
	float along;
	float share;
	float first;

	VOE_BASE_ASSERT(depth < VOE_EDITOR_DOCK_DEPTH,
			"a dock tree deeper than VOE_EDITOR_DOCK_DEPTH — the child indices are a cycle, or this is not the shallow tree a dock is");
	VOE_BASE_ASSERT(index < tree->count,
			"a dock node naming a child past the end of the tree");

	node = &tree->nodes[index];

	if (node->kind == VOE_EDITOR_DOCK_LEAF) {
		bool picture = node->panel == VOE_EDITOR_PANEL_SCENE_VIEW;
		float pad = picture ? 0.0f : PANEL_PAD;

		// Two leaves may show the same kind of panel, so a scene view is
		// keyed by the view it shows as well as by its name.
		voe_ui_panel_begin(
			ui, panel_key(node->panel), picture ? node->view : 0,
			VOE_UI_SURFACE_SURFACE,
			(voe_ui_container){
				.size = sizing_in(parent, size),
				.across = VOE_UI_ACROSS_FILL,
				// The gap between the things on a leaf is the
				// scroll area's below; the panel holds one
				// child and has nothing to space.
				.gap = picture ? PANEL_GAP : 0.0f,
				.pad = { pad, pad, pad, pad } });

		if (picture) {
			voe_editor_panel_draw(ui, node->panel, node->view,
					      scene, views);
			voe_ui_end(ui);
			return;
		}

		// Keyed by the panel and the view, exactly as the panel above
		// is: the area is inside that panel, so its key is already
		// distinct from every other leaf's, and the view keeps two
		// leaves of one kind apart the day there are two.
		voe_ui_scroll_begin(
			ui, panel_key(node->panel), node->view,
			(voe_ui_container){
				.size = { .along = { VOE_UI_SIZE_GROW, 1.0f } },
				.across = VOE_UI_ACROSS_FILL,
				.gap = PANEL_GAP },
			(voe_ui_scroll_axes){ .x = true, .y = true });
		voe_editor_panel_draw(ui, node->panel, node->view, scene,
				      views);
		voe_ui_end(ui);
		voe_ui_end(ui);
		return;
	}

	VOE_BASE_ASSERT(node->fraction > 0.0 && node->fraction < 1.0,
			"a dock split whose fraction is not between nought and one");

	// The seam comes off the length before the fraction divides it, so the
	// two children and the gap between them add up to exactly what this
	// node was given. A surface too narrow to hold the seam leaves both
	// children at nothing rather than at a negative size, which `ui` would
	// lay out as a rectangle that is inside out.
	along = node->axis == VOE_EDITOR_DOCK_ROW ? size.x : size.y;
	share = along - SEAM;
	if (share < 0.0f)
		share = 0.0f;
	first = (float)((double)share * node->fraction);

	if (node->axis == VOE_EDITOR_DOCK_ROW) {
		head.x = first;
		tail.x = share - first;
		voe_ui_row_begin(ui, (voe_ui_container){
					     .size = sizing_in(parent, size),
					     .across = VOE_UI_ACROSS_FILL,
					     .gap = SEAM });
	} else {
		head.y = first;
		tail.y = share - first;
		voe_ui_column_begin(ui, (voe_ui_container){
						.size = sizing_in(parent, size),
						.across = VOE_UI_ACROSS_FILL,
						.gap = SEAM });
	}

	walk_node(ui, tree, node->first, node->axis, head, depth + 1, scene,
		  views);
	walk_node(ui, tree, node->second, node->axis, tail, depth + 1, scene,
		  views);
	voe_ui_end(ui);
}

voe_editor_dock_tree voe_editor_dock_default(void)
{
	voe_editor_dock_tree tree = { 0 };

	// Three columns out of two ROW splits: the Scene list takes a fifth off
	// the left, and what is left is split three quarters to one, so the
	// middle gets three fifths of the whole and the Inspector the last fifth.
	tree.nodes[0] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_ROW,
						.fraction = 0.2,
						.first = 1,
						.second = 2 };
	tree.nodes[1] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_LEAF,
						.panel = VOE_EDITOR_PANEL_SCENE };
	tree.nodes[2] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_ROW,
						.fraction = 0.75,
						.first = 3,
						.second = 6 };
	// THE TWO VIEWS ARE A COLUMN SPLIT AT A HALF, AND WHICH IS ON TOP IS THE
	// `view` ON EACH LEAF. Swap the two numbers and the pictures change
	// places, with nothing else in this folder touched — the edit a tree of
	// data exists to make that small.
	tree.nodes[3] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_COLUMN,
						.fraction = 0.5,
						.first = 4,
						.second = 5 };
	tree.nodes[4] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_SCENE_VIEW,
		.view = 0
	};
	tree.nodes[5] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_SCENE_VIEW,
		.view = 1
	};
	tree.nodes[6] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_INSPECTOR
	};
	tree.count = 7;
	tree.root = 0;

	return tree;
}

// Every node and not a walk from the root: a leaf the root cannot reach is not
// laid out either, but a tree with one in it is a tree nobody built on purpose,
// and the walk already asserts on the shapes that would make one.
bool voe_editor_dock_shows_view(const voe_editor_dock_tree *tree, uint32_t view)
{
	VOE_BASE_ASSERT(tree != NULL, "asking no tree what it shows");

	for (uint32_t i = 0; i < tree->count; i++) {
		const voe_editor_dock_node *node = &tree->nodes[i];

		if (node->kind == VOE_EDITOR_DOCK_LEAF &&
		    node->panel == VOE_EDITOR_PANEL_SCENE_VIEW &&
		    node->view == view)
			return true;
	}

	return false;
}

void voe_editor_dock_walk(const voe_editor_dock_root *root,
			  voe_editor_dock_axis parent, voe_ui_context *ui,
			  voe_editor_scene *scene, voe_editor_views *views)
{
	VOE_BASE_ASSERT(root != NULL, "walking no dock root");
	VOE_BASE_ASSERT(ui != NULL, "walking a dock root into no interface");
	VOE_BASE_ASSERT(root->tree.count > 0, "walking an empty dock tree");
	VOE_BASE_ASSERT(root->size.x > 0.0f && root->size.y > 0.0f,
			"walking a dock root onto a surface with no area");
	VOE_BASE_ASSERT(scene != NULL, "walking a dock root with no scene");
	VOE_BASE_ASSERT(views != NULL, "walking a dock root with no views");

	// Last frame's rows and pictures named last frame's nodes and the arena
	// they were in has gone. The panels record this frame's as they draw.
	voe_editor_scene_rows_clear(scene);
	voe_editor_views_images_clear(views);

	// sizing_in(parent, root->size) IS root->size READ IN WHATEVER AXES
	// `parent` FLOWS IN — see the header on why this row needs that now
	// that it is not always the frame's own root.
	voe_ui_row_begin(ui, (voe_ui_container){ .size = sizing_in(
							 parent, root->size),
						 .across = VOE_UI_ACROSS_FILL });
	walk_node(ui, &root->tree, root->tree.root, VOE_EDITOR_DOCK_ROW,
		  root->size, 0, scene, views);
	voe_ui_end(ui);
}

// THE LIST IS THE IDENTITY TABLE AND NOTHING ELSE. It walks
// voe_scene_identity_rows and _entities rather than a list the editor keeps, so
// an entity the engine made for itself — no identity, hence not authored
// (ADR-0125) — cannot appear in it, and neither can an authored one go missing.
// There is nothing here to keep in step with the world.
//
// EVERY ROW IS KEYED BY ONE NAME AND THE ROW INDEX, which is what `index` on a
// widget is for (ui/widgets.h): one name for every button in the loop would make
// the whole list one button sharing one highlight. The row's label points into
// the table, which outlives the frame — a name is 64 bytes in the component and
// never a pointer.
static void scene_panel(voe_ui_context *ui, voe_editor_scene *scene)
{
	const voe_scene_identity *rows;
	const voe_ecs_entity *entities;
	uint32_t count;

	voe_ui_label(ui, "Scene");

	count = voe_scene_identity_count(scene->world);
	rows = voe_scene_identity_rows(scene->world);
	entities = voe_scene_identity_entities(scene->world);

	for (uint32_t i = 0; i < count; i++) {
		voe_ui_node row = voe_ui_button_begin(ui, "entity", i);

		if (voe_editor_scene_is_selected(scene, entities[i]))
			voe_ui_label(ui, SELECTED_MARK);
		voe_ui_label(ui, rows[i].name);
		voe_ui_end(ui);

		// The click is answered after voe_ui_frame_end and this
		// function has to have returned by then, so the node is handed
		// to the scene to be asked later. See scene.h.
		voe_editor_scene_row_add(scene, row, entities[i]);
	}
}

// WHAT IS ON THE INSPECTOR IS ONE CALL AND NOT A SECOND SCENE PANEL. It is
// handed the world and the selection and nothing else, because what it lists is
// the world's own component types and not anything this folder knows the name of
// — see inspector.h.
static void inspector_panel(voe_ui_context *ui, voe_editor_scene *scene)
{
	voe_editor_inspector_draw(ui, &scene->inspector, scene->world,
				  voe_editor_scene_selected(scene));
}

// ONE PICTURE, THE WHOLE OF THE VIEW'S TEXTURE, GROWING TO FILL THE PANEL. The
// panel stretches it across and it grows along, so its rectangle is the panel's;
// the node is handed to the view to be asked where it sat once the frame has
// ended, which is the size the view's target is drawn at next frame (view.h).
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
}

void voe_editor_panel_draw(voe_ui_context *ui, voe_editor_panel panel,
			   uint32_t view, voe_editor_scene *scene,
			   voe_editor_views *views)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a panel into no interface");
	VOE_BASE_ASSERT(scene != NULL, "drawing a panel with no scene");
	VOE_BASE_ASSERT(scene->world != NULL,
			"drawing a panel onto a scene with no world");
	VOE_BASE_ASSERT(views != NULL, "drawing a panel with no views");

	switch (panel) {
	case VOE_EDITOR_PANEL_SCENE:
		scene_panel(ui, scene);
		return;
	case VOE_EDITOR_PANEL_INSPECTOR:
		inspector_panel(ui, scene);
		return;
	case VOE_EDITOR_PANEL_SCENE_VIEW:
		scene_view_panel(ui, view, views);
		return;
	case VOE_EDITOR_PANEL_COUNT:
		break;
	}

	VOE_BASE_ASSERT(false, "drawing a panel that is not one");
}
