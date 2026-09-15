# 003 Render to a texture, and save it — tasks

Every task finishes with `cmake -P check.cmake` exiting zero on Linux (ADR-0130, CLAUDE.md rule 8).
Where a task also names a narrower command, run the narrow one while working and the whole check
before you say done. Say which platform you verified on and what you could not check there.

- [x] 1. `platform/` — write a whole file
  - Change: Decision: `Agentic/decisions/0157-*.md` point 1. `platform` owns files and has had no API for
    them; this is the first, and it stays one call.
    - `include/platform/file.h`, new:
      `[[nodiscard]] bool voe_platform_file_write(const char *path, const uint8_t *bytes, size_t count, voe_base_error *error);`
      Writes `count` bytes to `path`, replacing whatever is there and creating it if it is not.
      The header says: why files are this folder's; that the path is exactly what the caller gave —
      no directory is created, no extension added, nothing is written anywhere else; that a path
      that cannot be opened is `VOE_BASE_ERROR_UNAVAILABLE` and a write that fails part way is
      `VOE_BASE_ERROR_REFUSED`, with the real reason on a reported line (`base/report.h`); that a
      NULL path, NULL bytes or a count of zero is the caller's bug and asserts (rule 13); and that
      reading a file is not here and is written when a loader needs one (rule 10).
    - `src/file_wayland.c` — Linux. `open(O_WRONLY|O_CREAT|O_TRUNC, 0644)`, a write loop that
      handles a short write and `EINTR`, `close` whose result is checked too (a full disk is
      reported by `close` on some filesystems). Its header says why the name says wayland, exactly
      as `src/library_wayland.c`'s does.
    - `src/file_win32.c` — `CreateFileA` with `CREATE_ALWAYS`, a `WriteFile` loop, `CloseHandle`.
      Same failure mapping, `GetLastError` in the reported line.
    - `tests/file.c` — needs no window and no display: bytes written and read back with `fopen`/
      `fread` byte for byte; a longer file overwritten by a shorter one is the shorter length; a
      path inside a directory that does not exist returns false with `VOE_BASE_ERROR_UNAVAILABLE`
      and creates nothing; a one-byte file. Write into the working directory and `remove()` each
      file at the end, pass or fail.
    - `platform/platform.md`: the intro sentence stops saying files do not live here yet, and the
      three new files get their one line each.
  - Covers: 2 (the writing half)
  - Depends on: -
  - Done when: `ctest --test-dir build/debug -R '^platform/file$'` passes and `cmake -P check.cmake`
    exits zero.

- [x] 2. `assets/` — DEFLATE, the other direction
  - Change: A compressor, internal to the folder, so the PNG encoder in task 3 has one. Rule 5 and
    ADR-0023: written here, not fetched. It is the mirror of `src/inflate.c`, which is also the
    test's oracle.
    - `src/deflate.h`, new and internal:

      ```c
      typedef struct {
              uint8_t *bytes;
              size_t count;
      } voe_assets_deflate_result;

      void voe_assets_deflate(voe_base_arena *arena, const uint8_t *data,
                              size_t count, voe_assets_deflate_result *out);
      ```

      It produces **a zlib stream** — the two header bytes, the DEFLATE data, the big-endian
      Adler-32 — because that is what `src/inflate.h` consumes and what a PNG `IDAT` holds. The
      header says: why it cannot fail and therefore returns nothing (the only allocation is from
      the arena, and rule 11 makes that fatal); that the output buffer is pushed at the worst case
      for fixed Huffman (`count + count / 8 + 64` is enough — say why in the header) and the used
      length comes back in `out`; and that a count of zero asserts.
    - `src/deflate.c` — **fixed Huffman codes only**, one block, `BFINAL` set, a bit writer,
      Adler-32, and a greedy match finder: a hash of three bytes into a head/prev chain, window
      32 768, match lengths 3–258, a chain limit named as a constant so the cost is visible. No
      dynamic Huffman and no stored blocks — the plan says why. Its header carries the length and
      distance code tables' derivation in a few lines, and says what a reader has to know to change
      the matcher without changing the format.
    - `tests/deflate.c` — round trips through `../src/inflate.h` (include it by relative path, as
      `tests/inflate.c` does): one byte; 100 000 copies of one byte (long matches); a buffer of
      bytes with no repeats at all (literals only); a repeating 37-byte pattern over 200 kB (the
      chain and the window); data whose match is at the maximum distance 32 768 and one at the
      maximum length 258; a buffer one byte longer than the window. Each case checks the round trip
      is byte for byte and that the compressed form of the repetitive cases is smaller than the
      input — which is the claim that fails if the matcher silently emits only literals.
    - By hand, once, and put the result in your report: write one compressed stream to a file from
      a scratch program and run
      `python3 -c "import zlib,sys; print(len(zlib.decompress(open(sys.argv[1],'rb').read())))" <file>`.
      Our decoder agreeing with our encoder is not proof the format is right; python3's zlib is.
    - `assets/assets.md`: a line for `src/deflate.h`, `src/deflate.c` and `tests/deflate.c`.
  - Covers: 2 (the encoding half, part one)
  - Depends on: -
  - Done when: `ctest --test-dir build/debug -R '^assets/deflate$'` passes and
    `cmake -P check.cmake` exits zero.

