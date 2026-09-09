# 040 — the draw count on screen, with the other statistics

status: todo
claimed-by: -
blocked-by: -

Written by the tech lead under the standing grant, at the principal's request:
*"Can we have a draw call counter on screen with the other statistics?"* **Not a
spin-off**: it takes the next free number.

**There is no ADR behind this card and it does not need one.** It decides nothing
structural: the number already exists, the readout already exists, and this puts
the first inside the second. It is small on purpose — if it grows past the shape
below, stop and report rather than growing it.

## Goal

**The on-screen readout gains a line saying how many draw commands the frame took**,
beside the frames per second and the timings, and it agrees with the number the
console block already prints.

## Why it is worth a card at all

Because **ADR-0092's whole claim is a number of draw commands**, and until now that
number lives in a console block printed once. `voe_render_frame_draw_count`'s own
header says why it exists: *"'Many small things in one draw' is a claim about the
number of draw commands and nothing else, so a program that makes it says this
number rather than saying it drew one instanced draw."*

Put it on screen and the claim becomes something a person watches while they drag
the window around, add a panel, or turn something on — which is exactly when a
regression in it would otherwise go unnoticed.

## The one thing to understand before starting

**The readout is built before the draws are issued, so the count it can show is
last frame's.** Look at the order in `dev/src/main.c`: the readout's text is
formatted and uploaded as transient geometry *before* `voe_3d_draw_system_run`
walks the world, because the geometry has to be in the mesh table before the walk.
`voe_render_frame_draw_count` is *"reset by every `_begin`"*, so at the moment the
text is built, this frame's count is nought or partial.

**So: keep the completed count from the previous frame and show that.** This is
already how the file treats a number that is not available yet — the gpu line shows
a measurement *"from a frame two frames back"*, and that same function already has
three branches so the line holds still rather than flashing. Follow it:

- Capture the total after `voe_render_frame_end` — where `draws_after_elements` is
  already read — into the timing struct beside the samples.
- **Say in the label that it is the previous frame**, or make the wording honest in
  some other way you can defend, and put the reason in a comment. A number that is
  silently one frame stale is the kind of thing somebody later chases as a bug.
- **The first frame has no previous frame.** Show something that is true rather than
  a nought that looks like a claim; the gpu line's *"no measurement"* is the
  precedent.

## Scope — `dev` only

- One more line in the readout's `snprintf`, in the existing column style — the
  labels are left-aligned words with the number in a fixed field, and it should sit
  with `frame`, `update`, `draw` and `gpu` rather than looking bolted on.
- **Show the element share as well as the total**, because that is the number the
  claim is about: the whole frame's commands, and how many of them the element path
  cost. The console block already computes exactly this pair either side of
  `voe_dev_elements_submit`; reuse it rather than counting again.
- The counters live in the timing struct with the other measurements, not in file
  scope.
- **Nothing outside `dev` changes.** `render` already exposes the count; `text`,
  `3d` and `ui` are untouched. If you find yourself wanting a new call in `render`,
  stop and report — it means the number you want is not the number that exists.

## What to watch for

- **The readout draws itself.** It is a mesh like any other, so the total includes
  the draw that put the readout on screen. Do not subtract it and do not pretend
  otherwise — say it in a comment, because a person counting objects on screen and
  getting one more than they expected will otherwise think it is a bug.
- **The number must agree with the console block.** If the line on screen and the
  line in the terminal disagree, one of them is wrong and it is worth finding out
  which before you finish.
- **A glyph budget.** The readout's characters are counted for the transient glyph
  budget (`readout_glyphs`, checked against `MAX_TRANSIENT_GLYPHS`); a longer string
  costs more glyphs. Check the budget still holds and say what the new count is.
- **Do not add a second timing period or a second average.** A draw count is an
  integer that is exact, not a sample to average. Showing last frame's value plainly
  is right; a running mean of it would be a worse number, not a better one.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **A screenshot of the readout with the new line in it**, and the console block in
  the same run, so the two numbers can be read against each other.
- Turn something off and watch it move: the exhibit and the readout are both drawn
  every frame, so **hiding the element exhibit or the models should change the total
  by a predictable amount.** Say in your report what you changed and what the number
  did — that is the whole verification of a counter.
- The first-frame case, looked at once: run it and read the first line before any
  frame has completed.
- Windows is the principal's, and this is his request, so make the line legible at
  the default window size rather than only at yours.

## Report when this lands

- The line as it appears, quoted, and the wording you chose for *previous frame*.
- The glyph count before and after, against the budget.
- What the total is in a normal frame, broken down — how many are meshes, how many
  the element path, how many the readout itself. That breakdown is the interesting
  output of this card and it is what the principal actually asked to see.
- Anything the two numbers disagreeing revealed, if they did.
