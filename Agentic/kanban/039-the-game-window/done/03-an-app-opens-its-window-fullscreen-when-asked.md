# 03 — An app opens its window fullscreen when asked
folder: app
after: 02
decisions: 0168, 0291

## Change
0291 point 2. Card 02 gave `voe_platform_window_new` a `bool fullscreen` before the title.
Files: `app/include/app/app.h`, `app/src/app.c`, `app/app.md` if its `app.h` entry needs it.

- `app.h`: `voe_app_settings` gains `bool fullscreen`, false by a designated initializer's
  zero, so no caller in `dev`, `editor` or `game` changes. Header points: what it asks of the
  window, that the headless startup ignores it, and that the size is then the screen's, read
  from the frame as ever.
- `app.c`: pass `settings.fullscreen` to `voe_platform_window_new`.

## Done when
`cmake --build --preset debug --target voe_app` exits 0 and `ctest --test-dir build/debug -R
'^app/'` passes.
