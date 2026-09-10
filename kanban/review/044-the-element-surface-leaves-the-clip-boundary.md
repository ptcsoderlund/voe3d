# 044 — the element surface leaves the clip boundary (bug 001)

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -
fixes: `kanban/bugs/archive/001-element-surface-blank-on-windows.md`

Written by the tech lead under the standing grant. **Not a spin-off**: it is the next
number. **The first card in this project that fixes a reported fault**, so it travels
the loop ADR-0108, ADR-0109 and ADR-0110 describe — the report is archived and this
card names it.

The decision behind this card is **ADR-0111**. Everything you need is restated here;
read the report for the evidence.

## Read this first: you probably cannot see this bug, and that is expected

**The fault does not reproduce on Linux.** Our Linux checking runs on lavapipe, a
*software* graphics implementation, and it draws the missing rectangles perfectly
(D-205). The fault is on the principal's Windows machine, on a real driver, and
**there are no agents on Windows** — he is the only tester there.

**So your half of this card cannot end with "I saw it working."** It ends with: the
change made, nothing regressed on Linux, and a test that pins the invariant. **The
principal's half is the confirmation.** That is the one-platform rule (2026-09-04)
working as written, and it is why this card's verify section looks unusual.

**And it may not fix the bug.** If his Windows screen is still blank afterwards, that
is a finding and not your failure — see *If it does not work* at the bottom.

## The theory

**What happens:** nothing submitted to the screen-filling element surface reaches
the screen on Windows. Card 032's plate, its six ticks, its bottom bar, and card
034's whole interface — all missing. Element **panels** in the world draw correctly
in the same frame, through the same pipeline, with the same depth state, through the
same `voe_render_frame_draw_elements`.

**What is already proved, in the report, and you do not need to redo it:**

- **The records went in and the draws were recorded.** 122 element records and 31
  draw commands on Windows, matching Linux exactly. Not a stale build, not a missing
  call.
- **The pixels were measured, not eyeballed.** Nine positions computed from
  millimetres, all holding the background colour exactly.
- **The depth test is innocent.** Depth clears to `0.0` (`VOE_RENDER_DEPTH_CLEAR`)
  and every missing rectangle sits on untouched background, so a fragment at 1.0
  passes `GREATER` trivially. **The primitive never reached rasterisation.**

**The one thing that differs between a panel that draws and a surface that does
not** is the matrix. A panel gets the draw system's world-to-clip matrix; the
surface gets `voe_render_element_transform`, and that function puts z on the near
plane:

    // render/src/element.c
    // Z: THE NEAR PLANE, WHICH IS 1.0 BECAUSE DEPTH RUNS BACKWARDS HERE.
    onto_the_target.m[2][3] = 1.0f;

**Work the composed matrix through and z and w are bit-exact 1.0.** Row 2 of the
product is `[0,0,0,1]` and row 3 is `[0,0,0,1]`, so both come out as 1.0 × 1.0 with
no arithmetic that could drift. So the surface's every vertex sits **exactly** on the
near plane, at `z == w`.

**Vulkan's view volume is `0 ≤ z ≤ w` *inclusive*, so a conformant driver keeps
it.** We are probably in the right. **That is not the point** — see ADR-0111. The
point is that the boundary is the one position in a whole continuum of valid ones
where two implementations may legitimately disagree, and it was chosen for tidiness
rather than for a reason.

## What to change

### 1. The constant, and the comment that explains it

`render/src/element.c`, in `voe_render_element_transform`: **`1.0f` becomes
`0.9999f`.**

**Rewrite the comment.** The existing one says the value *is* the near plane because
depth runs backwards, and that stops being true. The new comment must carry **why
this number and not 1.0, and why not something further back**, because a number
without that is one a future reader tidies back to 1.0 and reintroduces this bug in a
form nobody connects to it.

The arithmetic to put in it — reverse-Z with a finite far plane gives
`d(z) = (n/(f − n)) · (f/z − 1)`, and with the dev camera (`NEAR_PLANE 0.1`,
`FAR_PLANE 100`):

| z constant | nearest world geometry that would occlude the surface | exposed shell |
|---|---|---|
| `0.9999` | 0.100010 m | **0.01 mm** |
| `0.999` | 0.1001 m | 0.1 mm |
| `0.9` | 0.1111 m | 11 mm |

