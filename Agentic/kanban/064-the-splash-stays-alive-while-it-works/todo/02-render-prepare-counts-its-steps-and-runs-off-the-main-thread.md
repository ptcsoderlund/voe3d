# 02 — Prepare counts its steps and may run on a worker
folder: render
after: 01
decisions: 0168, 0362, 0370

## Change
- `render/include/render/device.h`: add
  `uint32_t voe_render_device_prepare_steps(const voe_render_device *device)`: how many steps
  prepare takes on this card from an unprepared device (the pipelines it builds here, the two
  layered ones only with shaderOutputLayer, plus the relight's startup), so a caller can show
  "12/40". The prepare comment gains: one other thread may call prepare while the owner draws only
  element passes; a pass that would build pipelines must not be drawn meanwhile.
- `render/src/pipeline.c`: the count. A pipeline build takes no guard (0370 point 5); the relight's
  startup step takes card 01's guard pair around itself. Its header says why.
- `render/src/src.md`: the `pipeline.c` entry mentions the count.
- `render/tests/prepare.c`: the count equals the PREPARING answers plus one on a fresh device; and a
  `thrd_t` runs prepare to PREPARED on a fresh headless device while the main thread draws on that same device
  element-only frames, then a camera pass draws.

## Done when
`ctest --test-dir build/debug -R '^render/prepare$'` passes.
