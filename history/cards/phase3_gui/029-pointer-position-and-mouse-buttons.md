# 029 — pointer position and mouse buttons

status: review
claimed-by: claude-fable-5-1 (kanban-coder)
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it follows
028 and takes the next number.

**This is the first card of the GUI stack** (`docs/prestudy/gui-stack.md` in the
planning root — you do not need it). It is entirely inside `platform` and touches
nothing else.

## The card your own header asked for

`platform/include/platform/input.h` already says this:

> **MOUSE BUTTONS ARE NOT HERE, AND THAT IS THIS CARD REPORTING RATHER THAN
> FORGETTING.** Relative motion is the mouse's whole contribution to a camera and
> it is below; nothing in the engine clicks on anything yet, so a button would be
> surface with no caller. **The card that puts something on screen to click is the
> card that adds them.**

Something is about to be put on screen to click. This is that card, and the
paragraph above is the one to rewrite when you are done.

## Goal

A program can ask **where the pointer is** and **whether a button is down**, on
both platforms.

Today `platform` offers relative motion only — *how far the mouse moved, not where
it is* — which is exactly what a camera needs and nothing a GUI can use.

## Scope — `platform`

### Where the pointer is

- A call that hands back the pointer's position **in the window's own pixels**,
  origin top-left, matching what both backends report and what
  `voe_platform_size` already means.
- **It is a position, not a delta, and both now exist side by side.** Say in the
  doc comment which is for what: motion drives a camera, position drives a
  pointer. Somebody will reach for the wrong one.
- **What it does when the pointer is not over the window** is a real question and
  the card must answer it out loud rather than let each backend decide. Recommended:
  hand back the last position and a separate *is it over the window* answer, since
  a GUI needs to know the difference between *the pointer is at the edge* and *the
  pointer is gone*.
- **What it does while the pointer is locked.** `voe_platform_input_lock_pointer`
  exists for camera look and, while it is on, there is no meaningful position.
  Decide it and write it down; do not let it return stale numbers that look live.

### Which buttons are down

- Left, right and middle. Not more: engine rule 10, implement on demand, and the
  key table's own precedent is that keys are added when something wants them.
- **Down is not enough on its own and the GUI will need more.** A button reports
  *clicked* when it is released over the same widget it was pressed on, so a caller
  needs to distinguish *held* from *went down this frame*. Whether that edge
  detection belongs here or in the GUI is **yours to decide and to state** — the
  keyboard's existing shape is level-only (`_key_down`), so matching it and leaving
  edges to the caller is the consistent answer and probably the right one. Say
  which you chose and why.

### Scroll

- A wheel delta, drained per frame like motion. The scroll area on a later card is
  the caller. **If adding it would grow this card much, leave it out and say so** —
  it is honest to report that rather than half-build it.

## What must not change

State in your report that you checked each of these:

- **Relative motion stays exactly as it is.** The camera depends on it and on its
  drain-and-forget behaviour. This card adds beside it; it does not reshape it.
- **Pointer locking stays as it is**, including that it is a request whose answer
  is `_pointer_locked`.
- **No cursor image.** The header explains at length that this engine has none to
  hand a compositor, and that a hidden cursor with no way back is worse than a
  still one. **That reasoning stands and this card does not touch it.** Drawing a
  pointer is a GUI card's problem, later, and in the world rather than as a system
  cursor.
- **The key enum.** Buttons are not keys; do not fold them in.

## Where this card is likely to go wrong

- **The two backends disagree about coordinate origin and scaling.** Wayland
  reports surface-local coordinates that are already the compositor's business, and
  the existing motion comment says the units carry pointer acceleration. Get the
  origin and the scale to agree between platforms, and **state in your report what
  each backend actually gave you** — that note is what a later Windows bug gets
  written against.
- **Linux is what you can verify.** Windows is the principal's, as always. Write the
  Win32 side carefully and say plainly that it is unverified.
- **A position that is silently stale.** Locked pointer, pointer outside the window,
  window unfocused — three states where the honest answer may be *no position*, and
  a number that looks fine is worse than a refusal.
- **Over-building.** No gestures, no double-click, no drag detection, no cursor
  shapes. Those belong to whatever needs them.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **The dev program prints or draws the pointer position and the three button
  states**, live. Move the mouse to the four corners of the window and confirm the
  numbers reach the corners and do not overshoot or invert.
