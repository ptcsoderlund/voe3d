// The walk over the world's component types, the row per described field, and
// the record each control leaves behind for inspector_edit.c to read. See the
// header for why this file names no component; the replace intent a moved
// control becomes is inspector_edit.c's.
//
// IT IS DRIVEN BY base/describe.h AND BY NOTHING ELSE. A field's kind decides
// which control it gets and a field's offset decides where the bytes go; there
// is no table in here mapping a component to a layout, and adding one would be
// the thing the description exists to make unnecessary. The one switch over the
// kinds is deliberately exhaustive rather than defaulted, so a kind added to
// base is a build error here rather than a field that quietly draws nothing.
//
// A LABEL'S TEXT IS FORMATTED INTO THE FRAME'S ARENA AND NEVER ONTO A STACK.
// voe_ui_label does not copy: the string is read at voe_ui_frame_end, long after
// every function in this file has returned, so a buffer on the stack would be a
// label pointing at whatever the next call put there. `text`, which is
// inspector_value.h's, is the only way a number reaches the screen from here.
//
// THE THREE ANGLES ARE SHOWN AND NEVER STORED. A rotation is a quaternion in the
// component and a quaternion after the edit; the Z-Y-X decomposition exists for
// the length of one frame so that a person has three numbers to drag, and what
// the drag submits is the DIFFERENCE turned back into a rotation about a world
// axis. Keeping the angles instead is the bug this shape exists to prevent — two
// sources of one truth, which disagree the moment anything else writes the
// rotation.
//
// EVERY ROW FILLS THE COLUMN AND WRAPS (ADR-0153 point 11). A component's panel
// stretches its rows to the column's width and each row breaks onto further
// lines, so a narrow Inspector puts a field's number boxes under its label
// instead of past the column's edge. That needs a width from outside — see
// ui/layout.h, A RUN THAT WRAPS — which is what the ACROSS_FILL on the panel
// below and on the scroll area the leaf puts round it are both for. A single box
// wider than the whole column still has its line to itself at its full size, and
// the scroll area is what reaches it.
#include "inspector.h"

#include "entity_field.h"
#include "inspector_value.h"
#include "themes.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/quat.h>

#include <scene/camera_component.h>
#include <scene/prefab_component.h>

#include <ui/colour.h>
#include <ui/widgets.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// A component's fields sit on the theme's RAISED surface, a shade above the
// panel's own SURFACE, so that two components read as two blocks.

// Inside a component's four edges, between its rows, and between the things on
// one row. Millimetres.
#define COMPONENT_PAD (2.0f * VOE_EDITOR_SPACING)
#define COMPONENT_GAP (1.5f * VOE_EDITOR_SPACING)
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// Round the open list's rows and between them. Millimetres.
#define LIST_PAD (1.0f * VOE_EDITOR_SPACING)

// The thickness a Y scrollbar lies over the content with — the right padding
// ui's own example gives such an area (ui/widgets.h, THE SCROLL AREA). Kept off
// the rows only while the list is capped, since that is the only time a bar
// shows. Millimetres.
#define LIST_BAR 3.5f

// Between the sections of the content column. It is the gap dock.c's scroll
// area gives a panel's children (its PANEL_GAP): that column is the one child
// the area holds now, so the space between the sections is this column's to
// declare. Millimetres.
#define CONTENT_GAP (2.0f * VOE_EDITOR_SPACING)

// What one millimetre of horizontal drag is worth, by what the control writes.
// A real number moves in hundredths, a whole number in halves — so a millimetre
// is not quite a step and the dead zone still decides whether the drag began —
// and an angle in whole degrees.
#define PER_MM_REAL 0.01
#define PER_MM_WHOLE 0.5
#define PER_MM_DEGREE 1.0

// The word an empty panel says. A literal, because a label's text is read
// after this file has returned and a literal is still there then.
#define NOTHING_TEXT "Nothing selected"

#define DEGREES_PER_RADIAN 57.29577951308232