**Check that arithmetic against the real values rather than trusting the table** —
ADR-0105 applies to this card as much as to any other, and if my numbers are wrong
the comment must say the right ones. If the camera's near or far are not what I read
at `dev/src/main.c:586`, say so in your report.

### 2. The test, and it is the durable half of this card

**A plain arithmetic test on `voe_render_element_transform`: the composed clip-space
z of the surface's corners is *strictly less than* w.** Not `<=`. A matrix multiply
and a comparison — **no graphics card, no headless device, no window**.

**This is the part that outlives the fix.** Without it, the next person who wants the
overlay a notch further forward writes 1.0 and this returns. With it, doing so is a
red test with a message that explains itself. **Write that message for somebody who
has never read this card.**

## What must not change

State in your report that you checked each of these:

- **No depth clear is added**, and no pipeline is added or changed. Both were
  considered and rejected in ADR-0111 — a second full-screen clear per frame for
  every program with an interface, or a second pipeline, to solve what one constant
  solves. If you find yourself needing either, **stop and report**: that means the
  premise moved.
- **`voe_render_element_surface_matrix` is untouched.** The Y negation in it is the
  engine's one flip on this path and a second one is the classic invisible bug
  (ADR-0033 point 6, ADR-0099).
- **No public surface changes.** This is a constant, a comment and a test, all inside
  `render`.
- **Nothing in `dev`, `ui`, `text` or `3d`.** The fault is in `render` and so is the
  fix.
- **The reverse-Z convention itself stands.** Depth still clears to 0 and still
  compares `GREATER` everywhere; this card moves one overlay constant off a boundary,
  it does not revisit ADR-0033.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **The new test fails before your change and passes after.** Run it both ways and
  say so — a test that would have passed against `1.0` is not pinning anything, and
  that is exactly the class ADR-0105 exists for.
- **On Linux, everything that drew before still draws**, unchanged: the plate, the
  six ticks, the bottom bar, card 034's interface, the two panels, the models, the
  sprites, the readout. **A screenshot, because the risk of this change is a
  regression you cannot get from a test** — if the surface has moved behind something
  it used to cover, that is visible and nothing else would catch it.
- **State the exposure you computed** from the real near and far, and whether it
  matches the table above.
- **Windows is the principal's half.** Say in your report exactly what he should look
  for: the six salmon ticks across the upper third, the dark plate with the white
  square against the right edge, the dark bar along the very bottom, and card 034's
  panel with its heading and two buttons at the left below the readout. All of it or
  none of it — they fail and succeed together.

## If it does not work

**Do not iterate on it and do not widen the card.** If the principal reports Windows
still blank:

1. **Try `0.9f` as a probe and revert it before reporting.** It discriminates *the
   driver excludes the exact boundary* from *the driver has a precision window near
   it*, and those have different next suspects. **`0.9` must not be shipped** — the
   table above says why.
2. **Write a new bug report**, or add the finding to 001's archived file and say it
   moved back — the report returns to `bugs/` if this card did not fix it (ADR-0110
   point 2).
3. **The next suspects, in order**, so nobody starts from nothing: the element
   pipeline's own state as a whole (blend, raster, dynamic viewport) rather than the
   matrix; then whether the surface's draw is inside the frame's render pass at all
   on that driver; then the push-constant path, since the element pipeline pushes a
   matrix and a differing layout would silently deliver rubbish.

**This card is done either way.** Its deliverable is the change, the test and the
finding — not a working Windows screen, which is not in your hands.

## Report when this lands

- The exposure arithmetic from the real camera values, and whether the table was
  right.
- The test's message, quoted, so the next reader can judge whether it explains
  itself.
- Confirmation the test failed against `1.0` and passes against `0.9999`.
- The Linux screenshot, and whether anything moved that should not have.
- What you would look at next if Windows is still blank — your list, not mine, and
  say where you disagree with the three above.

## Notes — claude-opus-5 (kanban-coder), 2026-09-10

### What changed

Four files, all in `render`, and no signature anywhere changed:

- `src/element.c` — `onto_the_target.m[2][3]` is `0.9999f`, and the comment at it
  is rewritten to carry why it is not 1.0 and why not something further back.
