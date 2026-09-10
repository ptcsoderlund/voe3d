# bug 002 — the element surface is still blank on Windows after card 044

status: new
found-by: the principal, 2026-09-10, running `voe_dev` on Windows with card 044's
  change in it
reported-by: claude-opus-5 (kanban-coder), who wrote card 044 and is therefore
  reporting against his own change
folder: `render`
severity: unchanged from bug 001 — everything mapped onto the window is
  invisible on Windows. The world is unaffected, and Linux is unaffected.

**It continues bug 001**, `kanban/bugs/archive/001-element-surface-blank-on-windows.md`,
which was archived when card 044 was written. **Whether 001 comes back to this
inbox is the tech lead's or the principal's call and has not been made here** —
card 044 is not abandoned, it is in `review/` and its change is sound on its own
ground (ADR-0111), so ADR-0110's reversal does not obviously apply. This is a new
number rather than an edit to an archived file for that reason.

## In the principal's own words

> Didnt work. Still empty spaces

## What happened

Card 044 moved the screen-filling element surface's clip-space z off the near
plane — `1.0f` to `0.9999f` in `voe_render_element_transform`,
`render/src/element.c` — on the hypothesis in bug 001 that a real driver was
discarding geometry standing exactly on the boundary. **The surfaces are still
not drawn on Windows.**

## What should happen

Unchanged from 001: six salmon ticks across the upper third, a dark plate with a
white square in it against the right edge, a dark bar along the very bottom, and
card 034's interface — a panel, a heading and two buttons — at the left below the
readout. All of it or none of it; they fail and succeed together.

## Update, the same day: z is ruled out completely

**The principal then tried three more values himself.** In his own words:

> Ive tried 0.9, 1.0, 0.1. Nothing works.

So `0.1`, `0.9`, `0.9999` and `1.0` all leave the window blank. **That closes the
whole near-plane line of enquiry**, and it closes bug 001's hypothesis with it:

- `0.1` is nowhere near a clip boundary. A driver excluding the boundary, or one
  with a precision window near it, would have drawn at 0.1.
- **Nor is it the depth test at any of those values.** Every missing rectangle in
  001's measurement sits over untouched background, where depth is the clear
  value 0.0 and the compare is GREATER, so a fragment at 0.1 passes as easily as
  one at 1.0. (At 0.1 the surface *would* be hidden by world geometry nearer than
  about 0.9 m — that is the trade the constant makes — but the pixels measured
  are not over geometry.)

**The matrix's z is therefore innocent, and `0.9999` should stay** on ADR-0111's
own ground: the value is right whether or not it fixes anything, and the test in
`render/tests/elements.c` pins it. Card 044 remains not at fault.

**What is left is the rest of the difference between a panel and a surface**, and
the next section is the experiment that halves it.

## The experiment that halves it, and it is one added line

In `dev/src/surface.c`, beside the existing draw, draw **the exhibit panel's
range through the screen-filling transform**:

```c
(void)voe_render_frame_draw_elements(
        gpu, voe_render_element_transform(millimetres), 0, 80);
```

Records 0..79 are the exhibit's, and they are drawn correctly on Windows every
frame through the draw system's matrix. This asks them to come out through the
matrix and at the point in the command stream that does not work.

- **The exhibit's rectangles appear stretched across the window** → the
  transform, the element pipeline and a draw issued after the walk are all fine,
  and the fault is in the later records or in the ranges that name them. The next
  question is then why records 85..121 differ from records 0..84.
- **Nothing appears** → the records are innocent and the fault is in the matrix
  or in the position of the draw. The next question is then whether a *panel*
  matrix would carry those same records onto the screen.

It is one added line, it removes nothing, and it must be reverted afterwards.

## Second update, the same day: THE FAULT IS REPRODUCIBLE IN A HEADLESS TEST

**`cmake -P check.cmake` fails on the principal's machine**, on an NVIDIA GeForce
RTX 4070 Laptop GPU, Vulkan 1.4.341, clang 22. Thirty-eight tests pass and
**`render/elements` fails**, in 0.39 seconds, **with no window, no swapchain and
no compositor anywhere near it**. That changes what this bug is: it is no longer
a thing only one pair of eyes can see on one screen. It is a failing test on the
machine that has the fault.

The four checks that failed, from his console:

