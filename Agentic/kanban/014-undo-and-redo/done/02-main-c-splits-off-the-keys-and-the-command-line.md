# 02 — main.c splits off the keys and the command line
folder: editor
decisions: 0168

## Change
`editor/src/main.c` is 1017 lines and every card below adds to it. This card takes three things out of it and
changes nothing about what the program does. Read main.c's own header; each part takes its paragraphs with it.

`editor/src/keys.h` and `keys.c` — new. This frame's keyboard, read once:

```c
typedef struct { bool was_down[VOE_PLATFORM_KEY_COUNT]; } voe_editor_keys;
typedef struct {
	bool down[VOE_PLATFORM_KEY_COUNT];
	bool pressed[VOE_PLATFORM_KEY_COUNT];
} voe_editor_keys_frame;

void voe_editor_keys_read(voe_editor_keys *keys, voe_platform_window *window,
			  voe_editor_keys_frame *out);
```

`down` is the level `platform` reports; `pressed` is the down edge against last frame's. A NULL window — a
capture — leaves both false and still clears `was_down`. The header makes these points: every key edge in
this program is found here and nowhere else; an edge is tracked whether or not the caller may act on it, so a
shortcut held through the browser opening and closing does not fire the moment it is let through; a key is a
place and never a letter (`platform/input.h`); what an edge means — which shortcut, which guard, whether the
modifier is folded in — stays the caller's, because the guards differ per shortcut.

In main.c: the eight `*_was_down` locals and the three blocks that computed edges (Ctrl+N/O/S, Delete and
Ctrl+D, Escape/Backspace/Enter/Tab) go; one `voe_editor_keys_read` beside the other window reads takes their
place, and `control`, `shift`, `escape_fired` and the rest are read out of the frame struct. All the routing —
the browser and `voe_ui_typing` guards, Escape going to the picker then Preferences, the `voe_ui_keyboard`
handed to the interface — stays exactly where it is and keeps its comments.

`editor/src/options.h` and `options.c` — new: the command line.

```c
typedef struct {
	const char *folder;   // the one bare argument, or NULL
	const char *capture;  // --capture's path, or NULL
	int wide;             // --size's, or what the caller put here
	int high;
} voe_editor_options;

bool voe_editor_options_read(int argc, char *argv[], voe_editor_options *out);
```

`*out` arrives holding the default size and is left holding it when `--size` is absent. False means the one
usage line has been printed on stderr and the caller returns 2. `number` and `usage` move here whole, with
main.c's paragraphs on `--capture`, `--size` and the DEVIATION note about `char *argv[]`; main.c's header
keeps one sentence saying a capture is started from the command line and names options.h.

`editor/src/view.h` and `view.c`: `world_light` becomes `voe_editor_view_light(const voe_ecs_world *world)`
and `outline_colour` becomes `voe_editor_view_outline_colour(const voe_ui_theme *palette)`, comments carried
over; main.c calls both. They are what a view is drawn with, which is this file's subject.

`editor/src/src.md` gains the four new files; `view.h`'s and `view.c`'s entries gain the light and the
outline's colour.

## Done when
`checks.sh --folder editor` exits 0; `wc -l editor/src/main.c` is under 800; `voe_editor --capture
<scratch>/f.png --size 800x600` writes a picture, `--size 800x600` alone prints the usage line and returns 2,
and at a running `voe_editor` Ctrl+N, Ctrl+O, Ctrl+S, Delete, Ctrl+D, Escape, Backspace, Enter and Tab each do
what they did before this card.
