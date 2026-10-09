# 20 — A window runs without Best Practices, so its GPU times are true
folder: render
after: 19
decisions: 0168, 0358, 0367, 0398
read: feature.md

## Change
0398: a debug build loads the validation layer on every device, but turns on Best Practices and the vendor
sets only on a headless device, or on a windowed one when the environment has
`VOE_RENDER_BEST_PRACTICES=1`. Nothing changes for a release build or for the gate at close.

- `render/src/best_practices.c`: new `voe_render_best_practices_wanted(bool headless, const char *setting)`,
  where `setting` is the variable's value or NULL. True when headless. On a windowed device, true only for
  exactly `"1"`; NULL, `""`, `"0"` or anything else is false. Declare it in `render/src/device_calls.h`
  beside the other `voe_render_best_practices_*` calls. A pure function, so a test can call it with no
  device and no environment.
- `render/src/instance.c`: in `voe_render_instance_create`, call `turn_on_best_practices` only when
  `validate` and `voe_render_best_practices_wanted(device->headless, getenv("VOE_RENDER_BEST_PRACTICES"))`
  (`device->headless` is already set before the instance is made). With `validate` true and Best Practices
  not wanted, the layer and the messenger stay, `checks_on` stays false and `checks_missing` stays false.
  Header: the "VALIDATION IS DEBUG-ONLY" paragraph says Best Practices is on for a headless device, and
  for a windowed one only by the variable (0398), and why: it slows the card's own work about fivefold, so
  the Frame panel would not show the engine's cost.
- `render/src/device_internal.h`: beside `checks_on`, a `bool checks_off_for_window`, set by instance.c
  when the layer loaded on a windowed device and Best Practices was not wanted. Its comment: never set
  together with `checks_on` or `checks_missing`. Update the block's comment to name it.
- `render/src/best_practices.c`, `voe_render_best_practices_announce`: a windowed device with
  `checks_off_for_window` prints one warning: `best practices checks off for a window, so GPU times are
  true; VOE_RENDER_BEST_PRACTICES=1 turns them on (allowlist of N)`. A windowed device with `checks_on`
  keeps today's line, followed by `GPU times are slowed by these checks; the frame breakdown is not the
  engine's cost`. A headless device's line is unchanged. Assert the three flags pairwise exclusive.
- `render/include/render/device.h`: `voe_render_device_new`'s contract gains one sentence (in a debug
  build it runs core validation only, Best Practices by `VOE_RENDER_BEST_PRACTICES=1`, 0398), and
  `voe_render_device_new_headless`'s gains that a headless debug device always runs Best Practices. The
  close's "IT IS THE BEST PRACTICES GATE" paragraph is still true: validation errors still count on a
  window.
- New `render/tests/best_practices_wanted.c` (header comment; entry in `render/tests/tests.md`), needs no
  graphics card: `voe_render_best_practices_wanted` is true headless with NULL, `"0"` and `"1"`; true for a
  window with `"1"`; false for a window with NULL, `""`, `"0"` and `"yes"`.
- `render/src/src.md`: the `instance.c` and `best_practices.c` entries say the same in a clause each.

## Done when
`render/tests/best_practices_wanted.c` passes, and `render/tests/best_practices.c` still passes and still
finds the checks on (a headless device).

Human, on the laptop (the coder does none of this): start the debug editor on a project with a landscape.
The start line says Best Practices is off for a window. With the camera still and the probes settled, the
Frame panel's `view target 1: terrain` is near the release build's, not five times it. Start it again
with `VOE_RENDER_BEST_PRACTICES=1`: the line says the checks are on and the times are slowed.