- [x] 3. `assets/` — write a PNG
  - Change: The encoder beside the decoder. Decision: `Agentic/decisions/0157-*.md` point 2 — this folder
    encodes bytes and still opens no file.
    - `include/assets/image.h`, added:

      ```c
      typedef struct {
              uint8_t *bytes;
              size_t count;
      } voe_assets_bytes;

      [[nodiscard]] bool voe_assets_png_encode(voe_base_arena *arena,
                                               voe_assets_image image,
                                               voe_assets_bytes *out,
                                               voe_base_error *error);
      ```

      `image.pixels` is RGBA8, `image.width * image.height * 4` bytes, row zero the top — the same
      shape `_decode` hands back, and the header's existing paragraph about nothing turning a
      picture the right way up covers both directions now. The new paragraphs say: what is written
      — colour type 6, 8 bits, no interlace, filter 0 on every row — and that this is one path on
      purpose; that the bytes are the caller's arena's; that the one returned failure is a picture
      whose byte count does not fit a `size_t` (`VOE_BASE_ERROR_REFUSED`) while a width or height
      of zero and a NULL `pixels` assert; and that colour is written exactly as handed over — no
      gamma, no premultiplication, no channel swap (ADR-0156 says who does those).
    - `src/png_crc.h`, new and internal: `uint32_t voe_assets_png_crc(uint32_t seed, const uint8_t *bytes, size_t count);`
      implemented in `src/png.c` where the table already is — one CRC in this folder, not two.
      `src/png.c` changes by that much and no more; its header gains a line saying the CRC is
      shared with the writer.
    - `src/png_write.c`, new: the signature, `IHDR`, one `IDAT` holding the zlib stream from
      `src/deflate.h`, `IEND`; the filtered rows built as `(1 + width * 4) * height` bytes with a
      leading zero per row. Its header says why one `IDAT` and not many, why filter 0 (the plan's
      reason), and where the size of every buffer it pushes comes from.
    - `tests/png.c`, added to the existing file: a 4×3 picture with a different colour in every
      corner, encoded and decoded back byte for byte; a 1×1 picture; a 257×3 picture, whose odd
      width catches a row stride worked out from the wrong number; a picture with alpha 0, 128 and
      255 in it, back unchanged; the first eight bytes are the PNG signature and `IHDR` says the
      width, the height, bit depth 8, colour type 6, and zeroes for the rest.
    - `assets/assets.md`: lines for `src/png_write.c` and `src/png_crc.h`, and `image.h`'s line now
      says both directions.
  - Covers: 2
  - Depends on: 2
  - Done when: `ctest --test-dir build/debug -R '^assets/png$'` passes and `cmake -P check.cmake`
    exits zero.

