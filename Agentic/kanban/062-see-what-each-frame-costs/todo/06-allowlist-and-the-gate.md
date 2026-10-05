# 06 — Today's warnings allowed, and a new one fails at the device's close
folder: render
after: 05
decisions: 0168, 0358, 0367

## Change
- `render/src/device.c`: `voe_render_device_destroy`, in a debug build, reports how many new
  messages the device counted and asserts the count is nought (0367 point 6). Its comment says why
  the gate is here.
- `render/include/render/device.h`: `voe_render_device_destroy`'s comment states the gate.
- `render/src/best_practices.c`: fill the allowlist with today's messages. Build everything in
  debug and run every test of every folder, plus
  `build/debug/editor/voe_editor examples/tank_game --capture <scratch>/tank.png`, collecting each
  distinct id the messenger printed as not allowed. Each entry gets one line saying why it stands
  (what it flags and why it is not fixed in this feature). A validation error is never allowed: if
  one appears, fix its cause in `render` if it is there, else stop and report it as blocked.
- New test `render/tests/best_practices.c` (headless): in a debug build it asserts the checks are on
  (reading the device through `../src/device_internal.h`, as `card.c` reads `startup.h`), draws a
  frame with a shadow pass, a camera pass with a depth copy and an element, and closes; in a release
  build it says so and passes. Entry in `render/tests/tests.md`.

## Done when
- `cmake --build --preset debug && ctest --test-dir build/debug` exits 0.
- `ctest --test-dir build/debug -R '^render/best_practices$'` passes.

## Human
Start a debug build of the editor: the start log says Best Practices checks are on and how many
allowed warnings remain (feature step 6).