```
FAIL  render/tests/elements.c:721
      count_in(image, HALF, HALF, SIDE, SIDE, IS_GREEN) == QUADRANT
      actual:   0        expected: 64
FAIL  render/tests/elements.c:723
      count_in(image, 0, HALF, HALF, SIDE, NEITHER) == QUADRANT
      actual:   0        expected: 64
FAIL  render/tests/elements.c:998
      count_in(image, 0, HALF, HALF, SIDE, IS_GREEN) == QUADRANT
      actual:   0        expected: 64
FAIL  render/tests/elements.c:1000
      count_in(image, HALF, HALF, SIDE, SIDE, IS_GREEN) == QUADRANT
      actual:   0        expected: 64
```

They are in two tests — `a_mesh_after_an_element_draw_is_still_right` and
`two_ranges_two_matrices_two_draws` — and **every failing check is about the
bottom half of the picture**. The checks about the top half passed in both, as
did `four_colours_in_one_draw`, which puts an element in all four quadrants
through a single draw starting at record nought.

**Card 044's constant is not what broke these.** In
`two_ranges_two_matrices_two_draws` both ranges carry the same z — the second
matrix is the first composed with a translation in x and y, so row 2 of the
product is unchanged — and one range drew while the other did not. A constant
that is the same for both cannot separate them. The same test also draws no
meshes at all, so nothing is occluding anything: depth is the clear value 0.0
everywhere and any z above it passes GREATER.

**What is common to the two failures and absent from every passing one is a draw
with a non-zero `first`.** `two_ranges_two_matrices_two_draws` draws `(0, 2)` —
which landed — and `(2, 2)`, which did not. `a_mesh_after_an_element_draw_is_still_right`
draws `(0, 1)` and `(1, 1)`. Every test that passes draws one range starting at
nought. And `first` is exactly what `dev` uses to give four surfaces four ranges
of one buffer: the exhibit starts at 0 **and draws on Windows**; the badge at 80,
the screen-filling surface at 85 and the interface at 94 **are the three that do
not**.

**Two things that would have explained it are checked and clean**, both by
disassembling what `slangc` actually emits rather than by reading the shader:

- The instance arithmetic reduces to the right thing. `SV_InstanceID` becomes
  `InstanceIndex - BaseInstance` and `SV_StartInstanceLocation` becomes
  `BaseInstance`, so the record index is `InstanceIndex`, which is what Vulkan
  says it should be.
- The module declares `OpCapability DrawParameters`, and
  `render/src/device.c:412` enables `shaderDrawParameters` unconditionally, so
  the built-ins it reads are legal ones.

## The diagnostic now in the tree, and what it will settle

`render/tests/elements.c` temporarily prints the sixteen-by-sixteen picture each
of those three tests reads back, one character per pixel. **Temporary, and to be
reverted with the finding.** On Linux, where everything passes, it prints exactly
what the tests claim:

```
map two_ranges_two_matrices_two_draws (want R B / G G)
map   RRRRRRRRBBBBBBBB      (x8 rows)
map   GGGGGGGGGGGGGGGG      (x8 rows)
```

The same map from the failing machine answers the question outright:

- **bottom half red and blue** → the second draw ignored `first` and drew the
  first range's records. That is the exact failure the test was written to catch
  and its header says so.
- **bottom half clear** → the second draw read records that are not there —
  out of range, or zeroed — and the zeroed clip rectangle discarded every
  fragment.
- **bottom half green** → the elements landed and something later took them
  away, which would move the enquiry to the readback rather than the draw.

## What is not yet known, and it is asked first because it is cheap

1. **Whether the binary that ran contains the change.** 001 established that
   habit and it is worth keeping: `render/src/element.c` newer than
   `libvoe_render` / `voe_render.lib`, and that newer than `voe_dev.exe`.
2. **Whether it is still all four groups or only some of them.** *Empty spaces*
   is consistent with both, and "the plate came back but the interface did not"
   would be a different fault entirely.
3. **What the console said**, in full. Validation layers are on when they are
   present, and the Vulkan SDK is the usual way to get `slangc` — so if they are
   installed on that machine, a validation line about the element draws is
   evidence nobody has looked at yet. The counts matter too: `draws` and
   `elements` in the readout, against 31 and 122 from Linux.

## What card 044 rules out, and what it does not

