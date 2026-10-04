# 05 — The editor starts on the engine's splash
folder: editor
after: 02, 03
decisions: 0168, 0346, 0356

## Change
- `editor/src/splash.h`, `editor/src/splash.c` (new): `[[nodiscard]] bool
  voe_editor_splash_read(voe_render_device *device, voe_base_arena *scratch,
  voe_app_picture *out)` reads `<VOE_TOOLCHAIN_ENGINE>/game/src/splashscreen.png` (the
  generated `toolchain.h`, as `game_tree.c` includes it) with `voe_app_picture_read`;
  false with one stderr line naming the path. Header points: the editor always shows the
  engine's splash, never a project's (0346); read from the engine source at run time so a
  missing copy shows the plain screen without a rebuild (0356); the caller keeps the picture
  for the start and every scene load and gives the texture back before the device closes.
- `editor/src/main.c`: after the interface is made and before `voe_game_starting_prepare`,
  read the splash into `scratch` (a held `voe_app_picture` and a flag); pass it, or NULL when
  the read failed, to the prepare; rewind `scratch` as now. At shutdown, before the device
  closes, destroy the texture when held. Header paragraph "THE START SHOWS A LINE" says the
  line sits in the splash's box. Keep the change small: main.c is near 800 lines.
- `editor/src/src.md`: entries for `splash.h` and `splash.c`; `main.c`'s says splash.

## Done when
`cmake --build --preset debug --target voe_editor` succeeds, and
`build/debug/editor/voe_editor --capture "$(mktemp -d)/shot.png"` writes the picture and exits 0.

Human: How to test steps 1, 2, 3 and 8 of `feature.md`.
