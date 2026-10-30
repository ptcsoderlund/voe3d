# 02 — The app opens and closes the workers
folder: app
after: 01
decisions: 0168, 0497, 0504

## Change
A program's workers stand from its app's opening to its closing (0504 point 4).

`app/src/app.c`: `voe_app_new` and `voe_app_new_headless` call `voe_platform_workers_open()`
(`platform/burst.h`) once they will return a non-NULL app; `voe_app_destroy` calls
`voe_platform_workers_close()` after the device and window are closed. A failed new opens nothing.

`app/include/app/app.h`: the two startups' and destroy's comments say they open and close the
program's standing workers, and why here: every program (editor, game, dev, a capture) opens one
app for its whole run, so this is "once when it opens, once when it closes" (0497); every burst
started must be finished before destroy.

Index entries to update: `app/app.md`'s `app.h` line (the startups also open the workers);
`app/src/src.md`'s `app.c` line.

## Done when
`app/tests/capture.c` gains a check: after `voe_app_new_headless` succeeds,
`voe_platform_workers_standing()` is at least 2, and after `voe_app_destroy` it is 0 (skipped
without a graphics card as the test already does). The test `app/capture` passes.
