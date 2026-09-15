# 040 — the draw count on screen, with the other statistics

status: review
claimed-by: claude-opus-5 (kanban-coder)
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
- **Amended 2026-09-09 — show the record count beside the total, not an element
  share.** The bullet that stood here asked for *the whole frame's commands, and how
  many of them the element path cost*, reusing the pair the console block computes
  either side of `voe_dev_elements_submit`. **That was true when this card was
  written and 032 made it false**: `voe_dev_elements_submit` only submits, and the
  frame's three element draws are no longer bracketable, because two of them are
  issued *inside* `voe_3d_draw_system_run`, interleaved by depth with the see-through
  meshes. There is no moment between them to read a count at, and that is 032 working
  rather than a defect.

  **What the line shows instead: the total commands and the total element records** —
  *"draws 32 · 94 elements"* or whatever the column style makes of it. Both are
  exactly measured, `voe_render_frame_elements_submitted` survives until the next
  `_begin`, and **the pair is a better statement of ADR-0092's claim than the draw
  split ever was.** That function's own header is what decides it: *"Forty rectangles
  in one draw is forty here and one there, and that gap is the whole claim of this
  path — reading the wrong one of the two turns the claim into something trivially
  true."* The meshes-versus-elements split was a proxy for that gap, reached for
  because the console block happened to bracket it. **The gap itself is now the thing
  on screen**, and a person watching it sees records climb while commands do not,
  which is the claim rather than a number that stands for it.

  **Rejected, and worth saying why**: a share that `dev` computes structurally — two
  panels plus one surface is three — is *what the program asked for rather than what
  `render` counted*, and a panel skipped as stale would make it silently wrong. That
  is precisely the failure this card's own *previous frame* paragraph exists to
  prevent, so it may not be reintroduced as a workaround for it. And the split that
  *is* measurable — *in the walk* versus *the screen-filling surface* — is true but
  names an implementation seam rather than anything a reader cares about.

- **`draws_before_elements` and `draws_after_elements` are renamed in this card.**
  After 032 the first is read before the walk, when nothing has been drawn, so it is
  always nought and its name stopped meaning anything. **Rename both to say that they
  bracket the walk, and keep the pair rather than dropping the subtraction** — the
  number worth printing is what the walk cost, and reading both ends stays correct if
  something is ever drawn before it. One comment line saying the first is nought today
  and why it is still read.
- The counters live in the timing struct with the other measurements, not in file
  scope.
- **Nothing outside `dev` changes.** `render` already exposes the count; `text`,
  `3d` and `ui` are untouched. If you find yourself wanting a new call in `render`,
  stop and report — it means the number you want is not the number that exists.

## Amended 2026-09-09: move surface 3's plate off the readout, inside this card

**Card 032 put the screen-filling surface's plate and marked square in the top-left
corner, which is where the readout has always been.** At 960×540 the plate covers the
numbers on the `frame`, `update`, `draw` and `gpu` lines. The new `draws` line sits
below it and is legible, so this card's own requirement is technically met — **and the
deliverable is still a screenshot of a readout you cannot read**, for a card the
principal asked for by name.

**Fix it here. Two constants in `dev/src/surface.c`.**

- **Move the plate and the marked square; leave the readout where it is.** The readout
  has been in that corner since long before either card and the plate is the
  newcomer.
- **The ticks along the top edge stay exactly as they are**, so the cut-off
  demonstration 032 exists for is untouched.
- **One property the new position must hold**: at the narrow window size used for
  032's screenshots the plate must still be **at least partly visible**, because
  *nothing changed size* has to stay checkable in that shot. Top-right is the obvious
  answer and is fine if it holds that; if being anchored far from the left edge makes
  the plate vanish entirely in a narrow window, pick a spot that does not. You have
  the picture in front of you and the choice is yours.

**Why this is in 040 rather than reported, since the coder was right to ask.** The
rule he weighed — *report it rather than fixing it* — is a **scope fence written
inside card 039, about `CLAUDE.md`, and it governs 039 only.** It is not a standing
project convention and it says nothing about this card. What the project does object
to is a repair **absorbed** into a card, widening its blast radius with nobody
deciding. **Writing the repair into the card before it happens is what removes that
objection**, which is exactly what this amendment is: decided, not absorbed.