// A colour's swatch, in millimetres: wider than tall, about a line of text high.
#define SWATCH_WIDE 8.0f
#define SWATCH_HIGH 3.0f

// ---------------------------------------------------------- what it draws

static double per_millimetre(voe_base_field_kind writes)
{
	if (is_signed(writes) || is_unsigned(writes))
		return PER_MM_WHOLE;
	if (writes == VOE_BASE_FIELD_QUAT)
		return PER_MM_DEGREE;

	return PER_MM_REAL;
}

static bool room(const voe_editor_inspector *inspector, uint32_t wanted)
{
	return inspector->control_count + wanted <=
	       VOE_EDITOR_INSPECTOR_CONTROLS;
}

static void record(voe_editor_inspector *inspector,
		   voe_editor_inspector_control control)
{
	VOE_BASE_ASSERT(room(inspector, 1),
			"recording a control the inspector has no room for — field_row asks before it draws");

	inspector->controls[inspector->control_count++] = control;
}

// One number box, its figure inside it, and the record the read finds it again
// by. `name` and `lane` are the key: a field's name cannot repeat inside one
// component, so the pair is unique under the component's own panel.
static void number_box(voe_ui_context *ui, voe_editor_inspector *inspector,
		       voe_ecs_type type, const char *name, uint32_t lane,
		       size_t offset, voe_base_field_kind writes, double value,
		       const char *figure)
{
	voe_ui_node node = voe_ui_number_begin(ui, name, lane, value,
					       per_millimetre(writes));

	voe_ui_label(ui, figure);
	voe_ui_end(ui);

	record(inspector, (voe_editor_inspector_control){
				  .node = node,
				  .type = type,
				  .offset = offset,
				  .writes = writes,
				  .axis = lane,
				  .shown = value });
}

// The three rows a rotation is: the field's name, then an angle in degrees per
// world axis. The angle is this frame's decomposition and the record keeps it,
// because what the read submits is the difference between what the box came
// back with and what it was showing.
static void rotation_rows(voe_ui_context *ui, voe_editor_inspector *inspector,
			  voe_ecs_type type,
			  const voe_base_field_description *field,
			  const uint8_t *bytes)
{
	voe_math_quat rotation;
	voe_math_float3 angles;

	memcpy(&rotation, bytes, sizeof rotation);
	angles = shown_angles(rotation);

	voe_ui_label(ui, field->name);

	for (uint32_t axis = 0; axis < 3; axis++) {
		double degrees = (double)angle_of(angles, axis) *
				 DEGREES_PER_RADIAN;

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, axis_name(axis));
		number_box(ui, inspector, type, field->name, axis,
			   field->offset, VOE_BASE_FIELD_QUAT, degrees,
			   text(inspector->arena, "%.3f", degrees));
		voe_ui_end(ui);
	}
}

