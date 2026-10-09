# 09 — The interface file is split by what it does
folder: editor
after: none
decisions: 0168

## Change
`editor/src/interface.c` is 840 lines and cards 26, 29 and 30 change it. Split it by function, no
behaviour changed:

- New `editor/src/interface_assets.c` with an internal `editor/src/interface_assets.h` — the four
  Assets helpers now static in `interface.c` (`landscape_made_path`, `assets_request_do`,
  `landscape_open`, `assets_menu_do`), renamed `voe_editor_interface_assets_*`, and the stretch of
  `voe_editor_interface_draw` that carries out the Assets panel's requests, its Delete question and
  its right-button menu after `voe_ui_frame_end`, as one function those helpers serve.
- New `editor/src/interface_read.c` (declared in `interface_assets.h` or its own internal header,
  whichever reads better) — the rest of the read between `voe_ui_frame_end` and the arena's rewind
  that is not the Assets panel's: the top bar's, browser's, Preferences', Project, Errors and Landscape
  panels' commands and the colour picker's result, as one or two functions.
- `editor/src/interface.c` keeps the frame per root, the dock walk, the draws over the dock and the
  calls into the two.

Move the matching paragraphs of `interface.c`'s header comment to the new files' headers; each new
file's header says what it owns and why it is apart. `interface.h` is unchanged. Update
`editor/src/src.md`'s `interface.c` line and add the new files.

## Done when
`wc -l editor/src/interface.c editor/src/interface_assets.c editor/src/interface_read.c` shows each
under 450, and the editor builds.

## Blocked
The split is done: interface.c 425, interface_assets.c 230 and interface_read.c 333 lines, src.md
updated, and all three compile clean with the project's flags (`-fsyntax-only`). But `voe_editor`
cannot build: `3d/src/models_material.c` fails because `voe_assets_material` is defined in both
`assets/include/assets/material.h` and `assets/include/assets/model.h` (card 01's new header against
the old one; card 04, which moves `3d` over, is blocked). Unblocked once that conflict is resolved;
then `checks.sh --folder editor` should be rerun and the card moved to done.