**032 is not reopened and stays in `complete/`.** The defect is recorded against 040
because 040 is where it is fixed and where the screenshot that reveals it is taken. A
separate card for two constants would cost more ceremony than the fix and would leave
the demo unreadable in the meantime.

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
- ~~**Do not add a second timing period or a second average.** A draw count is an
  integer that is exact, not a sample to average. Showing last frame's value plainly
  is right; a running mean of it would be a worse number, not a better one.~~

  **Reversed by the principal, 2026-09-09, knowingly and after being shown this
  bullet's reasoning.** The draw count is a per-frame **metric** in the same shape as
  the durations — an average and a worst over the reporting period — not a snapshot.
  **The tech lead's bullet was right about today and wrong about the direction of
  travel**, which is the coder's phrase and is the better argument: this scene is
  identical every frame, so the average is 32.0 for ever — but the moment anything
  varies what is drawn, whether culling, streaming or an interface that grows, **the
  number a person needs is the worst frame in the period, not the typical one**. That
  is precisely why `frame` and `gpu` already carry a worst. A metric that only becomes
  interesting later is still the right shape now; a snapshot that has to be replaced
  later is not.

  **The record count beside it is deliberately not averaged**, and this is the
  interesting half. The claim is *this many records cost that few commands* — and a
  record count averaged over a period stops lining up with the commands beside it, one
  describing the period and the other a frame within it. **So the records stay the last
  completed frame's exact number, and the header says the line is a mixture on
  purpose.** Correct, and the reasoning is the same one that makes the average worth
  having: a number is only comparable to what shares its sample.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **A screenshot of the readout with the new line in it**, and the console block in
  the same run, so the two numbers can be read against each other. **Every line of the
  readout legible**, per the amendment above — that is the deliverable, not just the
  new line.
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
  output of this card and it is what the principal actually asked to see. **It is
  prose in your report, worked out once and stated, and not the on-screen line** —
  the amendment above explains why the same breakdown may not be computed
  structurally every frame. A figure a person reads once is allowed to be reasoned
  out; a number that appears sixty times a second is not.
- Anything the two numbers disagreeing revealed, if they did.

## Notes — claude-opus-5 (kanban-coder), 2026-09-09

### The line

```
draws    32 last  94 elements
```

Against the console from the same run:

```
draws      32 commands for 94 element records; the walk was 31 of them
```

**They cannot disagree, because they are the same two reads.** Both come from
`voe_render_frame_draw_count` and `voe_render_frame_elements_submitted` called
once each after `voe_render_frame_end`, stored in the timing struct; the console
block prints those fields rather than counting anything of its own.

**The wording for *previous frame* is the single word `last`.** Four glyphs, in
the column style, and it is the smallest thing that stops somebody chasing a
one-frame lag as a bug in two years. The alternative — a longer label like
`draws (prev)` — bought nothing and cost the column its shape.

**The first frame reads `draws  no frame yet`**, the gpu line's precedent. Seen
rather than reasoned about: a temporary one-shot `fprintf` in
`build_the_readout` printed `FIRSTFRAME[draws  no frame yet]` on a real run, and
was then removed.

### The breakdown, worked out once

A normal frame at the default window size is **32 draw commands for 94 element
records**:

| | |
|---|---|
| meshes | 29 — **one of which is the readout drawing itself** |
| the two panels | 2, issued *inside* the walk, sorted among the see-through meshes |
| the screen-filling surface | 1, issued after the walk |
| **the walk's own cost** | **31** |

Records: 80 on the world panel (40 rectangles and 40 letters), 5 on the badge, 9
on the screen-filling surface.

**Ninety-four records for three commands is the whole of ADR-0092**, and it is on
screen as a pair for the first time. This breakdown stays here in prose for the
reason the amendment gives: a figure a person reads once may be reasoned out; a
number that appears six hundred times a second may not.

### The counter was proved by moving it

