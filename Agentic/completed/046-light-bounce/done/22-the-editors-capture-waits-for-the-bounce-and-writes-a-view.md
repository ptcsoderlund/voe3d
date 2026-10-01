# 22 — The editor's capture waits for the bounce and writes a scene view
folder: editor
after: none
decisions: 0168, 0177, 0310

## Change
0310's proof needs the editor's own scene-view picture of a project loaded
from its file, after the bounce grid has filled. `--capture` draws two frames
and writes only the window, whose layout depends on remembered settings; the
grid enters 4096 of its 32³ probes a frame (0308), so two frames show a
quarter of it. Read `editor/src/options.h` / `.c`, `editor/src/capture.h` /
`.c`, `editor/src/main.c` (the capture parts), `editor/src/view.h` (the view
struct's `target`) and `editor/src/src.md`.

- `options.h` / `.c`: two more arguments, each refused like `--size` when
  there is no `--capture`, and a bad or missing value is the usage line:
  - `--frames <n>`: frames drawn before writing; n a whole number ≥ 2,
    default 2. Field `frames`.
  - `--capture-view <path>`: the first scene view's picture is written to
    that path as well. Field `capture_view`, NULL when absent.
  The usage line and the header's form name both.
- `capture.h` / `.c`: `voe_editor_capture_enough` takes the frame count to
  reach instead of the fixed two; a second writer
  `bool voe_editor_capture_write_view(voe_app *app, voe_render_target target, const char *path)`,
  true without a path, writing that target through `voe_app_capture_png`
  between frames. Header points: two frames is still the default and why;
  more frames let the bounce grid fill (0308's rate); the view picture is the
  scene view's own target at its own size, which a measurement can project
  into, unlike the window.
- `main.c`: hands `options.frames` to the count and, after the loop where the
  window is written, writes `views.views[0].target` to `options.capture_view`.
  Its header's capture paragraph says both.
- `src.md`: the `capture.h` and `options.h` lines.

## Done when
After the folder's build, from the repo root:
`build/debug/editor/voe_editor examples/tank_game --capture build/debug/tank_window.png --size 1280x720 --frames 60 --capture-view build/debug/tank_view.png`
exits 0, both files exist, and `tank_view.png` is not 1280×720 (the view's
own size; `file build/debug/tank_view.png` shows it).
`build/debug/editor/voe_editor --frames 3` exits 2 with the usage line.
