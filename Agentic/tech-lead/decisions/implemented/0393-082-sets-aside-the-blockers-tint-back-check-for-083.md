# 0393 — 082 sets aside the bounce scene's tint-back-after-a-blocker check; 083 restores it
date: 2026-10-08
by: planner

## Decision
For 082's suite (card 59 blocked: `3d/bounce_scene` fails at `3d/tests/bounce_scene.c:566`), carrying out
0392: the BLOCKED case of `3d/tests/bounce_scene.c` loses its last check, that the patch is back within
1/255 of before once the blocker is destroyed and settled. The checks while the blocker stands stay. The
file's header says the check is set aside until work order 083. 083's planner puts the check back as part
of fixing the add-then-remove path of a blocker.

## Reasoning
0392 sends 082 to its suite without the tint and names the add-then-remove path of a blocker as 083's fault.
That failing check is exactly that fault. 082 cannot fix it, and its suite cannot pass while the check stands.
- Keep the check and mark the whole test as expected to fail: this also hides any other regression in the file.
- Skip the whole BLOCKED case: this drops the blocker-standing checks too, and they still pass.

## Replaces
Nothing. Carries out 0392 for 082's suite.
