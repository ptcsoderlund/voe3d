# 0398 — A window runs without Best Practices, so its GPU times are true
date: 2026-10-09
by: tech-lead

## Decision
For 084, amending 0358 and 0367 point 4.

1. **Best Practices is on for a headless device only.** A debug build still loads the validation layer on
   every device. A headless device (`voe_render_device_new_headless`) also turns on Best Practices and the
   four vendor sets, as 0367 point 4 says; every test opens a headless device, so 0358's gate is unchanged.
2. **A windowed device (`voe_render_device_new`) runs core validation only**: the editor, the game and dev,
   in a debug build. Validation errors are still counted, and still fail at the device's close (0367
   point 6). The environment variable `VOE_RENDER_BEST_PRACTICES=1` turns Best Practices back on for that
   run, to hunt warnings in a running editor.
3. **The start line says which.** A windowed debug device prints that Best Practices is off and that
   `VOE_RENDER_BEST_PRACTICES=1` turns it on. When it is on, the line also says that GPU times are slowed
   and that the frame breakdown does not show the engine's cost.

## Reasoning
Bug 02 of 084 was planned from the debug editor's Frame panel, and 0397 was built against costs that were
not the engine's. A measurement on 2026-10-09 (RTX 4070 Laptop, `Hill.landscape` 1000 m and 512 cells, the
editor's camera, 1024², headless, the same command stream in every row):

| run                                      | each capture | view terrain | settled frame |
|------------------------------------------|--------------|--------------|---------------|
| release                                  | 0.54 ms      | 1.34 ms      | 3.0 ms        |
| debug, layer and Best Practices on       | ~2.5 ms      | 7.8 ms       | 18 ms         |
| debug, layer off                         | 0.54 ms      | 1.36 ms      | 3.1 ms        |
| debug, layer on, Best Practices off      | 0.54 ms      | 1.34 ms      | 3.1 ms        |

Best Practices slows the card's own work about fivefold once a landscape is drawn. GPU-assisted validation
and sync validation are not the cause. Turning off the NVIDIA, AMD or Arm set alone did not help; only all of
Best Practices off did. With two cubes and no landscape the two builds time the same, which is why 0358's
breakdown looked sound until now. A person works in the debug editor, so its Frame panel must show the
truth by default (0388: smooth is judged by it).

- Best Practices off in every debug build: it ends 0358's gate, which every test enforces at close.
- Opt-out by environment variable: the default still shows wrong times, and the bug came from the default.
- Judge performance only in a release build: the Frame panel would then be wrong in the build people use.
- Turn off only the vendor sets: measured, it does not help.

## Replaces
Nothing. Amends 0358 (Best Practices runs on headless devices, which the checks open, not in every debug
run) and 0367 point 4 (on a windowed device only by `VOE_RENDER_BEST_PRACTICES=1`).
