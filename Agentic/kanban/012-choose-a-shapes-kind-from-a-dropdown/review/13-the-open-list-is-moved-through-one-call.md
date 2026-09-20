# 13 — The open list is moved through one call
folder: editor
decisions: 0168, 0195, 0198, 0199

## Change
`editor/src/scene.h` and `editor/src/scene.c` hold the open list's target, so they are where its place is set
too — the panel works it out, this file is the one that writes it.

- `voe_editor_scene_dropdown_showing` goes, from both files. Nothing has called it since card 12: what it did
  before a frame — close the list when its entity is gone, is no longer the selection or no longer has the
  row — is now the Inspector's, because a list whose field is not on the panel draws no rows at all and is
  closed after the frame (card 14, inspector_edit.h). `voe_editor_scene_dropdown_open` and `_close` stay
  exactly as they are, the picker's mutual exclusion included.
- One call arrives in their place, declared beside `_close` and written beside it:

		// Moves the open list to `left`, `top` — millimetres inside the
		// Inspector's content column (inspector.h). An overlay is
		// positioned from the widget it opened from, every frame it is
		// open and never once when it opened (ADR-0199), and the panel
		// is the only thing that knows where its button sits; so
		// inspector_edit.h measures it and this is where it lands.
		// Does nothing while the list is closed.
		void voe_editor_scene_dropdown_place(voe_editor_scene *scene,
						     float left, float top);

- `scene.h`'s paragraph THE OPEN DROPDOWN'S TARGET IS HERE TOO loses the clause about closing it here on the
  next ask, and gains instead that it is closed by `interface.c` on Escape and by `inspector_edit.c` on a
  choice, on a press outside it and on the first frame its field is not on the panel; and that where it sits is
  measured by that same file and set through `_dropdown_place` every frame it is open, because an overlay
  follows its widget rather than remembering where it opened (ADR-0199).

`editor/src/src.md`: the `scene.h` and `scene.c` entries say the place is set through one call and that the
showing question is gone; the `inspector.h`, `inspector.c` and `interface.c` entries say what card 12 made
true — the Inspector draws the open list as an anchored child of its own content column, and `interface.c`
only closes it on Escape.

## Done when
`checks.sh --folder editor` exits 0, `cmake --build --preset debug` builds the whole tree, and `grep -rn
dropdown_showing editor` prints nothing.