// One field: its name, then whatever it gets. A label when the description says
// read-only, when the kind has no control, or when the component has no intent
// to replace a row through — see the header on why a dead control is worse than
// a number. `description` is the walk's, for the names a field's values may have.
static void field_row(voe_ui_context *ui, voe_editor_inspector *inspector,
		      const voe_ecs_world *world, voe_ecs_type type,
		      bool editable,
		      const voe_base_struct_description *description,
		      const voe_base_field_description *field,
		      const uint8_t *row)
{
	const uint8_t *bytes = row + field->offset;
	bool text_box = field->kind == VOE_BASE_FIELD_CHAR && field->rank == 1;
	bool colour = field->kind == VOE_BASE_FIELD_COLOUR;
	const voe_base_field_names *names =
		field->kind == VOE_BASE_FIELD_UINT32 && field->rank == 0
			? voe_base_names_find(description, field->name)
			: NULL;
	bool entity = field->kind == VOE_BASE_FIELD_ENTITY && field->rank == 0;
	uint32_t boxes = text_box || colour || entity ? 1 : lanes(field->kind);
	bool shown_only = !editable || field->read_only || boxes == 0 ||
			  !room(inspector, boxes);
	voe_ui_sizing swatch = { .along = { VOE_UI_SIZE_FIXED, SWATCH_WIDE },
				 .across = { VOE_UI_SIZE_FIXED, SWATCH_HIGH } };
	voe_math_float3 linear;

	// A COLOUR IS A SWATCH AND NEVER THREE NUMBERS (card 16): inside a
	// button when it can be replaced, which opens the picker once the
	// frame has ended (voe_editor_inspector_buttons_read), and bare when
	// it cannot.
	if (colour) {
		voe_ui_node node = VOE_UI_NODE_NONE;

		memcpy(&linear, bytes, sizeof linear);
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		if (!shown_only)
			node = voe_ui_button_begin(ui, field->name, 0);
		voe_ui_swatch(ui, linear, swatch);
		if (!shown_only)
			voe_ui_end(ui);
		voe_ui_end(ui);

		if (!shown_only)
			record(inspector, (voe_editor_inspector_control){
						  .node = node,
						  .type = type,
						  .offset = field->offset,
						  .writes = VOE_BASE_FIELD_COLOUR });
		return;
	}

	// A NAMED FIELD IS A DROPDOWN AND NEVER A NUMBER (ADR-0198): the name
	// of the value it holds, in a button that only opens the list once the
	// frame has ended when it can be replaced, and a label when it cannot.
	// A value no entry names is the number it is. An ENTITY is the same
	// button, shown by the name of what it points at (entity_field.h).
	if (names != NULL || entity) {
		voe_ui_node node = VOE_UI_NODE_NONE;
		uint64_t value = entity ? 0 : whole_unsigned(field->kind, bytes);
		const char *shown =
			names != NULL && value < names->value_count &&
					names->values[value] != NULL
				? text(inspector->arena, "%s",
				       names->values[value])
				: value_text(inspector->arena, world, field,
					     bytes);

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		if (!shown_only)
			node = voe_ui_button_begin(ui, field->name, 0);
		voe_ui_label(ui, shown);
		if (!shown_only)
			voe_ui_end(ui);
		voe_ui_end(ui);

		if (!shown_only)
			record(inspector, (voe_editor_inspector_control){
						  .node = node,
						  .type = type,
						  .offset = field->offset,
						  .writes = entity
								    ? VOE_BASE_FIELD_ENTITY
								    : VOE_BASE_FIELD_UINT32,
						  .names = names });
		return;
	}

	if (shown_only) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		voe_ui_label(ui, value_text(inspector->arena, world, field,
					    bytes));
		voe_ui_end(ui);
		return;
	}

	if (text_box) {
		voe_ui_node node;

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		node = voe_ui_field(ui, field->name, 0,
				    chars(inspector->arena, field->size, bytes),
				    (voe_ui_sizing){
					    .along = { VOE_UI_SIZE_GROW, 1.0f } });
		voe_ui_end(ui);

		record(inspector, (voe_editor_inspector_control){
					  .node = node,
					  .type = type,
					  .offset = field->offset,
					  .writes = VOE_BASE_FIELD_CHAR,
					  .size = field->size });
		return;
	}

	if (field->kind == VOE_BASE_FIELD_QUAT) {
		rotation_rows(ui, inspector, type, field, bytes);
		return;
	}

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, field->name);

	if (field->kind == VOE_BASE_FIELD_BOOL) {
		voe_ui_node node = voe_ui_button_begin(ui, field->name, 0);

		voe_ui_label(ui, bytes[0] != 0 ? TRUE_TEXT : FALSE_TEXT);
		voe_ui_end(ui);

		record(inspector, (voe_editor_inspector_control){
					  .node = node,
					  .type = type,
					  .offset = field->offset,
					  .writes = VOE_BASE_FIELD_BOOL });
	} else if (is_vector(field->kind)) {
		// A DOUBLE3 is a world position (ADR-0250): each lane is read
		// and written back as a double, so a typed 100000.001 stays so.
		bool wide = field->kind == VOE_BASE_FIELD_DOUBLE3;

		for (uint32_t lane = 0; lane < boxes; lane++) {
			double value = wide ? real64_at(bytes, lane) :
					      (double)real32_at(bytes, lane);

			number_box(ui, inspector, type, field->name, lane,
				   field->offset +
					   (size_t)lane * (wide ? sizeof(double) :
								  sizeof(float)),
				   wide ? VOE_BASE_FIELD_FLOAT64 :
					  VOE_BASE_FIELD_FLOAT32,
				   value, text(inspector->arena, "%.3f", value));
		}
	} else {
		double value = dragged(field->kind, bytes);
		const char *figure;

		if (is_signed(field->kind))
			figure = text(inspector->arena, "%" PRId64,
				      whole_signed(field->kind, bytes));
		else if (is_unsigned(field->kind))
			figure = text(inspector->arena, "%" PRIu64,
				      whole_unsigned(field->kind, bytes));
		else
			figure = text(inspector->arena, "%.3f", value);

		number_box(ui, inspector, type, field->name, 0, field->offset,
			   field->kind, value, figure);
	}

	voe_ui_end(ui);
}

