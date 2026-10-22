# 04 — Import textures asks before it writes
folder: editor/src
after: 03
decisions: 0168, 0177, 0453, 0466

## Change
Card 03's question wired in where the levels question is: the Assets menu's Import textures opens
it, and Import runs the import at the chosen size.

- `editor/src/session.h` — beside `levels_asking`, `voe_editor_textures_ask textures_asking`: the
  Import textures question, opened by the Assets menu, drawn by interface.c, answered in
  interface_assets.c.
- `editor/src/interface.c` — a `textures_asking` taken from `session->textures_asking.open` as
  `levels_asking` is, and treated as it is everywhere: closed when the browser shows, closing the
  Assets menu, taking the pointer from the dock below the bar, drawn with
  `voe_editor_textures_ask_draw` after the levels question, and handed to the read below.
- `editor/src/interface_assets.h`, `editor/src/interface_assets.c` —
  `voe_editor_interface_assets_read` gains `bool textures_asking` after `levels_asking`, and its
  header says so. The menu row IMPORT_TEXTURES opens the question on the selected path instead of
  importing. When `textures_asking`, the answer is read after the levels question's: IMPORT runs
  `voe_editor_interface_assets_textures_do` with the path and `largest` (it gains a
  `uint32_t largest` in place of card 02's 2048) and closes; CANCEL closes and writes nothing.
  The file's header says the question in a line.
- `editor/src/frame_commands.c` — Escape cancels an open Import textures question as it does the
  Delete question, before the Scene list's drag; the ESCAPE'S ORDER comment names it.
- `editor/src/interface.h` — a paragraph after the picture rows' counting the question's nodes and
  elements (panel, title, size row and its four buttons with labels, button row, Import and
  Cancel; the title at its kept room) on top as the levels question is, and
  `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` raised by them.
- `editor/src/src.md` — `session.h`, `interface_assets.h` and `interface_assets.c` entries name the
  Import textures question.

## Done when
The folder check passes. The human runs `## How to test` 1–7 of `feature.md`: the question with
2048 lit, PNGs at most 1024 with shape kept, normal bumps right, 4096 giving back the `.glb`'s size,
512 pictures kept at 512, and Cancel writing nothing.