**It does not rule out the near plane.** `0.9999` is one ten-thousandth off the
boundary, which is enough for a driver that treats the bound as exclusive and is
*not* enough for one with a precision window near it. That is exactly why 044
named the next probe rather than iterating.

**It does rule out, or leave very weak, four things that would otherwise be the
obvious suspects — three of them on evidence in 001's own screenshot:**

- **The instance offset is ruled out.** The badge panel's range starts at
  `first = 80`, not nought, and the badge draws correctly on Windows. So
  `SV_StartInstanceLocation` reaching the shader and being added to
  `SV_InstanceID` is proven on that driver by a picture that works.
  `shaderDrawParameters` is also enabled unconditionally in
  `render/src/device.c:412`, so a device that lacked it would have failed to be
  created at all.
- **The element pipeline exists on that machine and both draws were recorded.**
  `voe_render_frame_draw_elements` refuses and records nothing when the pipeline
  is missing, and the Windows readout showed 31 draw commands against Linux's 31
  with the same 122 records. A pipeline that failed to build would also have
  printed `vkCreateGraphicsPipelines failed` at startup.
- **The push-constant path is weak.** The two panels push the same sixty-four
  bytes through the same shared layout in the same frame, immediately before
  their own draws, and they land. A layout that delivered rubbish would corrupt
  those too.
- **The x and y clip boundaries are not the same risk as z, so they are not the
  next place to look.** The surface's corners do sit exactly on `x = ±w` and
  `y = ±w`, but a quad whose interior is inside is never discarded by those
  planes. z was different precisely because *every* vertex was on it, leaving a
  clipper that wants one vertex strictly inside with none.

## The experiment that settles the first question, in one line

`render/src/element.c`, in `voe_render_element_transform`: **`0.9999f` to
`0.9f`**, rebuild, look, and **revert it whatever it does**. Card 044 says why it
must not ship: at `0.9` the overlay is occluded by anything within 11 mm of the
camera's near plane, against 0.01 mm at `0.9999`.

- **The surfaces appear** → the driver has a precision window near the boundary
  rather than excluding the boundary alone, and the question becomes what the
  permanent value is — a decision, and an ADR, not a patch.
- **They do not** → the matrix is innocent, `0.9999` should stay on ADR-0111's
  own reasoning, and the next question is the one below.

## If `0.9` changes nothing, the next thing to look at

The fault correlates with a *function*, not with a pipeline: everything drawn
through `voe_render_element_transform` is missing and everything drawn through
the draw system's world-to-clip matrix is present, in the same frame, through the
same pipeline, the same buffer and the same shader. If the numbers in that matrix
are not the cause, the way to find out what is, is to **draw the surface's own
records through a matrix that is known to work on that driver** — hand the
screen-filling range the exhibit panel's matrix for one frame. Same records, same
pipeline, same push constant, a matrix with a picture to prove it.

- **They appear** → it is the matrix after all and `0.9` was the wrong probe.
- **They do not** → nothing about the matrix is the cause, and what is left is
  the draw's position in the command stream: these two are the only draws issued
  after the draw system's walk.

The one structural difference nobody has tested is that every other matrix in
that frame has `m[3][2] = -1`, so `w = -z_view` per vertex, while this is the
only one where `w` is the constant 1 for every vertex.

## Every platform tried

**Windows: the fault is here.** The principal's machine, the same one 001 was
found on.

**Linux: does not reproduce, and this is the second time that is worth saying.**
WSL2 on lavapipe (llvmpipe, Vulkan 1.4.335). Under card 044 the same `voe_dev`
frame was captured with `1.0f` and with `0.9999f` and diffed: 316 of 518400
pixels differed, all of them the readout's own timing digits. Every surface drew
correctly at both values. **A software rasteriser cannot see this fault at all**,
which is what makes the principal's screen the only instrument there is.

## Which cards this does not impeach

**Card 044 is not at fault and should not be reopened.** Its deliverable was the
change, a test that pins the surface strictly inside the boundary, and the
finding if it did not work — this file is that finding, which the card asked for
in writing. `cmake -P check.cmake` is green on Linux with it.

**Cards 030, 032, 034 and 040 are not at fault either**, for the reason 001 gave:
each recorded that Windows was unchecked with no machine to check it on, and the
fault is older than all but the first of them.