- `include/render/device.h` — the doc comment on
  `voe_render_element_transform` said *z IS THE NEAR PLANE, WHICH IS 1.0* and
  *cannot be occluded*, both of which stopped being true. Comment only.
- `tests/elements.c` — the new test, plus the header paragraph naming its claim
  (twelve claims became thirteen).
- `render.md` — the `tests/elements.c` entry gained the claim, and *four of its
  claims need no graphics card* became five.

### The exposure, computed from the real values

`dev/src/main.c:586` is `NEAR_PLANE 0.1f` and `:587` is `FAR_PLANE 100.0f`, so
the card read them right. And `3d/src/projection.c` builds exactly the formula
the card assumed — `m[2][2] = n/(f-n)`, `m[2][3] = n*f/(f-n)`, `m[3][2] = -1` —
which for a point `t` metres in front of the eye gives
`d = (n/(f-n)) * (f/t - 1)`, inverting to `t = f / (1 + d*(f-n)/n)`.

| z constant | nearest geometry that would occlude the surface | exposed shell |
|---|---|---|
| `0.9999` | 0.1000100 m | 0.0100 mm |
| `0.999` | 0.1001001 m | 0.1001 mm |
| `0.9` | 0.1110988 m | 11.1 mm |

**The card's table was right** to the digits it gave. Nothing to correct.

Worth writing down beside it: this is the *dev* camera's answer, not the
engine's. The depth the surface carries is fixed at 0.9999, and how much room
that leaves in front of it is entirely the caller's near and far — a program with
`n = 0.01` would have a shell ten times shallower. The comment at the constant
says so.

### The test, and it was red first

`the_surface_stops_short_of_the_near_clip_boundary` in `render/tests/elements.c`.
Arithmetic only: `voe_render_element_transform`, four corners plus a fifth at a
second size with nothing in common with the first, and `clip.z < clip.w` —
strictly, never `<=`. No device, no window, so it runs on a machine with no
Vulkan at all.

**Against `1.0f` it fails six times** — the four corners, the fifth size, and the
existing `the_transform_puts_the_origin_at_the_top_left`, whose z expectation is
now `0.9999f` at a tolerance of `1e-6` so that the constant itself is pinned as
well as its side of the boundary. Run with the old constant in place:

```
FAIL  .../render/tests/elements.c:1187
      origin.z == 0.9999f
      actual:   1
      expected: 0.999899983
      off by 0.000100016594, tolerance 9.99999997e-07
      the top-left corner of a screen-filling element surface came out at z 1, w 1
      z must be STRICTLY less than w. At z == w every vertex of the surface
      stands exactly on Vulkan's near clip boundary. The view volume is
      0 <= z <= w inclusive, so a conformant driver keeps it — but a real
      driver discarded the whole surface there and nothing mapped onto the
      window reached the screen, while the same records drew perfectly
      through the same pipeline on another driver. The z constant in
      voe_render_element_transform belongs just inside the plane and not on
      it; the comment at that constant says why 0.9999 and why not something
      further back.
FAIL  .../render/tests/elements.c:1222
      clip.z < clip.w
      the top-right corner of a screen-filling element surface came out at z 1, w 1
FAIL  .../render/tests/elements.c:1222
      clip.z < clip.w
```

The nine-line explanation is printed **once** per run and each corner then costs
two lines; one constant decides every corner, so the real failure is all five at
once and saying the paragraph five times would bury the numbers that differ.

**Against `0.9999f` it passes**, inside a `check.cmake` that exits zero.

### check.cmake

Green on Linux, on the final tree, 46 s:

```
ok    tests (39 passed — 3d 6, assets 6, base 2, dev 0, ecs 3, math 5, platform 2,
      render 6, scene 3, sprite 1, text 3, ui 2)
ok    analyser (106 files)
```

All steps ok, no analyser finding, no suppression added.

### The screenshot, and the regression it was taken to catch

WSL2, lavapipe (llvmpipe, Vulkan 1.4.335), `voe_dev` at 960x540, the real program
and the real frame — not a headless re-enactment of one call. The compositor here
still offers no screenshot protocol, so the frame was taken by a throwaway probe
**in the scratch mirror only**: it grabs `voe_render_frame_current` before
`_begin`, copies the colour target after `_end` on frame 300, writes BGRA and
exits, and it pins the scene's step to 1/240 s so that two runs land on the same
world. Nothing of it is in this repository or in the diff. The pictures are at
`~/voe3d-scratch/capture/044-after-0.9999.png` and `044-before-1.0.png`.

