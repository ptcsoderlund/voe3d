# 03 — Render draws depth into the sun's cascades
folder: render
decisions: 0168, 0258

## Change
The shadow maps and the pass that fills them (0258 points 1–2). Nothing reads them yet; card
04 does. Additions only: no call site outside `render` changes.

- `render/include/render/device.h`:
  - `#define VOE_RENDER_SHADOW_CASCADES 4`.
  - `voe_render_capacities` gains `uint32_t shadow_size` — texels a side of each cascade, per
    frame slot; nought is none. Its paragraph: cost (size² × 4 bytes × cascades × slots),
    nought allowed.
  - `[[nodiscard]] bool voe_render_shadow_pass_begin(voe_render_device *device, uint32_t
    cascade, const voe_render_view *light)` — opens a pass onto that cascade of this slot's
    map, clears depth to the reversed clear, viewport the map's size with the one Y flip every
    pass has; `light` is the camera block's view (sun block zeroed). Closed by
    `voe_render_pass_end`. False, with a line, when `passes` are spent. Asserts: no open frame,
    a pass open, a cascade out of range, a device with `shadow_size` nought.
  - `voe_render_frame_draw`'s paragraph: in a shadow pass it draws depth only through the
    shadow pipeline. `_draw_blended`, `_clear_depth` and `voe_render_frame_draw_elements` in a
    shadow pass assert; say so at each.
- `render/src/shadow.c` (new) — per frame slot one D32 2D array image of
  `VOE_RENDER_SHADOW_CASCADES` layers (one texel a side when `shadow_size` is nought, so card
  04's binding is always valid), an array view and one view per layer; startup, shutdown, and
  the barriers: to depth attachment when a shadow pass opens, to shader read-only when it
  closes. Header: 0258, the lifetime (startup to shutdown, not rebuilt on resize), why per slot.
- `render/src/device_parts.h` / `device_internal.h` (card 01) — the slot's shadow images and
  views, the shadow pipeline handle, `shadow_size`, whether the open pass is a shadow pass;
  declarations for shadow.c's calls.
- `render/src/pipeline.c` — a third mesh pipeline: `draw.slang`'s vertex entry only, no
  fragment stage and no colour attachment, D32 depth test and write GREATER, culling off,
  depth bias on with slope and constant factors signed for reversed depth (named constants,
  each with its reason). Header: three mesh pipelines now, what the shadow one drops.
- `render/src/pass.c` (card 02) — `voe_render_shadow_pass_begin`; `voe_render_pass_end`
  closes either kind, the shadow one with shadow.c's barrier.
- `render/src/draw.c` (card 02) — `draw_with` binds the shadow pipeline in a shadow pass; the
  asserts above.
- `render/src/element.c` — the element draw asserts in a shadow pass.
- `render/src/device.c`, `startup.h` — shadow.c's startup in order after the targets, before
  the descriptors; shutdown in reverse; `shadow_size` kept.
- `render/src/src.md` — entry for `shadow.c`; `pipeline.c`'s says three.
- `render/tests/shadow.c` (new, headless) — a device without `shadow_size` opens and draws as
  before; with it, four shadow passes each drawing a cube, then a window pass, end true, draw
  count five; a shadow pass past `passes` refused and the frame still ends. Skips without a
  graphics card.
- `render/tests/tests.md` — entry for `shadow.c`.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^render/shadow'` passes.
