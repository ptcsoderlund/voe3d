# 06 — What the open dropdown is open on
folder: editor
decisions: 0168, 0192, 0195, 0198

## Change
`editor/src/scene.h`: a second target beside `picking`, for the same reason and in the same shape — it outlives
the frame the control fired in, and the Inspector's own struct forgets its controls every frame.

	// How many of a named field's values the open list shows. A further one
	// gets no row; the shapes' three are what there is today.
	#define VOE_EDITOR_DROPDOWN_ROWS 16

	// What the open dropdown chooses among. Zeroed is a closed one.
	typedef struct {
		bool open;
		voe_ecs_entity entity;
		voe_ecs_type type;
		// Bytes from the start of the row to the field's uint32_t.
		size_t offset;
		// The field's value names (base/describe.h): entry i names
		// value i. A table of the declaring folder's own, static and
		// so outliving every frame.
		const voe_base_field_names *names;
		// Where the control that opened it sat on the surface, in
		// millimetres: the list is anchored to that left edge, just
		// under that bottom.
		float left;
		float top;
	} voe_editor_dropdown;

added to `voe_editor_scene` as `voe_editor_dropdown dropdown;` beside `picking`, with `<base/describe.h>` among
the includes. And four calls, spelled like the picker's:

	void voe_editor_scene_dropdown_open(voe_editor_scene *scene,
					    voe_editor_dropdown dropdown);
	void voe_editor_scene_dropdown_close(voe_editor_scene *scene);
	// Whether the list shows this frame, and the value in its row when it
	// does. Closes it first when its entity is not alive, is no longer the
	// selection or no longer has the row.
	[[nodiscard]] bool
	voe_editor_scene_dropdown_showing(voe_editor_scene *scene,
					  uint32_t *value);

`editor/src/scene.c`: those three, written the way `voe_editor_scene_picker_open`, `_picker_close` and
`_picker_showing` are written directly above them — the same aliveness, selection and row checks, and the value
read as a `uint32_t` copied out of the row's bytes at `offset` instead of a `voe_math_float3`. Opening either of
the two closes the other: `_dropdown_open` closes the picker and `_picker_open` closes the dropdown, so one popup
shows at a time and Escape means one thing.

`editor/src/scene.h`'s prose gets the paragraph that says so: THE OPEN DROPDOWN'S TARGET IS HERE TOO, for the
picker's reason. It names an entity, a component type, the field's offset in the row and the names the field's
values have, never a component (0195); it is opened by the Inspector (inspector_edit.h), closed by interface.c on
Escape or a press outside it, and here, on the next ask, when its entity is gone, no longer selected or without
the row; closing chooses nothing, every choice having been submitted as it was made; and only one of it and the
colour picker is ever open, because opening either closes the other.

`editor/src/src.md`: `scene.h`'s and `scene.c`'s entries say the dropdown's target beside the picker's.

## Done when
`checks.sh` for `editor` exits 0 and `cmake --build --preset debug` builds the whole tree. Nothing calls the
three new functions yet; cards 08 and 09 are their callers.
