// One `ui` frame per root, submitted into the open frame and drawn with
// voe_render_element_transform. See the header for why a root is a frame and why
// the pointer arrives already in millimetres.
//
// NOTHING IN HERE DECIDES WHAT IS ON A PANEL. It opens the frame, hands over the
// pointer it was given, asks the dock to walk, and moves records into the
// device; what a panel says is dock.c's `voe_editor_panel_draw`.
//
// IT DOES ASK ONE QUESTION ABOUT WHAT WAS CLICKED, AND THAT IS NOT THE SAME
// THING. A `ui` widget answers what the pointer did to it only after
// voe_ui_frame_end and only while the arena its nodes came out of still holds
// them — a window this function opens and closes. So the one line that reads
// the frame's clicks is here, between the two, and what a click MEANS is
// scene.c's.
#include "interface.h"

#include "inspector.h"

#include <base/assert.h>

voe_ui_context *voe_editor_interface_new(voe_base_arena *arena,
					 const voe_text_font *font)
{
	voe_ui_context *ui;

	VOE_BASE_ASSERT(arena != NULL, "making an interface without an arena");
	VOE_BASE_ASSERT(font != NULL, "an interface with no font");

	ui = voe_ui_context_new(
		arena, (voe_ui_capacities){
			       .nodes = VOE_EDITOR_INTERFACE_NODES,
			       .elements = VOE_EDITOR_INTERFACE_ELEMENTS,
			       .scrolls = VOE_EDITOR_INTERFACE_SCROLLS });
	voe_ui_font_set(ui, font);

	return ui;
}

void voe_editor_interface_surface(voe_platform_size target,
				  voe_math_float2 *millimetres,
				  float *pixels_per_millimetre)
{
	float scale;

	VOE_BASE_ASSERT(millimetres != NULL,
			"asking how big the surface is with nowhere to put the answer");
	VOE_BASE_ASSERT(pixels_per_millimetre != NULL,
			"asking what a millimetre is worth with nowhere to put the answer");
	VOE_BASE_ASSERT(target.width > 0 && target.height > 0,
			"a surface on a window with no area");

	scale = (float)target.height / VOE_EDITOR_SURFACE_HIGH *
		VOE_EDITOR_UI_SCALE;

	*millimetres = voe_render_element_surface_size(target, scale);
	*pixels_per_millimetre = scale;
}

bool voe_editor_interface_draw(voe_render_device *gpu, voe_ui_context *ui,
			       voe_base_arena *arena,
			       const voe_editor_dock_root *roots,
			       uint32_t count, voe_editor_scene *scene,
			       voe_editor_views *views)
{
	struct voe_base_arena_mark mark;
	bool ok = true;

	VOE_BASE_ASSERT(gpu != NULL, "drawing the interface on no device");
	VOE_BASE_ASSERT(ui != NULL, "drawing no interface");
	VOE_BASE_ASSERT(arena != NULL, "drawing the interface without an arena");
	VOE_BASE_ASSERT(roots != NULL, "drawing an interface with no roots");
	VOE_BASE_ASSERT(scene != NULL, "drawing an interface with no scene");
	VOE_BASE_ASSERT(views != NULL, "drawing an interface with no views");

	for (uint32_t i = 0; i < count && ok; i++) {
		const voe_editor_dock_root *root = &roots[i];
		uint32_t first;
		uint32_t records;

		// The tree lives in the arena only until its records have been
		// read out of it, which is before the next root is walked.
		mark = voe_base_arena_mark(arena);

		voe_ui_frame_begin(ui, arena);
		voe_ui_pointer_set(ui, root->pointer);
		// The inspector formats every label it draws into this arena
		// and hands back its controls through nodes out of this frame,
		// so it is opened here beside the frame and not inside the walk.
		voe_editor_inspector_frame_begin(&scene->inspector, arena);
		voe_editor_dock_walk(root, ui, scene, views);

		if (!voe_ui_frame_end(ui)) {
			voe_base_arena_rewind(arena, mark);
			return false;
		}

		// Before the rewind below, which is the whole of the window a
		// widget will answer in. A refused frame above is not asked at
		// all: nothing was laid out, so nothing was clicked.
		//
		// The edits go first, and which order they are in is a fact and
		// not a taste: a click read here can move the selection, and the
		// controls above were drawn for whatever was selected when the
		// frame was built. The inspector keeps that entity itself, so
		// the order is belt as well as braces.
		voe_editor_inspector_edits_read(&scene->inspector, ui,
						scene->world);
		voe_editor_scene_clicks_read(scene, ui);
		voe_editor_views_rects_read(views, ui);

		// The range this root fills, read either side of its own
		// submissions: there is no id and nothing allocated, and
		// another root's records are another range of the same buffer.
		first = voe_render_frame_elements_submitted(gpu);
		records = voe_ui_element_count(ui);
		for (uint32_t e = 0; e < records && ok; e++)
			ok = voe_render_frame_submit_element(
				gpu, voe_ui_element(ui, e));

		// ONE COMMAND FOR THE WHOLE ROOT, WHATEVER IS ON IT. Not one per
		// panel and not one per letter: every record in the range is the
		// same eighty bytes in the same buffer.
		if (ok)
			ok = voe_render_frame_draw_elements(
				gpu, voe_render_element_transform(root->size),
				first, records);

		voe_base_arena_rewind(arena, mark);
	}

	return ok;
}