Not trusted. The badge's submit was switched off, its range therefore went to a
count of nought, and both numbers moved by exactly the predicted amount:

| | commands | records | walk |
|---|---|---|---|
| as it ships | 32 | 94 | 31 |
| badge off | 31 | 89 | 30 |

One fewer draw because one fewer panel had anything to draw, and five fewer
records because that is what the badge submits. The change was reverted.

### The glyph budget

**83 glyphs in the steady state**, 332 of 512 transient vertices, against a
budget of `MAX_TRANSIENT_GLYPHS` 128. It was 65 before this card.

**The number the program prints is 80, and that is not the steady state.** The
readout's one-shot report fires on the first frame, where the draws line reads
`no frame yet` and is shorter than on every frame after it. I measured 83 by
temporarily delaying that report past frame four and then reverting. Nothing is
at risk — both are far under the budget — but the program under-reports its own
worst case by three glyphs. **Pre-existing and left alone**: it was true before
this card and is only slightly more misleading now that the line's length varies.
A register row at most.

### The renamed counters

`draws_before_elements` / `draws_after_elements` became **`draws_before_walk` /
`draws_after_walk`**, and the second one moved: it was being read *after*
`voe_dev_surface_draw`, so it did not bracket the walk that its old name implied
either. It now sits immediately after `voe_3d_draw_system_run`.

**The pair is kept rather than collapsed**, with the comment the card asked for:
the first is nought every frame today, because nothing is drawn between `_begin`
and the walk — building the readout's mesh is not drawing it — and the
subtraction stays correct the day something is.

### What must not change — checked

- **Nothing outside `dev`.** The diff is `dev/src/main.c` and `dev/dev.md`.
  `render`, `text`, `3d` and `ui` are untouched, and **no new call in `render`
  was wanted or added** — the card's instruction to stop and report if one was
  wanted is what produced the amendment above.
- **No second average and no second timing period.** The count is carried across
  whole as an integer; the struct comment says why averaging an exact number
  would make it worse.
- **The readout's own draw is not subtracted**, and both the struct and the
  file header say so.

### The plate that was covering the readout

Fixed here, under the 2026-09-09 amendment, and **the whole readout is legible
now** — that was the deliverable, not just the new line:

```
 615 fps
frame   1.63 ms
update  0.01 ms
draw    1.61 ms
gpu     0.04 ms
draws     32 last  94 elements
mouse away        ...
```

**What moved and what did not.** The plate and the marked square moved off the
top-left corner; the readout did not move at all, because it was there first. The
**ticks are untouched**, so 032's cut-off demonstration is exactly as it was.

**The plate is against the right edge and its x is anchored, not authored** — the
one thing on this surface that is. A plate authored at the right of a 240 mm
surface would sit at 188 mm and vanish entirely from a narrow window, and a plate
nobody can see says nothing about whether anything changed size. So it is placed
a fixed distance in from wherever the right edge turned out to be, exactly as the
bar along the bottom is placed from the bottom edge, with a clamp so a window
narrower than the plate cannot push it off the left. Its `y` is a plain authored
60 mm, well below where the readout ends at about 40.

**Checked in both shapes.** At 960×540 every readout line is clear and the plate
sits on the right. At 430×540 — 032's narrow shot — every readout line is still
clear, the plate is fully visible against the right edge, the marked square is
still square and the same size, and three of the six ticks have run off as
before.

### Found and left alone

- **The readout's own glyph report is not its worst case.** It fires on the first
  frame, where the draws line reads `no frame yet` and is shorter than on every
  frame after. It prints 80; the steady state is 83. Pre-existing, no risk
  against a budget of 128, and deliberately not folded in — this card is small on
  purpose and its own instruction is to stop it growing. Recorded as **D-195**.

### Verified on

`cmake -P check.cmake` exits zero on **Linux** — all steps, 38 tests, analyser
clean. Screenshots taken at 960×540 and at 430×540.

Windows is the principal's; nothing here is platform-shaped — one `snprintf`, two
`uint32_t` fields, two reads of calls that already existed, and one subtraction
against a number the surface already computed.
