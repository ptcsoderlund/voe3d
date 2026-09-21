# 01 — main.c splits off this frame's shortcuts
folder: editor
decisions: 0168

## Change
`editor/src/main.c` is 865 lines and card 07 adds to it. This card takes one thing out of it and changes
nothing about what the program does. Read main.c's own header; the paragraphs on the shortcuts and on
Escape's order go with the part that takes them.

`editor/src/shortcuts.h` and `shortcuts.c` — new. What this frame's keyboard asked the editor to do, worked
out once from `keys.h`'s frame and the guards the caller is holding:

```c
typedef struct {
	bool browser_showing;   // browser.showing
	bool typing;            // voe_ui_typing(ui)
	bool picker_open;       // scene.picking.open
	bool dropdown_open;     // scene.dropdown.open
	bool pointer_down;      // the primary button's level
} voe_editor_shortcuts_guards;

typedef struct {
	bool new_project, open, save;
	bool delete_entity, duplicate;
	bool undo, redo;
	bool at_rest;      // no drag, no typing, no browser, no picker, no open list
	bool escape;       // the raw edge, handed to `ui` as this frame's keyboard
	bool escape_free;  // that edge, when nobody is typing
} voe_editor_shortcuts;

voe_editor_shortcuts voe_editor_shortcuts_read(const voe_editor_keys_frame *keys,
					       voe_editor_shortcuts_guards guards);
```

Move into it, unchanged in behaviour and with their comments: the three Ctrl+N/O/S conditions, the Delete and
Ctrl+D block with its `quiet`, the `at_rest` expression and the Ctrl+Z, Ctrl+Shift+Z and Ctrl+Y conditions
built on it, and the two lines that work out `escape_fired` and `escape_free`. Nothing that acts moves:
main.c keeps every `voe_editor_session_do`, the picker close and the Preferences hide that Escape's order
runs through, and keeps its own `escape_free` local so closing the picker can spend the edge. `control` and
`shift` stay read in main.c for the views' drag.

The header makes these points: which key edge means which command is one answer given once, so no two callers
can disagree; a guard is a fact the caller holds and never something this file asks a panel for; why the
shortcut flags and the rest a step is recorded at (`undo.h`) are worked out together; and that acting on a
flag — a command, a panel closing — is the caller's, because this file may not name `session.h` or
`preferences.h`.

`editor/src/src.md` gains the two files.

## Done when
`checks.sh --folder editor` exits 0, `wc -l editor/src/main.c` is under 780, and at a running `voe_editor`
Ctrl+N, Ctrl+O, Ctrl+S, Delete, Ctrl+D, Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y and Escape each do what they did before
this card — Escape still cancelling typing first, then closing the colour picker, then the browser or
Preferences.
