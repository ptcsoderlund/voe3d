# 03 — The Import textures question
folder: editor/src
after: 02
decisions: 0168, 0453, 0466

## Change
0466 point 4: the question as a panel of its own, drawn and read the way `levels_ask.h` is. Nothing
opens it yet; card 04 does. Read `editor/src/levels_ask.h` and `editor/src/levels_ask.c` for the
pattern (count buttons with the chosen one lit, the anchored panel, Make and Cancel, the read after
the frame with a press outside a Cancel) and follow it.

- `editor/src/textures_ask.h` (new) —
  - `#define VOE_EDITOR_TEXTURES_ASK_SIZES 4`
  - `voe_editor_textures_ask`, zeroed a question never asked: `bool open`; `path`, relative to
    `Assets/` as the Assets menu hands it, `char[VOE_EDITOR_ASSETS_PATH]` (`assets_panel.h`);
    `uint32_t largest`, the chosen size; the title's kept text; nodes for the panel, the four size
    buttons, Import and Cancel.
  - `voe_editor_textures_ask_answer`: NONE, IMPORT, CANCEL.
  - `void voe_editor_textures_ask_open(voe_editor_textures_ask *, const char *path)` — opens it,
    `largest` 2048, the title naming the file's name.
  - `void voe_editor_textures_ask_close(voe_editor_textures_ask *)`.
  - `void voe_editor_textures_ask_draw(voe_ui_context *, voe_editor_textures_ask *, float top)` —
    centred across the surface `top` below its top: the title, a "Largest size" row of 512, 1024,
    2048 and 4096 with `largest`'s lit, then Import and Cancel. Asserts when not open.
  - `voe_editor_textures_ask_answer voe_editor_textures_ask_read(const voe_ui_context *,
    voe_editor_textures_ask *, bool pointer_down, voe_math_float2 at)` — a fired size sets
    `largest` and is NONE; Import or Cancel fired, or a press outside the panel (Cancel); else NONE.
  - Header points: what it asks and why 2048 every time (0466 point 4); that nothing here imports
    or closes, the caller carries out the answer; the path's room; a usage block as `levels_ask.h`
    has.
- `editor/src/textures_ask.c` (new) — the sizes as one array, the open with its title, the draw,
  the read. Header: what the file holds.
- `editor/src/src.md` — an entry for each new file, after `levels_ask.c`'s.

## Done when
The folder check passes (the new file compiles into the editor).