// One component: its heading with a Remove button beside it — none for a
// kept type — the line saying what it needs when the entity lacks that, and a row
// per described field. A panel and not a column because the key is what makes
// every widget beneath it unique — two components with a field of the same name
// would otherwise be one widget sharing one highlight (ui/widgets.h).
static void component_panel(voe_ui_context *ui,
			    voe_editor_inspector *inspector,
			    voe_ecs_world *world, uint32_t index,
			    voe_ecs_type type, bool removable, bool part,
			    const uint8_t *row)
{
	const voe_base_struct_description *description;
	bool editable = !part && voe_ecs_component_replace(world, type).set;
	voe_ecs_type needed;

	voe_ui_panel_begin(ui, "component", index, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = COMPONENT_GAP,
				   .pad = { COMPONENT_PAD, COMPONENT_PAD,
					    COMPONENT_PAD, COMPONENT_PAD } });

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, heading(inspector->arena, world, type));
	if (removable && !part &&
	    inspector->remove_count < VOE_EDITOR_INSPECTOR_SECTIONS) {
		voe_ui_node node = voe_ui_button_begin(ui, "remove", 0);

		voe_ui_label(ui, "Remove");
		voe_ui_end(ui);
		inspector->removes[inspector->remove_count++] =
			(voe_editor_inspector_type_button){ .node = node,
							    .type = type };
	}
	voe_ui_end(ui);

	if (voe_ecs_component_needs(world, type, &needed) &&
	    voe_ecs_component_get(world, needed, inspector->entity) == NULL)
		voe_ui_label(ui, text(inspector->arena, "Needs %s",
				      heading(inspector->arena, world,
					      needed)));

	description = voe_ecs_component_description(world, type);
	if (description != NULL)
		for (uint32_t i = 0; i < description->field_count; i++)
			field_row(ui, inspector, world, type, editable,
				  description,
				  &description->fields[i], row);

	voe_ui_end(ui);
}

// Add component's one button, and its menu built while the list is open on the
// entity this frame draws.
static void add_component(voe_ui_context *ui, voe_editor_inspector *inspector,
			  voe_ecs_world *world)
{
	inspector->add_component = voe_ui_button_begin(ui, "add component", 0);
	voe_ui_label(ui, "Add component");
	voe_ui_end(ui);

	if (inspector->adding &&
	    inspector->adding_for.index == inspector->entity.index &&
	    inspector->adding_for.generation == inspector->entity.generation)
		voe_editor_add_menu_build(&inspector->menu, world,
					  inspector->entity);
}