- **Confirm the camera still works.** Fly around with the mouse as before — this is
  the check that motion was not disturbed.
- **Toggle pointer lock and confirm the position behaves as you documented**, rather
  than as whatever fell out.
- Move the pointer off the window and back; confirm the over-the-window answer.
- Windows is the principal's.

## Report when this lands

- Whether edge detection went in `platform` or was left to the caller, and why.
- What each backend reported for coordinates, in its own units, and what you did to
  make them agree.
- Whether scroll made it in.
- What the position does when locked, outside the window, and unfocused.
- The rewritten *mouse buttons are not here* paragraph — it is now false and it is
  the sentence a future reader will trust.

## Notes (coder, 2026-09-09, Linux/WSL, claude-fable-5-1)

**Implemented in full. `cmake -P check.cmake` exits zero — all steps, 35 tests,
analyser clean over 94 files — but not in place.** This WSL machine has no
`ninja`, `slangc`, `wayland-scanner`, `pkg-config` or Wayland headers, and no
sudo, so a private toolchain was assembled in the session's scratch directory
(Ubuntu's own `.deb`s for ninja, pkgconf and libwayland extracted without
installing, plus the Slang 2026.17 Linux release) and put on `PATH`. With that,
step 1 passes and every folder configures standalone. The second obstacle is the
9p mount the checkout sits on: CMake's `configure_file` and the
`compile_commands.json` copy fail on it with "Operation not permitted", inside
any build directory under the repository, and `check.cmake` hardcodes its build
tree there. So the source tree was mirrored byte for byte into scratch
(everything but `build/`, `.git/` and the compile databases) and the unmodified
script run on the mirror, twice: once before and once after a comment-only
correction to the public header. Both runs green, the second after the final
sync. `platform` was also configured and built standalone on the real checkout
(Ninja generator, `-Werror`), its two tests passed under `ctest`, and
`clang --analyze` was replayed by hand over the three changed Linux files from
that build's compile database: nothing reported.

**The dev program ran under WSLg on llvmpipe** for eight seconds from the
mirror's build: window opened at 960x540, models loaded, readout reported
`71 glyphs` (it was about fifty; the difference is the new `mouse` line), timing
blocks came at ~190 frames a second, clean exit on the timeout. **What was not
done here is the part that needs a hand:** moving the pointer to the corners,
pressing the buttons, dragging out of the window, toggling the lock. There is no
tool on this machine to drive a Wayland pointer, and I cannot see the readout.
That check and all of Windows are the human's. The main.c header's "what there
is to try" list has the exact things to look for.

**What was added.** `platform` only, plus the two lines of `dev` the card's own
Verify section asks for and nothing else in it:

- `include/platform/input.h` — `voe_platform_pointer { float x, y; bool over; }`
  and `voe_platform_input_pointer`; `voe_platform_button` (LEFT, RIGHT, MIDDLE,
  COUNT) and `voe_platform_input_button_down`. The "mouse buttons are not here"
  paragraph is replaced by three: motion versus position and which is for what,
  buttons are level not edge and why, scroll is not here and why.
- `src/input.h` / `src/input.c` — four new fields on the shared struct
  (`pointer_x`, `pointer_y`, `pointer_over`, `buttons[]`), the two accessors,
  and one new shared clear, `voe_platform_input_pointer_lost`, which both
  backends call when the pointer leaves or goes. `begin_poll` deliberately does
  not touch any of it. The lock is folded into `over` in `input.c`, once, for
  both platforms.
- `src/window_wayland.c` — `enter`, `motion`, `button` and `leave` filled in;
  `axis` still received and dropped, with the reason. `pointer_left` (mouse
  unplugged) calls the new clear.
- `src/window_win32.c` — `WM_MOUSEMOVE` and the six button messages,
  `SetCapture` while a button is held, `WM_CAPTURECHANGED` as the single place
  that reacts to losing it, `TrackMouseEvent` for `WM_MOUSELEAVE`. **Unverified:
  no Windows toolchain here.** Written against the documented message shapes;
  the three things to watch on Windows are listed under "where this could be
  wrong" below.
- `tests/input.c` — four new tests: a poll keeps the pointer, buttons and
  over-flag; focus lost leaves all three alone; pointer lost releases every
  button and keeps the position; pointer lost leaves keys, lock and motion.
