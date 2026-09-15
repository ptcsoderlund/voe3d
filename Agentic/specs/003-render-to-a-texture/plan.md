# 003 Render to a texture, and save it — plan

Drawing into a texture already works — `voe_render_target_create` hands back a target and the
texture id that shows its picture, and the editor's scene views use it. What is missing is the way
back: the pixels are on the card and nothing brings them to memory, nothing writes a PNG, nothing
writes a file, and every program needs a window to open a device. So `render` learns to read a
target back (ADR-0156), `assets` learns to encode a PNG and `platform` to write a file, `app` ties
the three into one call and gains a startup with no window (ADR-0157), the editor gains a command
line that captures one frame, and `3d` learns to leave one entity out of a pass so a surface can
show a camera standing in the same world (ADR-0158).

## Decisions

- Reading a target back becomes `render`'s public surface, RGBA8, straight alpha, top row first —
  rule 10's caller now exists, and the format knowledge stays where the format is. Project-wide:
  `Agentic/decisions/0156-reading-a-target-back-is-public-and-a-picture-is-rgba8.md`.
- `platform` writes whole files, `assets` encodes bytes, `render` reads a target, `app` ties them
  in `voe_app_capture_png` and can open with no window. Project-wide:
  `Agentic/decisions/0157-saving-a-picture-is-one-call-in-app-over-three-folders.md`.
- `voe_3d_frame` gains `hidden`, so the pass drawing a target does not draw the surface showing it.
  Project-wide: `Agentic/decisions/0158-a-pass-may-hide-one-entity.md`.
- **The PNG is written with our own DEFLATE, fixed Huffman codes and a greedy match finder**
  (rule 5, ADR-0023). Not stored blocks: an editor screenshot is mostly flat colour, which is
  where matching pays, and a stored-block file is the pixel count plus five bytes per 65 535. Not
  dynamic Huffman: the tables are a second format to get wrong for maybe a fifth more. Our own
  `assets/src/inflate.c` and python3's `zlib` are both oracles for it. Feature-local.
- **The encoder writes colour type 6 (RGBA), 8 bits, no interlace, filter 0 on every row.** One
  path, and the decoder beside it already proves the other filters are readable. Feature-local.
- No new dependency edge anywhere. `app` adds `assets` to its `DEPENDS`, which
  `cmake/voe.cmake` already allows; nothing is added to `base`, `math`, `ecs` or `scene`, which is
  what makes criterion 5 true by construction.
- Criterion 6 holds by construction rather than by comparison: the bytes saved are the bytes of
  the offscreen colour image that a windowed run blits to the window, and task 4's test draws the
  same frame into the window target and into a target of its own and checks the two pictures are
  identical.

## Folders

- `platform/` — changed — adds `include/platform/file.h`: `voe_platform_file_write`.
- `assets/` — changed — adds `voe_assets_png_encode` to `include/assets/image.h`; internal
  `src/deflate.h` (compressor) and `src/png_crc.h`.
- `render/` — changed — adds `voe_render_picture` and `voe_render_target_read` to
  `include/render/device.h`; `vkCmdCopyImageToBuffer` in the loader table.
- `3d/` — changed — `voe_3d_frame` gains `hidden`; no call site breaks.
- `app/` — changed — adds `voe_app_new_headless` and `voe_app_capture_png`; `voe_app_window` may
  now return NULL; `DEPENDS` gains `assets`.
- `editor/` — changed — internal only: a command line, and a one-frame capture path.
- `dev/` — changed — internal only: a second camera's picture on a surface in the world.

## Verification

- `cmake -P check.cmake` — exits zero on Linux (criterion 7). It is what finishes every task, and
  step 2 configuring each folder standalone is also what proves criterion 5: `base`, `math`, `ecs`
  and `scene` are untouched and no new edge exists.
- `ctest --test-dir build/debug -R '^assets/'` — the encoder's output decodes back to the pixels
  that went in, and python3's `zlib` accepts the stream (criterion 2).
- `ctest --test-dir build/debug -R '^render/targets$'` — a target read back is the picture that
  was drawn, in RGBA order, top row first, and the window target and a target of its own drawn the
  same way are byte-for-byte identical (criteria 2, 6).
- `ctest --test-dir build/debug -R '^app/capture$'` — a headless app draws a known frame, saves a
  PNG and reads it back to the colours it drew (criteria 2, 3, 4, 6).
- `ctest --test-dir build/debug -R '^3d/draw_system$'` — a hidden entity is not drawn and every
  other one still is (criterion 1).
- `./build/debug/editor/voe_editor --capture shot.png --size 1280x720` over a terminal with
  `WAYLAND_DISPLAY` unset — exits 0, `shot.png` is 1280×720 and shows the editor (criterion 4).
  Then `python3 -c "import zlib,struct,sys; d=open('shot.png','rb').read(); print(d[:8], struct.unpack('>II', d[16:24]))"` — the signature and the size.
- By hand, the sponsor: run `voe_dev` and watch the surface in the world showing the second
  camera's view change as the scene moves (criterion 1); open a captured PNG in an image viewer
  and compare it with the same frame on screen (criteria 2, 6).
