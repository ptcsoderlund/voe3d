# 06 — The selection can be set from outside the Scene panel
folder: editor
decisions: 0168, 0193

## Change
Today the selection only ever moves because a row in `Scene` fired (`voe_editor_scene_clicks_read`) or because
something was added, duplicated or deleted. A click in a scene view is a third door to the same selection, and
it needs one call rather than a second file writing the field.

`editor/src/scene.h`: declare, beside `voe_editor_scene_selected`,

```c
// Selects this entity, or clears the selection when it is zeroed or no longer
// alive. The same selection a row in `Scene` moves (voe_editor_scene_clicks_read)
// and the same one the Inspector shows: there is one, and this is how anything
// that is not the Scene panel moves it.
void voe_editor_scene_select(voe_editor_scene *scene, voe_ecs_entity entity);
```

and add to the header's paragraph about the selection being the editor's own that a click in a scene view
(pick.h, card 08) moves it through here, so that what is selected stays one field with one meaning whichever
of the two was clicked.

`editor/src/scene.c`: the call. A zeroed entity, or one `voe_ecs_entity_alive` says is gone, clears `selected`;
anything else lands in it. Nothing else changes — no structural count, no notice: selecting is not an edit.

`editor/src/src.md`'s `scene.h` line says the selection is moved by the Scene panel's rows and by a click in a
view.

## Done when
`checks.sh editor` exits 0 and `cmake --build --preset debug` builds the tree; the call is used by card 08.
