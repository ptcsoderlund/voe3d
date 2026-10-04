# 01 — app reads a PNG file into a texture
folder: app
after: none
decisions: 0168, 0356

## Change
A new public module that reads a PNG file into a colour texture with its size, for the splash.

- `app/include/app/picture.h` (new): `typedef struct { voe_render_texture texture; uint32_t width;
  uint32_t height; } voe_app_picture;` and
  `[[nodiscard]] bool voe_app_picture_read(voe_render_device *device, const char *path,
  voe_base_arena *scratch, voe_app_picture *out, voe_base_error *error)`. Header points: the
  file read whole (platform/file.h), decoded (assets/image.h `voe_assets_png_decode`) and
  uploaded as VOE_RENDER_TEXTURE_COLOUR with clamped sampling (render/device.h
  `voe_render_texture_create`); false with `error` set for a missing file, bytes that are not a
  PNG, or a refused upload, `out` untouched; the decoded pixels and file live in `scratch`,
  which the caller rewinds, and a large picture needs about twice its RGBA size there; a startup
  operation (the upload waits for the GPU); the texture is the caller's, given back with
  `voe_render_texture_destroy`; why it lives in `app` (0356: `game` may not name `assets`).
- `app/src/picture.c` (new): the call.
- `app/tests/picture.c` (new, headless, skips without a graphics card as `app/tests/capture.c`
  does): `reads_a_png` — a 4×2 RGBA picture encoded with `voe_assets_png_encode`, written to a
  file under the system temp folder with `voe_platform_file_write`, read back with width 4,
  height 2 and a texture id that `voe_render_texture_destroy` accepts; `missing_file_fails` —
  a path that does not exist answers false; `not_a_png_fails` — a file of text answers false.
- `app/include/app/app.md`, `app/app.md`, `app/src/src.md`, `app/tests/tests.md`: one entry
  each for the new files.

## Done when
`app/tests/picture.c`'s three cases pass (`ctest --test-dir build/debug -R '^app/'`).