// Add component's lists: the top one under its button, then one per open
// group beside its row. A level whose group is no longer a group under the
// one open above it — the menu is rebuilt every frame — ends the drawing
// there, so a submenu never shows another group's children.
static void add_menu_lists(voe_ui_context *ui, voe_editor_inspector *inspector)
{
	uint32_t parent = VOE_EDITOR_ADD_MENU_TOP;
	voe_editor_inspector_place at = inspector->adding_at;

	VOE_BASE_ASSERT(inspector->menu.count > 0, "drawing an empty menu");
	VOE_BASE_ASSERT(inspector->open_count < VOE_EDITOR_ADD_MENU_DEPTH,
			"more groups open than a path is deep");

	for (uint32_t level = 0; level <= inspector->open_count; level++) {
		if (level > 0) {
			const uint32_t group = inspector->open_groups[level - 1];

			if (group >= inspector->menu.count ||
			    !inspector->menu.entries[group].group ||
			    inspector->menu.entries[group].parent != parent)
				return;
			parent = group;
			at = inspector->open_at[level - 1];
		}
		voe_editor_add_menu_draw(ui, inspector->arena, &inspector->menu,
					 parent, at.left, at.top, at.height,
					 &inspector->menu_lists[level]);
	}
}

// An ENTITY field's rows: None and every authored entity by its label
// (entity_field.h), each row carrying its entity, the one `held` points at
// marked by inversion as a named value is. A dead `held` marks None.
static void entity_rows(voe_ui_context *ui, voe_editor_inspector *inspector,
			const voe_ecs_world *world, voe_ecs_entity held)
{
	voe_ecs_entity choices[VOE_EDITOR_DROPDOWN_ROWS];
	uint32_t count = voe_editor_entity_field_choices(
		world, choices, VOE_EDITOR_DROPDOWN_ROWS);

	VOE_BASE_ASSERT(inspector->row_count == 0,
			"entity rows drawn after other rows");

	if (!voe_ecs_entity_alive(world, held))
		held = (voe_ecs_entity){ 0 };
	for (uint32_t i = 0; i < count; i++) {
		inspector->rows[i].node = voe_ui_choice_begin(
			ui, "kind", i,
			choices[i].index == held.index &&
				choices[i].generation == held.generation);
		inspector->rows[i].entity = choices[i];
		voe_ui_label(ui, voe_editor_entity_field_label(
					 world, choices[i], inspector->arena));
		voe_ui_end(ui);
	}
	inspector->row_count = count;
	VOE_BASE_ASSERT(inspector->row_count <= VOE_EDITOR_DROPDOWN_ROWS,
			"more entity rows than the list holds");
}