- [x] 4. `render/` — read a target back
  - Change: Decision: `Agentic/decisions/0156-reading-a-target-back-is-public-and-a-picture-is-rgba8.md`.
    The pixels only exist on the card; this is the way back, and it is public now because a program
    that saves a picture is the caller rule 10 was waiting for.
    - `include/render/device.h`, in the targets section:

      ```c
      typedef struct {
              uint32_t width;
              uint32_t height;
              uint8_t *pixels;   // width * height * 4
      } voe_render_picture;

      [[nodiscard]] bool voe_render_target_read(voe_render_device *device,
                                                voe_render_target target,
                                                voe_base_arena *arena,
                                                voe_render_picture *out,
                                                voe_base_error *error);
      ```

      Header paragraphs, each of them a sentence a caller would otherwise get wrong: RGBA8 in that
      byte order whatever the image's format is, and why the swap is this folder's; row zero is the
      top; straight alpha, un-premultiplied here because ADR-0069 makes the target premultiplied
      and PNG is not, and that alpha 0 comes back as transparent black; the bytes are already
      sRGB-encoded by the format, so nothing applies a gamma and the colours saved are the colours
      drawn; it waits for the card to go idle and is therefore not a per-frame call, in the same
      breath as `voe_render_target_create`; calling it inside an open frame asserts, as does an id
      naming no target or a NULL arena; it reads the slot the frame that ended last drew into, and
      before any frame has ended — or after a resize — the picture is undefined, which is the rule
      targets already have; `VOE_RENDER_TARGET_WINDOW` works here like any other target, which is
      what makes a saved picture and a shown one the same picture; and the one returned failure is
      the card refusing the staging buffer or the copy (`VOE_BASE_ERROR_REFUSED`).
    - `src/loader.h`/`src/loader.c`: `vkCmdCopyImageToBuffer` joins the table — read `loader.h`'s
      header first, and follow whichever of its three passes it belongs in.
    - `src/target.c`: the implementation. A host-visible coherent buffer through `src/buffer.c`,
      a one-shot command buffer, the copy, wait for idle, map, convert BGRA→RGBA and un-premultiply
      into the arena, unmap, destroy the buffer. `render/tests/offscreen.c` does all of this by hand
      already — read it before writing this; it is the shape, not the code to move.
    - `tests/targets.c`, added: a target of its own drawn with a picture whose four quadrants differ,
      read back — the corner pixels are the colours drawn, `pixels[0]` is the red channel (byte
      order), the top-left quadrant is at row zero (orientation), and alpha is 255 where the clear
      shows; **the same draws into `VOE_RENDER_TARGET_WINDOW` on the headless device read back and
      `memcmp`ed against the target's picture — identical, which is acceptance criterion 6**; and a
      read of a target no pass has drawn into still returns true (the picture is undefined, not an
      error). Skips with a named reason where there is no card, as the other headless tests do.
    - `render/render.md` and `render/src/src.md`: `device.h`'s line gains reading a target back;
      `target.c`'s line says it holds the readback. Leave `tests/offscreen.c`'s hand-resolved copy
      alone — ADR-0156 says why it stays.
  - Covers: 2, 3, 6
  - Depends on: -
  - Done when: `ctest --test-dir build/debug -R '^render/targets$'` passes (or skips with a reason
    where there is no card — then say so) and `cmake -P check.cmake` exits zero.