- `platform.md` — four lines updated. `dev/dev.md` one line.
- `dev/src/main.c` — the readout gains a sixth line, `mouse  x y LMR`, or
  `mouse  locked` / `mouse  away` with the button letters; the header's
  checklist says what to look for; `READOUT_CHARS` 128 → 192 so the extra line
  fits with room; the "about fifty" glyph estimate corrected to seventy.

**The decisions the card asked me to make and state:**

- **Edge detection stays with the caller.** `_button_down` is level-only,
  matching `_key_down`. Reason: a GUI's click is "released over the widget the
  press landed on", which platform cannot know; the header already explains the
  two-bools-per-call-site cost for keys and the Tab toggle in dev is the worked
  example; a second, edge-shaped API beside the level one is exactly the second
  API the header says this folder must not grow.
- **Not over the window: last position kept, `over` false.** As the card
  recommended. One struct rather than two calls, so a caller cannot read the
  position and forget to ask whether it is live.
- **Locked: `over` false, position kept.** Decided out loud in the header. A
  locked pointer is the camera's; Wayland sends no motion at all while locked so
  the number underneath simply stops, and Windows keeps sending clipped moves
  under a hidden cursor — either way a live-looking number would be a lie. The
  fold happens in `input.c` so both platforms agree by construction, and
  `pointer_over` itself is left as the window system said so that unlocking
  needs no event to bring the position back.
- **Unfocused: still live.** Focus is the keyboard's. A pointer resting over an
  unfocused window is over it on both window systems and reports where. Tested.
- **Drag past the edge: position goes outside the client area and `over` stays
  true** while a button is held. Wayland does this on its own (implicit grab
  until release); Windows is asked for it with `SetCapture`. A slider dragged
  past the window keeps following; the caller clamps if it wants to.
- **Scroll left out**, as the card permitted. Two reasons beyond size: nothing
  reads a wheel (rule 10), and the unit is a real decision — Windows gives 120
  per notch, version-1 `wl_pointer.axis` gives a compositor-chosen length per
  notch with no notch count (that arrives at version 5, and the seat is bound
  at 1 for the reasons the Wayland header gives), and the signs are opposite.
  The scroll area card should choose the unit with its needs in hand. The
  header says where it goes: beside motion, drained by the poll.

**What each backend actually gives, in its own units — for the Windows bug
report later:**

- **Wayland:** `wl_pointer.enter`/`motion` carry surface-local `wl_fixed_t`
  (24.8 fixed point, so fractions are real and are kept as `float`), origin at
  the surface's top-left, +x right, +y down. A surface unit is one of our pixels
  because the engine never sets a buffer scale, so it is the same unit the
  configure size and `voe_platform_size` are in. Buttons arrive as evdev codes
  (`BTN_LEFT` 0x110 …) from the header the scancodes already come from. **No
  arithmetic was needed** to make it agree with Windows.
- **Win32:** `WM_MOUSEMOVE` and the button messages carry client coordinates in
  `lparam`, whole pixels, origin top-left, +x right, +y down; the same space
  `WM_SIZE` reports the client area in. The halves are read as *signed* shorts
  because a captured pointer goes negative — `LOWORD` alone would read −1 as
  65535. The process declares no DPI awareness, so on a scaled display Windows
  virtualises both the size and the position by the same factor and they still
  agree with each other and the swapchain. **No arithmetic was needed here
  either**; the agreement is by both systems' own conventions.

**What must not change — checked:** relative motion untouched (same two lines
in each backend, same `begin_poll` drain, camera code unchanged, the existing
motion tests still pass); pointer locking untouched (`lock_start`/`lock_stop`,
`apply_lock`, `_pointer_locked` all as they were); no cursor image anywhere and
the Wayland header paragraph on it stands; the key enum untouched — buttons are
their own enum.

**Where this could be wrong, on the platform I could not run:**

- `WM_MOUSELEAVE` under capture: if Windows delivers it while a button is held,
  it is ignored on purpose and the release recomputes `over`; if it does not,
  nothing is lost either. Both paths are written.
- `WM_CAPTURECHANGED` also arrives for our own `ReleaseCapture`, with every
  button already up, so the clear there is a no-op on that path — intended.
- If the pointer reads as `away` while visibly over the window on Windows,
  suspect `TrackMouseEvent` firing early; `pointer_at` re-arms only while inside
  to avoid exactly that.

**Markers left:** none. No `DEVIATION:`, no `BLOCKED:`.
