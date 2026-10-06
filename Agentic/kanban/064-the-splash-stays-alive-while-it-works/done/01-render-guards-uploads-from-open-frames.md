# 01 — The device keeps uploads out of a frame open on another thread
folder: render
after: none
decisions: 0168, 0362, 0370

## Change
One other thread may upload while the owner draws frames (0370 point 5). No public signature changes.

- `render/src/device_internal.h`: the device gains the guard: an `mtx_t`, a `cnd_t`, whether a frame
  is open and the `thrd_t` that opened it. Its header says what the guard excludes and why (a set
  may not change while a recorded frame binds it).
- `render/src/device.c`: made in both device opens, destroyed in `voe_render_device_destroy`.
- `render/src/frame.c`: `voe_render_frame_begin` holds the guard for its own work and marks the
  frame open with its thread when it answers drawing; `voe_render_frame_end` holds it for submit and
  present, then marks the frame closed and wakes waiters, on every path out. A begin that answers
  false or not drawing leaves no frame open.
- One internal pair, declared in `render/src/device_calls.h` under device.c: take the guard
  (waiting while a frame is open on a thread other than the caller's) and give it back. A frame open
  on the caller's own thread does not wait, so the main thread's own uploads are unchanged.
- Every public call that creates, frees or writes a pool, the texture or shading table, a target or a
  descriptor set, or uses the queue outside a frame, takes the pair around its whole body: at least
  `voe_render_geometry_create`, `_destroy`, `voe_render_texture_create`, `_destroy`,
  `voe_render_shading_create`, `_destroy`, `voe_render_target_create`, `voe_render_target_resize`.
  Find the rest with `grep -n "^\[\[nodiscard\]\]\|^bool\|^void" render/include/render/device.h`
  and the header of each owning file in `render/src/` (`geometry.c`, `texture.c`, `shading.c`,
  `target_own.c`); `voe_render_geometry_create_transient` and everything inside a frame take nothing.
- `render/include/render/device.h`: the top comment says which calls one other thread may make
  while the owner draws frames, and that a frame open elsewhere makes them wait.
- `render/tests/worker_guard.c` (new): a headless device with element room; a `thrd_t` creates and
  destroys 100 small textures and 100 geometries while the main thread runs 100 element-only frames
  (as `render/tests/elements.c` draws one); every call true, the worker joined, the device destroyed.
  Skips without a graphics card, as the other tests do.
- `render/src/src.md`, `render/tests/tests.md`: entries for the changed files and the new test.

## Done when
`ctest --test-dir build/debug -R '^render/worker_guard$'` passes, and `render/elements` and
`render/textures` still pass.