- [x] 5. `app/` — start with no window, and save a picture
  - Change: Decision: `Agentic/decisions/0157-*.md` points 4 and 5. Two calls, and they are what make a
    program able to draw with no display and write what it drew.
    - `CMakeLists.txt`: `voe_module(app DEPENDS render assets platform math base)` — `assets` is an
      edge `cmake/voe.cmake` already allows; nothing else changes.
    - `include/app/app.h`:
      `[[nodiscard]] voe_app *voe_app_new_headless(voe_base_arena *arena, voe_base_arena *scratch, voe_app_settings settings, voe_base_error *error);`
      opens `voe_render_device_new_headless` at `settings.width` by `settings.height` and no window.
      `settings.title` is unused and may be NULL here. The header says: `voe_app_window` returns
      NULL for such an app and that is the honest answer — a program wanting input asks a window it
      did not ask for; `voe_app_frame_open` does not poll, reports the settings' size, is never
      `closing` and never `minimised`, and still ticks the clock; everything else in the loop is
      unchanged, which is the point — one drawing path, with or without a display.
      `[[nodiscard]] bool voe_app_capture_png(voe_app *app, voe_render_target target, voe_base_arena *scratch, const char *path, voe_base_error *error);`
      reads the target back, encodes a PNG and writes it. The header says: it is called between
      frames and never inside one; `scratch` holds the picture and the file's bytes and may be
      rewound the moment it returns; the failure comes straight from whichever of the three steps
      refused, with its own line already on stderr; nothing is written when any earlier step failed;
      and `VOE_RENDER_TARGET_WINDOW` is what a program means by "what I just drew".
    - `src/app.c`: a `window == NULL` branch in `_frame_open` and `_destroy`, the headless startup
      beside the windowed one (share what is shareable; do not copy the struct's fill twice), and
      the three-step capture. Its header keeps saying that what it holds is the order.
    - `tests/capture.c`, new: a headless app at 64×48, one frame drawing element rectangles — red
      left half, blue right half, and something in only the top-left corner so up and down are
      distinguishable — copy the submit-and-draw pattern from `render/tests/elements.c`; capture to
      a file in the working directory; decode it with `voe_assets_png_decode` and check the size is
      64×48, that the left half is red and the right blue, that the corner mark is at the top, and
      that alpha is 255; then a capture to a path in a directory that does not exist returning false
      and writing nothing. `remove()` the file at the end, pass or fail. Skips with a named reason
      where there is no card.
    - `app/app.md`: the intro says a program may open with no window; lines for the two new calls
      and the new test.
  - Covers: 2, 3, 4, 6
  - Depends on: 1, 3, 4
  - Done when: `ctest --test-dir build/debug -R '^app/'` passes (the capture test skipping with a
    reason where there is no card — then say so) and `cmake -P check.cmake` exits zero.

- [ ] 6. `3d/` — a pass may hide one entity
  - Change: Decision: `Agentic/decisions/0158-a-pass-may-hide-one-entity.md`. A surface showing a target
    must not be drawn into the pass that draws that target — Vulkan leaves reading an image while
    writing it undefined, and `render`'s debug check already asserts on it.
    - `include/3d/draw_system.h`: `voe_3d_frame` gains `voe_ecs_entity hidden;` after `light`, with
      a comment saying a zeroed entity means none (a zeroed `voe_ecs_entity` is never live), that it
      is one entity and not a list or a mask and why (rule 10, ADR-0158), and what it is for. The
      header's paragraph on which entities are drawn says the exception in one line, and
      `voe_3d_draw_system_frame` leaves the field zeroed.
    - `src/draw_system.c`: skipped in both tables, in both layers and in both passes — before the
      sort, so a hidden entity does not take a sort slot either.
    - `tests/draw_system.c`, new, headless: two mesh entities and a panel, drawn with `hidden`
      zeroed — `voe_render_frame_draw_count` counts all of them; drawn again with one mesh hidden —
      one fewer, and the picture still holds the other one; a `hidden` naming an entity that is not
      drawable changes nothing; a hidden panel is skipped too. Skips with a named reason where there
      is no card. Its header says why the count is the measurement and what a wrong skip would look
      like.
    - `3d/3d.md`: `draw_system.h`'s line says a pass may hide one entity; a line for the new test.
    - No call site breaks: every `voe_3d_frame` in the tree is a designated initialiser.
  - Covers: 1
  - Depends on: -
  - Done when: `ctest --test-dir build/debug -R '^3d/'` passes (the new test skipping with a reason
    where there is no card — then say so) and `cmake -P check.cmake` exits zero.

- [ ] 7. `editor/` — capture one frame from the command line
  - Change: Acceptance criterion 4: the editor started so that it draws with no window, writes a
    PNG and exits, naming the file and the size. It reaches all of that through `app` and names
    neither `assets` nor `platform`.
    - `src/main.c`: `int main(int argc, char *argv[])` — spelled as an array because C's own
      signature is what it is; say so in one line of the header, which is the rule 6 deviation and
      the only one.
      - `--capture <path>` and `--size <W>x<H>`. `--size` without `--capture` and any unknown
        argument print one usage line on stderr and return 2. Without `--capture` the editor is
        exactly what it is today, and `--size` defaults to the window size it opens with.
      - With `--capture`: `voe_app_new_headless` at that size, the world, the font, the interface
        and the scene built exactly as now, **the loop body run twice**, then
        `voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW, scratch, path, &error)` and return 0,
        or 1 with the line `app` already printed. Twice because a view's target size lags the
        layout by a frame (`src/view.h`) — one frame would capture the views at their opening size.
        Say that in the header.
      - Every read of the window — the pointer, the wheel — is guarded by `window != NULL` and
        reads as zeroed input when there is none. Nothing else in the loop changes shape: same
        passes, same order, same draw calls, so the captured frame is the frame a person sees.
      - The capture scratch is its own arena, destroyed before returning.
    - `editor/editor.md`: the intro says the editor can also be started to write one picture and
      exit; `main.c`'s line says where the command line is read.
  - Covers: 4
  - Depends on: 5
  - Done when: `cmake -P check.cmake` exits zero, and, after
    `cmake --build --preset debug --target voe_editor`, with `WAYLAND_DISPLAY` unset in a terminal,
    `./build/debug/editor/voe_editor --capture /tmp/editor.png --size 1280x720; echo $?` prints 0
    and
    `python3 -c "import struct; d=open('/tmp/editor.png','rb').read(); assert d[:8]==b'\x89PNG\r\n\x1a\n'; w,h=struct.unpack('>II',d[16:24]); print(w,h,len(d)); assert (w,h)==(1280,720)"`
    prints `1280 720` and a length well above a blank file's. Put the picture in your report as a
    path and say what it shows.

- [ ] 8. `dev/` — a camera's picture on a surface in the world
  - Change: Acceptance criterion 1, as the thing the sponsor runs and looks at: a second camera's
    view on a surface standing in `dev`'s world, updating as the world moves.
    - `src/monitor.h`, `src/monitor.c`, new — the pattern is `src/quad.c` and `src/surface.c`
      beside them, and the camera is `editor/src/view.c`'s shape (a camera of its own, not an
      entity: the world has exactly one camera entity and `voe_3d_draw_system_frame` asserts on
      that). It holds: a target and its texture id from `voe_render_target_create` (512×512), a
      camera looking at the world's cubes from somewhere the world's own camera is not, the sun it
      draws with, a quad entity standing in the world with a transform, a mesh and a material whose
      base colour texture is the target's, `base_colour_uv_rect` `{0, 0, 1, 1}` and **unlit true**
      so what shows is the picture and not the picture times a lambert term, and the
      `voe_3d_frame` it hands back with `hidden` set to its own quad entity. Its header says why
      `hidden` is not optional here (ADR-0158, and what the debug assert says when it is forgotten)
      and why the camera is not an entity.
    - `src/main.c`: create it after the world is built; each frame, before the window pass, open a
      pass onto its target with its camera, run `voe_3d_draw_system_run` with its frame, close the
      pass. Its capacities gain one pass, one target, one texture slot, one geometry, one shading
      and one object — bump the named constants, do not guess new numbers into the call. The
      header's list of things to look at gains the monitor: what it should show, and what a
      forgotten `hidden`, a wrong `unlit` or a target nothing drew into look like.
    - `dev/dev.md`: the intro sentence lists the surface showing the second camera's view; a line
      for `src/monitor.c` and `src/monitor.h`.
  - Covers: 1, 3
  - Depends on: 6
  - Done when: `cmake -P check.cmake` exits zero and, after
    `cmake --build --preset debug --target voe_dev`, `./build/debug/dev/voe_dev` opens and the surface
    shows the second camera's view of the world, changing as the world moves — say in your report
    what you saw, and on a machine with no compositor say that and stop there.
