# 0233 — A held right button flies the view under it, the pointer locked and hidden
date: 2026-09-23
by: planner

## Decision
For 023. A right-button press over a scene view (last frame's rectangle, as a click is read) makes that
view the flying one until the release; a press over no view, while the browser shows, or while the
middle-button drag holds a view flies nothing. Flying is `view.c`'s, beside the orbit that owns the eye:
turning changes `yaw` and `pitch` with the eye held still, the focus moved to stay `distance` ahead;
moving adds the same step to the eye and the focus. W/S along the look direction including pitch, A/D
along the level right, E/Q along world ±Y; held keys are summed and the sum normalised; 4 m/s, Shift ×3,
times the frame's capped `tick.step`. Mouse motion (`voe_platform_input_motion`, not the pointer) turns
at 0.004 rad per unit, dev's rate, pitch clamped to the orbit's limit.

While flying, `main.c` asks `platform` for the lock, hands `ui` no pointer and no typed text, Enter, Tab
or Backspace, and every shortcut is silenced by a `flying` guard in `shortcuts.h`, which also takes the
editor out of rest. Nothing is written to the world, undo or settings.

`platform` hides a locked pointer. On Wayland, once the lock is reported `locked` and only while
cursor-shape-v1 is bound, `wl_pointer_set_cursor` is given no surface; on unlock or release the shape is
put back by name, which is the way back the old "never hidden" rule lacked. Without the protocol the
pointer is frozen and visible, as before. On Windows the lock remembers the cursor's screen position
when the clip starts and puts it back when the clip ends, so both platforms show the pointer where it
was. `window_win32.c`, past 800 lines, is first split into window and seat as the Wayland side is.

## Reasoning
The feature asks for the Unreal/Unity fly; the orbit's pose already holds an eye, yaw and pitch, so
keeping focus and distance consistent lets the middle-button orbit carry on from wherever flying left
off with no second pose. `dev`'s flight cannot be reused: `dev` is a leaf the editor may not name, and it
reads the window itself. A named shape (0227) is what makes hiding reversible without a cursor image.

## Replaces
Nothing. Narrows `include/platform/input.h`'s note that hiding the pointer for the lock is unowned.
