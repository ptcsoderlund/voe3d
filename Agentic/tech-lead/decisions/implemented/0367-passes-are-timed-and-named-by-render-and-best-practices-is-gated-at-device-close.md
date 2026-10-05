# 0367 — Passes are timed and named by render, and Best Practices is gated at device close
date: 2026-10-05
by: planner

## Decision
For 062, carrying out 0358.

1. **Render names every pass from its kind**, never the caller: `view window`, `view target N`,
   `interface window`, `interface target N` (a pass with no camera), `shadow light L cascade C`,
   `point shadows`, `bounce capture N` (N from 1 in the frame), `bounce sun shadow`, `bounce relight`.
   At most `VOE_RENDER_PASS_NAME` bytes with the NUL.
2. **One query pool per frame slot holds 2 + 2 × (passes + 1) timestamps**: the frame's own pair at 0 and
   1 as today, then a pair per timed pass in the order run, the + 1 for the relight. The pass's pair is
   written where `voe_render_pass_start` runs and at `voe_render_pass_end`. They are read after the
   slot's fence with the frame's pair, so the breakdown describes the same frame as
   `voe_render_frame_gpu_time`; `voe_render_frame_pass_times` copies it out.
3. **The same name is the pass's debug label.** `VK_EXT_debug_utils` is enabled whenever the instance
   offers it, in every build; images, buffers and pipelines are named through one helper in
   `render/src/debug_names.c`, a no-op when the extension is absent.
4. **Best Practices, debug build only.** When the validation layer offers `VK_EXT_layer_settings`, the
   instance sets `validate_best_practices` and all four vendor sets (AMD, Arm, IMG, NVIDIA); a vendor's
   message is dropped when its id name carries a vendor other than the chosen card's. Without it, core
   Best Practices through `VkValidationFeaturesEXT`, and the start line says vendor checks are off.
5. **The allowlist is a table in `render/src/best_practices.c`**: a message id name and one line why.
   Every validation error, and every warning whose id is not on it, is counted as new on the device.
6. **The gate is the device's close**: a debug device that counted a new message reports the count and
   asserts in `voe_render_device_destroy`, so any test that drew one fails. A missing layer prints one
   line at open and fails `render/tests/best_practices.c`; the program still runs.
7. Once the card is chosen, render prints one line: checks on or missing, the vendor, the allowlist's
   length.

## Reasoning
Names from the kind need no new argument on any begin, and every pass already goes through
`voe_render_pass_start` and `voe_render_pass_end`. Gating at close reaches every test in every folder that
opens a device, without a test per folder.
- A name argument on every begin: a public change in six callers for a label the kind already gives.
- A gate only in one render test: misses warnings that only the editor's or 3d's tests draw.
- Aborting at the message: also kills a person's debug session mid-edit, and gives no count.

## Replaces
Nothing.
