# 45 — Readiness is the validity image's second channel
folder: render
after: 44
decisions: 0168, 0386, 0387, 0388, 0389

## Change
Carry each probe's readiness (card 42) to the GPU (0389 point 4). The read uses it in card 47.

- `render/src/bounce_volume.c`: the validity image becomes `VK_FORMAT_R16G16_SFLOAT`. R stays validity,
  G is readiness from 0 to 1.
- `render/src/bounce_relight.h`:
  - A list word is the probe index in bits 0–15, readiness in bits 16–20, and the holds bit 31.
  - The push block gains the count of changed probes beside the count listed; keep its size assert true.
- `render/src/bounce_relight.c`:
  - Listing writes changed probes first, then fading probes that did not change, each with its readiness.
  - Settle dispatches over every listed probe.
  - Relight levels and sum run only when a probe changed or `voe_render_bounce_probes_lights_changed`.
    A begin with only fading probes records the settle and its barriers, and nothing else.
  - The `relit` call still follows.
- `render/src/bounce_shadow.c`: the sun-map pass opens only when a probe changed or the lights changed,
  never for fading alone.
- `render/shaders/bounce_relight.slang`:
  - The settle entry reads the word's fields and writes readiness / `VOE_RENDER_BOUNCE_FADE` into G.
  - A fading-only word leaves R and the moments as they are.
  - `relight_validity` becomes a two-channel storage image; every other reader of it reads `.r`.
- Header comments of the changed files: list word layout, the two runs of the list, settle-only begins.
  `render/include/render/device.h`'s `voe_render_bounce_relight` comment: a frame with only fading probes
  records a settle alone.
- `render/tests/bounce_settle.c`, new cases:
  - `a_new_picture_reads_ready_after_sixteen_frames`: readiness read back is 0 at the first relight, rises,
    and is 1 after `VOE_RENDER_BOUNCE_FADE` more frames.
  - `a_fading_frame_records_only_the_settle`: `device->relight_dispatches` rises by one on a frame with
    only fading probes, and no shadow pass opens.
- `render/tests/tests.md`: the entry names them.

Per frame (0388), `bounce relight`: one settle dispatch per volume for 16 frames after a capture. Settled
frames record nothing, as before.

## Done when
`ctest --test-dir build/debug -R '^render/bounce_(settle|probes_scene|read)$'` passes with the two new cases.