// The open list, hanging from the button that opened it inside the content
// column it is anchored to — see the header on why it is this panel's and not
// the editor's. Nothing is drawn unless the list is open on a field of an
// entity this frame drew, and the value in force is read out of that entity's
// row.
static void dropdown_list(voe_ui_context *ui, voe_editor_inspector *inspector,
			  voe_ecs_world *world)
{
	const voe_editor_dropdown *dropdown = &inspector->dropdown;
	const float h = dropdown->height;
	const bool capped = h > 0.0f;
	const uint8_t *row;
	uint32_t value = 0;
	voe_ecs_entity held = { 0 };

	if (!dropdown->open ||
	    (dropdown->names == NULL && !dropdown->entities) ||
	    dropdown->entity.index != inspector->entity.index ||
	    dropdown->entity.generation != inspector->entity.generation)
		return;

	row = voe_ecs_component_get(world, dropdown->type, dropdown->entity);
	if (row == NULL)
		return;
	if (dropdown->entities)
		memcpy(&held, row + dropdown->offset, sizeof held);
	else
		memcpy(&value, row + dropdown->offset, sizeof value);

	// The column round it only carries the anchor, the list being a panel
	// of its own. One row per value the names name, and no row for a value
	// they leave unnamed; the value in force is marked by inversion and
	// nothing else (ADR-0194, ADR-0196).
	voe_ui_column_begin(
		ui, (voe_ui_container){
			    .anchor = { .anchored = true,
					.x = { VOE_UI_ACROSS_START,
					       dropdown->left },
					.y = { VOE_UI_ACROSS_START,
					       dropdown->top } } });
	inspector->list = voe_ui_panel_begin(
		ui, "list", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
				    .pad = { LIST_PAD, LIST_PAD, LIST_PAD,
					     LIST_PAD },
				    .blocks_pointer = true });
	// The rows scroll inside the panel, so a list that had to be capped
	// still reaches its last value (ADR-0200). A NATURAL height is the rows
	// as tall as they come, which is every list nobody has capped.
	inspector->list_rows = voe_ui_scroll_begin(
		ui, "kinds", 0,
		(voe_ui_container){
			.size = { .along = capped
					   ? (voe_ui_size){ VOE_UI_SIZE_FIXED,
							    h }
					   : (voe_ui_size){ 0 } },
			.across = VOE_UI_ACROSS_FILL,
			.gap = LIST_PAD,
			.pad = { .right = capped ? LIST_BAR : 0.0f } },
		(voe_ui_scroll_axes){ .y = true });
	if (dropdown->entities)
		entity_rows(ui, inspector, world, held);
	for (uint32_t i = 0; dropdown->names != NULL &&
			     i < dropdown->names->value_count &&
			     inspector->row_count < VOE_EDITOR_DROPDOWN_ROWS;
	     i++) {
		if (dropdown->names->values[i] == NULL)
			continue;
		inspector->rows[inspector->row_count].node =
			voe_ui_choice_begin(ui, "kind", i, i == value);
		inspector->rows[inspector->row_count].value = i;
		inspector->row_count++;
		voe_ui_label(ui, dropdown->names->values[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
}

// Whether another row `entity` holds names `type` as needed (0302 point 4), so
// that removing `type` would leave that row doing nothing. A runtime-only row
// counts too: it is not shown, but it still needs the place.
static bool needed_by_another(const voe_ecs_world *world,
			      voe_ecs_entity entity, voe_ecs_type type)
{
	uint32_t types = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < types; i++) {
		voe_ecs_type other = voe_ecs_component_type_at(world, i);
		voe_ecs_type needed;

		if (other.value != type.value &&
		    voe_ecs_component_needs(world, other, &needed) &&
		    needed.value == type.value &&
		    voe_ecs_component_get(world, other, entity) != NULL)
			return true;
	}
	return false;
}

// ----------------------------------------------------------- the surface

void voe_editor_inspector_frame_begin(voe_editor_inspector *inspector,
				      voe_base_arena *arena,
				      const voe_editor_dropdown *dropdown)
{
	VOE_BASE_ASSERT(inspector != NULL, "opening a frame on no inspector");
	VOE_BASE_ASSERT(arena != NULL, "an inspector frame with no arena");

	inspector->arena = arena;
	inspector->dropdown = dropdown != NULL ? *dropdown
					       : (voe_editor_dropdown){ 0 };
	inspector->content = VOE_UI_NODE_NONE;
	inspector->area = VOE_UI_NODE_NONE;
	inspector->list = VOE_UI_NODE_NONE;
	inspector->list_rows = VOE_UI_NODE_NONE;
	inspector->row_count = 0;
	inspector->control_count = 0;
	inspector->replaced = 0;
	inspector->entity = (voe_ecs_entity){ 0 };
	inspector->duplicate = VOE_UI_NODE_NONE;
	inspector->remove = VOE_UI_NODE_NONE;
	inspector->remove_count = 0;
	inspector->add_component = VOE_UI_NODE_NONE;
	inspector->menu.count = 0;
	for (uint32_t i = 0; i < VOE_EDITOR_ADD_MENU_DEPTH; i++)
		inspector->menu_lists[i] = (voe_editor_add_menu_list){
			.panel = VOE_UI_NODE_NONE, .area = VOE_UI_NODE_NONE
		};
}

void voe_editor_inspector_area_set(voe_editor_inspector *inspector,
				   voe_ui_node area)
{
	VOE_BASE_ASSERT(inspector != NULL, "handing an area to no inspector");

	inspector->area = area;
}

bool voe_editor_inspector_is_part(const voe_ecs_world *world,
				  voe_ecs_entity entity, voe_ecs_entity *root)
{
	const voe_scene_prefab_part *part;

	VOE_BASE_ASSERT(world != NULL, "asking for a part in no world");

	part = voe_scene_prefab_part_get(world, entity);
	if (part == NULL || (part->instance.index == entity.index &&
			     part->instance.generation == entity.generation))
		return false;
	if (root != NULL)
		*root = part->instance;
	return true;
}

void voe_editor_inspector_draw(voe_ui_context *ui,
			       voe_editor_inspector *inspector,
			       voe_ecs_world *world, voe_ecs_entity selected,
			       const voe_ecs_type *kept, uint32_t kept_count)
{
	uint32_t types;
	voe_ecs_entity root = { 0 };
	bool part;

	VOE_BASE_ASSERT(ui != NULL, "drawing an inspector into no interface");
	VOE_BASE_ASSERT(inspector != NULL, "drawing no inspector");
	VOE_BASE_ASSERT(inspector->arena != NULL,
			"drawing an inspector before its frame was opened");
	VOE_BASE_ASSERT(world != NULL, "drawing an inspector on no world");
	VOE_BASE_ASSERT(kept != NULL || kept_count == 0,
			"kept types counted but not handed in");

	inspector->entity = selected;

	if (!voe_ecs_entity_alive(world, selected)) {
		voe_ui_label(ui, NOTHING_TEXT);
		return;
	}

	// EVERYTHING BELOW SITS IN ONE COLUMN, which is what the open list is
	// anchored to and what the scroll area scrolls and clips — see the
	// header. The gap between the sections is this column's now that the
	// area holds only it.
	inspector->content = voe_ui_column_begin(
		ui, (voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
					.gap = CONTENT_GAP });

	// A part says whose prefab it is and draws no button (inspector.h).
	part = voe_editor_inspector_is_part(world, selected, &root);
	if (part) {
		const voe_scene_prefab *prefab =
			voe_scene_prefab_get(world, root);

		voe_ui_label(ui, text(inspector->arena,
				      "Part of the prefab %s. Open it from the "
				      "Assets panel to change it.",
				      prefab != NULL ? prefab->path : "?"));
	}

	// No Duplicate and no Delete for the camera's entity: the scene's one
	// camera is neither copied nor deleted (ADR-0218). A light's has both:
	// a copy is a second directional light (0357 point 5).
	if (!part && voe_scene_camera_get(world, selected) == NULL) {
		voe_ui_row_begin(ui,
				 (voe_ui_container){ .gap = COMPONENT_GAP });
		inspector->duplicate = voe_ui_button_begin(ui, "duplicate", 0);
		voe_ui_label(ui, "Duplicate");
		voe_ui_end(ui);
		inspector->remove = voe_ui_button_begin(ui, "delete", 0);
		voe_ui_label(ui, "Delete");
		voe_ui_end(ui);
		voe_ui_end(ui);
	}

	// THE WALK, AND THE WHOLE OF WHAT THIS PANEL KNOWS ABOUT COMPONENTS.
	// Every described type the world holds, asked whether this entity has
	// a row of it.
	types = voe_ecs_component_type_count(world);
	for (uint32_t i = 0; i < types; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);
		const void *row = voe_ecs_component_get(world, type, selected);
		bool removable = true;

		if (row == NULL || voe_ecs_component_runtime_only(world, type))
			continue;

		for (uint32_t k = 0; k < kept_count; k++)
			removable = removable && type.value != kept[k].value;
		removable = removable && !needed_by_another(world, selected, type);

		component_panel(ui, inspector, world, i, type, removable, part,
				(const uint8_t *)row);
	}

	if (part) {
		voe_ui_end(ui);
		return;
	}

	add_component(ui, inspector, world);
	// AFTER EVERY SECTION, because submission order is paint order
	// (ui/layout.h) and a list emitted beside its button would be painted
	// over by the rows below it.
	dropdown_list(ui, inspector, world);
	if (inspector->menu.count > 0)
		add_menu_lists(ui, inspector);

	voe_ui_end(ui);
}
