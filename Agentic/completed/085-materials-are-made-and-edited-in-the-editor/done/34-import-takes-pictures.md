# 34 — Import takes pictures
folder: editor
after: 24
decisions: 0168, 0399

## Change
0399 point 8: a texture set comes into Assets by Import, as a model does.

- `editor/src/browser.h` / `browser.c` — IMPORT mode lists and a press imports `.png`, `.jpg` and
  `.jpeg` files beside `.glb`, any case. `voe_editor_browser_names_model` becomes a name for what it
  now answers (e.g. `voe_editor_browser_names_import`) at its call sites in this folder.
- `editor/src/assets_panel.h` / `assets_panel.c` — the import copies whichever of those it is handed
  into the shown folder, as it copies a `.glb`; the header's Import sentence names pictures too.

Update `editor/src/src.md`'s `browser.h` line.

`editor/src/interface.c` was split into `interface_assets.c` and `interface_read.c` (committed) but
never linked: the build failed in `3d` until card 21. If the editor build fails in those three
files, fix it there, behaviour unchanged.

## Done when
`grep -n 'jpeg' editor/src/browser.c` finds the import test, and the editor builds. Human: Import
lists a `.png` and a `.jpg` in a folder and each press copies it into the shown Assets folder.
