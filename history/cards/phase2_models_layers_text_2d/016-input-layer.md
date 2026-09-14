# 016 — an input layer

status: complete
claimed-by: claude-code (kanban-coder)
blocked-by: 015

Keyboard and mouse, in `platform`, on both platforms. Ships with a consumer:
by the end of this card the camera is flown by hand instead of orbiting.

## Goal

Move and look with keyboard and mouse in the dev window.

## Scope

- **Keyboard and mouse**, per the v1 capability list. Gamepad is on the *later*
  list and is not this card.
- **`platform` owns it and names no Vulkan and no renderer.** The seam that already
  exists for window handles is the model to copy.
- The camera from card 015 is driven by it. The orbit stays available — it is the
  better thing to look at when checking rendering rather than input.

## What makes this two-platform work rather than one

    Linux     wl_seat, wl_keyboard, wl_pointer   (Wayland, ADR-0037)
    Windows   WM_KEYDOWN / WM_INPUT / WM_MOUSEMOVE

**Decide and state whether input is polled state or a queue of events.** A camera
wants "is this key held"; a future editor wants "what happened, in order". Pick
one, say why, and do not build both.

**Mouse look needs relative motion, not cursor position**, and on Wayland that is
`zwp_relative_pointer_v1` plus a pointer lock — a separate protocol from the ones
already vendored. If that turns into more than this card should carry, report it
rather than half-building it.

## What this unblocks

The parked question of whether VOE3D draws its own window frame. That was blocked
on pointer input and stays parked — this card does not answer it, it just stops
being the reason.

## Verify

- Fly the camera on both platforms. Held keys, released keys, focus lost with a key
  still down — that last one is where input layers leak state.
- `check.cmake` zero. Windows is the principal's.

## Notes — verification

Implemented in `platform` (`include/platform/input.h`, `src/input.h`,
`src/input.c`, both backends), consumed by `dev` through `render`'s
`voe_render_camera_input`. `platform.md` updated.

**Decision the card asked to be stated: polled state, not a queue of events.**
The reasoning is in the header of `include/platform/input.h` — a camera asks
"is this key held", nothing has a caller for an ordered history, and rule 10
says the queue is written when something wants one.

Both extra Wayland protocols were needed and are vendored:
`relative-pointer-unstable-v1.xml` and `pointer-constraints-unstable-v1.xml`.
Both are optional at run time; a compositor offering neither leaves the keyboard
working and mouse look absent. That did not overflow the card.

### What was run

`cmake -P check.cmake` — **all 15 steps ok**, 11 tests passed, analyser clean
over 33 files, on Linux (clang 21.1.8, cmake 4.2.3, slangc 2026.16.1,
wayland-scanner). Guard steps 4a/4b/4c all fired as expected.

`dev/voe_dev` launched against WSLg: window opened 960x540, Vulkan device came
up (llvmpipe), orbit camera reported. No crash, clean startup.

### Not verified, and needs the principal

- **Flying the camera by hand — not done here.** Held keys, released keys and
  alt-tab-with-a-key-down all need a person at a keyboard; this session has no
  way to synthesise input into the compositor. `tests/input.c` covers the two
  places the state actually leaks (the poll drain and the focus clear) without a
  display, but the end-to-end feel is the review.
- **Windows — not touched.** Per the card, the principal's.
- Only a software renderer (llvmpipe) was available here.

### Environment finding, not a code problem

`cmake -P check.cmake` **cannot be run in place on this machine.** The repo sits
on a 9p/DrvFs Windows mount (`/mnt/dev` → `C:\Users\...`), where CMake's
`configure_file` and `cmake -E copy_if_different` fail with "Operation not
permitted" because they cannot set permissions on the destination. That stops
step 2, and separately the `compile_commands.json` export target in
`cmake/voe.cmake` cannot write back into the source tree. The run above was made
against a copy of the tree on tmpfs, where the unmodified script passes. Nothing
in the repository was changed for it. Worth a decision if the principal wants
`check.cmake` runnable from a Windows-mounted working copy under WSL.

### Reported, not fixed

The working tree shows all 77 files modified, but the diff is **entirely CRLF
line endings** — `git diff --ignore-cr-at-eol` is empty, 38421 insertions
against 38421 deletions. The repo has no `.gitattributes` and `core.autocrlf` is
unset, so a checkout on Windows rewrites every line ending. Pre-existing and
unrelated to this card, so left alone.

No `DEVIATION:` and no `BLOCKED:` markers were left in the code.

## Notes — review round 1

Principal confirmed on review: in fly mode the pointer goes away and the mouse
turns the camera. Asked for two more keys.

**Added `VOE_PLATFORM_KEY_Q` and `VOE_PLATFORM_KEY_E`** — E up, Q down, alongside
Space and Ctrl rather than replacing them, so the whole of flying is reachable
from the left hand. Four sites: the enum in `include/platform/input.h`, the
`key_of` switch in each backend, and the `focus_gained` table in
`src/window_win32.c` — which must grow with the enum or an alt-tab back in would
read the two new keys as up. Neither folder `.md` enumerates keys, so neither
needed an edit.

The two synonym pairs are one test each (`SPACE || E`, `CONTROL || Q`) and not
two. Two tests each adding a step would make Space and E held together a rise of
two; normalizing downstream fixes the speed but not the direction, so a diagonal
held with both would lean upwards more than the same diagonal held with one.

`cmake -P check.cmake` re-run after the change — **all 15 steps ok**, 11 tests,
analyser clean over 33 files. `voe_dev` still starts clean on WSLg. Pressing the
new keys is still the principal's, on both platforms.

**Open question for the principal, not changed here:** movement is delta-time
based in form — `speed * dt` in `render/src/cube.c`, and mouse look deliberately
is not scaled by dt — but `dt` is the fixed placeholder `NOMINAL_FRAME_SECONDS`
(1/60) in `render/src/frame.c`, because nothing owns a clock yet. On a display
that is not 60 Hz everything moves by the ratio of the refresh rates. Card 020
is where that one line is replaced by a measured frame.