Everything the card lists is there and unchanged: the six salmon ticks across the
upper third, the dark plate with the white square against the right edge, the
dark bar along the very bottom, card 034's panel with its heading and two buttons
at the left below the readout, the readout itself, both element panels, the
models, the cubes, the sprites and the sign.

**And the picture was diffed rather than looked at.** The same frame captured
with `1.0f` and with `0.9999f`: **316 of 518400 pixels differ, all of them inside
x 41..119, y 15..121** — the readout's own timing digits, which are different
numbers on two different runs. Every other pixel is bit-identical. **Nothing
moved, nothing went behind anything, no edge shifted.**

The console from the same run, which is also the report's two numbers:

```
draws      31 commands for 122 element records; the walk was 29 of them
```

### What must not change — checked

- **No depth clear added and no pipeline added or changed.** `element.c`'s
  pipeline description is untouched; the diff in that file is the one constant
  and comments.
- **`voe_render_element_surface_matrix` untouched.** Still the one negation on
  this path; no second flip anywhere.
- **No public surface change.** No signature, no new function, no new type. The
  header changed in a comment that had gone false.
- **Nothing in `dev`, `ui`, `text` or `3d`.** The diff is four files in `render`.
- **Reverse-Z stands.** `VOE_RENDER_DEPTH_CLEAR` is still 0, the compare is still
  `GREATER`, and `depthTestEnable`/`depthWriteEnable` on the element pipeline are
  as they were.

### Markers

None. No `DEVIATION:` and no `BLOCKED:` were needed.

### What I would look at next if Windows is still blank

Agreeing with the card first: **try `0.9f`, look, and revert it**. It separates
*this driver excludes the exact boundary* from *this driver has a precision
window near it*, and it is one character.

Where I would go after that differs from the card's list, because two of its
three suspects are already contradicted by the report's own evidence:

- **The push-constant path is weak.** The panels push the same sixty-four bytes
  through the same shared layout in the same frame and land correctly on Windows.
  A differing layout would corrupt them too.
- **The instance offset is ruled out, and this is worth saying because it is the
  attractive suspect.** The badge's range starts at `first = 80`, not nought, and
  the badge draws on Windows — so `SV_StartInstanceLocation` reaching the shader
  is proven on that driver by a picture that works.
- **The x and y boundaries are not the same risk as z, so do not chase them.**
  The surface's corners sit exactly on `x = ±w` and `y = ±w` as well, but a quad
  whose interior is inside is never discarded by those planes. z is different
  precisely because *every* vertex is on it, so a clipper wanting one vertex
  strictly inside has none.

So, in order:

1. **`0.9f`, then revert.** As above.
2. **If `0.9` also draws nothing, the matrix is innocent and the discriminator is
   to draw the surface's own records through a matrix that is known to work.**
   Give the screen-filling records the exhibit panel's range for one frame — same
   records, same pipeline, same push constant, a matrix that demonstrably
   survives that driver. If they appear, the fault is in
   `voe_render_element_transform`'s numbers after all and `0.9` was not the right
   probe; if they still do not, nothing about the matrix is the cause and the
   next line of enquiry is the draw's position in the command stream.
3. **Then the one structural difference left.** Every other matrix in the frame
   has `m[3][2] = -1` and therefore `w = -z_view` per vertex; this is the only
   one where `w` is the constant 1 for every vertex. That is a real difference in
   what reaches the clipper and the perspective divide, and it is the last thing
   the two paths do not share.
4. **Only then the pipeline state as a whole**, which is where the card put it —
   and note that the viewport and scissor are set once at `_begin` in
   `frame.c` and by nothing afterwards, so a state left behind by the walk is not
   available as an explanation.

**And whichever way it goes, this card's change stands**: standing exactly on the
boundary bought nothing, and the test is the part that outlives the answer.

### Platform

**Linux only, and by the card's design.** WSL2 on lavapipe, `check.cmake` zero,
the picture above. **Windows is unchecked and is the principal's half** — the
fault does not reproduce here and there is no machine to see it on. What to look
for is the list in *The screenshot* above: all of it or none of it, they fail and
succeed together.
