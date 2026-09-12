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
// THE ROOT IS WRAPPED IN A ROW OF ITS OWN, WHICH IS ONE NODE AND IS NOT
// CEREMONY. `ui`'s frame root must be a row or a column (see ui/layout.h) and a
// leaf emits a panel, so a tree that is a single panel filling the window would
// otherwise be the one shape the walk could not express.
#include "dock.h"

#include "inspector.h"

#include <base/assert.h>

#include <math/float4.h>

#include <scene/identity_component.h>

// The plate every panel is drawn on: dark, linear, and the same for all of them.
// The two regions are told apart by the seam between them and by where their
// headings start, not by being different colours — a colour per panel would be
// this file deciding something nobody has decided.
#define PANEL_RED 0.04f
#define PANEL_GREEN 0.05f
#define PANEL_BLUE 0.07f
#define PANEL_ALPHA 0.95f

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
	case VOE_EDITOR_PANEL_COUNT:
		break;
	}

	VOE_BASE_ASSERT(false, "a dock leaf naming no panel");
	return "";
}

static void walk_node(voe_ui_context *ui, const voe_editor_dock_tree *tree,
		      uint32_t index, voe_editor_dock_axis parent,
		      voe_math_float2 size, uint32_t depth,
		      voe_editor_scene *scene)
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
		voe_ui_panel_begin(
			ui, panel_key(node->panel), 0,
			(voe_math_float4){ PANEL_RED, PANEL_GREEN, PANEL_BLUE,
					   PANEL_ALPHA },
			(voe_ui_container){
				.size = sizing_in(parent, size),
				.across = VOE_UI_ACROSS_START,
				.gap = PANEL_GAP,
				.pad = { PANEL_PAD, PANEL_PAD, PANEL_PAD,
					 PANEL_PAD } });
		voe_editor_panel_draw(ui, node->panel, scene);
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

	walk_node(ui, tree, node->first, node->axis, head, depth + 1, scene);
	walk_node(ui, tree, node->second, node->axis, tail, depth + 1, scene);
	voe_ui_end(ui);
}

voe_editor_dock_tree voe_editor_dock_default(void)
{
	voe_editor_dock_tree tree = { 0 };

	// THE ONE LINE THE CARD'S CLAIM RESTS ON. Turn VOE_EDITOR_DOCK_ROW into
	// VOE_EDITOR_DOCK_COLUMN and the fraction into 0.5 and the two panels
	// are stacked instead of side by side, with nothing else in this folder
	// touched.
	tree.nodes[0] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_SPLIT,
						.axis = VOE_EDITOR_DOCK_ROW,
						.fraction = 0.25,
						.first = 1,
						.second = 2 };
	tree.nodes[1] = (voe_editor_dock_node){ .kind = VOE_EDITOR_DOCK_LEAF,
						.panel = VOE_EDITOR_PANEL_SCENE };
	tree.nodes[2] = (voe_editor_dock_node){
		.kind = VOE_EDITOR_DOCK_LEAF,
		.panel = VOE_EDITOR_PANEL_INSPECTOR
	};
	tree.count = 3;
	tree.root = 0;

	return tree;
}

void voe_editor_dock_walk(const voe_editor_dock_root *root, voe_ui_context *ui,
			  voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(root != NULL, "walking no dock root");
	VOE_BASE_ASSERT(ui != NULL, "walking a dock root into no interface");
	VOE_BASE_ASSERT(root->tree.count > 0, "walking an empty dock tree");
	VOE_BASE_ASSERT(root->size.x > 0.0f && root->size.y > 0.0f,
			"walking a dock root onto a surface with no area");
	VOE_BASE_ASSERT(scene != NULL, "walking a dock root with no scene");

	// Last frame's rows named last frame's nodes and the arena they were in
	// has gone. The panel records this frame's as it draws them.
	voe_editor_scene_rows_clear(scene);

	voe_ui_row_begin(ui, (voe_ui_container){
				     .size = { .along = { VOE_UI_SIZE_FIXED,
							  root->size.x },
					       .across = { VOE_UI_SIZE_FIXED,
							   root->size.y } },
				     .across = VOE_UI_ACROSS_FILL });
	walk_node(ui, &root->tree, root->tree.root, VOE_EDITOR_DOCK_ROW,
		  root->size, 0, scene);
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

void voe_editor_panel_draw(voe_ui_context *ui, voe_editor_panel panel,
			   voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a panel into no interface");
	VOE_BASE_ASSERT(scene != NULL, "drawing a panel with no scene");
	VOE_BASE_ASSERT(scene->world != NULL,
			"drawing a panel onto a scene with no world");

	switch (panel) {
	case VOE_EDITOR_PANEL_SCENE:
		scene_panel(ui, scene);
		return;
	case VOE_EDITOR_PANEL_INSPECTOR:
		inspector_panel(ui, scene);
		return;
	case VOE_EDITOR_PANEL_COUNT:
		break;
	}

	VOE_BASE_ASSERT(false, "drawing a panel that is not one");
}
