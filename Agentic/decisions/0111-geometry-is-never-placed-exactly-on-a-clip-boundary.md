# 0111. Geometry is never placed exactly on a clip boundary

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** nothing. Opens D-211, D-212. **Motivated by bug 001, and deliberately not justified by it** — see below.

## Context

Bug 001: nothing submitted to the screen-filling element surface reaches the screen
on Windows, while element panels draw correctly in the same frame through the same
pipeline. The coder isolated it to the one thing that differs — the matrix — and
named `render/src/element.c`:

    // Z: THE NEAR PLANE, WHICH IS 1.0 BECAUSE DEPTH RUNS BACKWARDS HERE.
    onto_the_target.m[2][3] = 1.0f;

**The tech lead's review added three things.** The depth test is innocent: depth
clears to `0.0` and every missing rectangle sits on untouched background, so a
fragment at 1.0 would pass `GREATER` trivially — the primitive never reached
rasterisation. The z and w that reach clip space are **bit-exact 1.0** (row 2 of the
composed matrix is `[0,0,0,1]`, row 3 is `[0,0,0,1]`, so the product is 1.0 × 1.0),
which rules out drift across the boundary and leaves a driver treating an inclusive
bound as exclusive. And `depthClampEnable` is unset, so clipping is on.

Vulkan's view volume is `0 ≤ z_c ≤ w_c` **inclusive**, so a conformant driver keeps
geometry at `z == w`. **We are probably in the right and it does not matter.**

## The reason this ADR is not "the fix for bug 001"

**The diagnosis is unconfirmed.** It cannot be confirmed on the verification machine
— the fault does not reproduce under lavapipe (D-205) — and it needs the principal's
Windows machine. **This morning the tech lead recommended an engine change on a
premise he had not checked and was overruled for it (ADR-0106); repeating that
pattern the same day would be worse than the first time.**

So this decision is taken on a ground that *is* checkable and does not depend on the
diagnosis at all:

> **Standing exactly on a clip boundary buys the engine nothing, and it is the one
> place where two conformant implementations may legitimately disagree about a
> pixel.**

The overlay does not need to be *at* the near plane. It needs to be **in front of
everything**, and there is a continuum of positions that satisfy that. Choosing the
single boundary value out of that continuum was a free choice made for tidiness, and
it is the only value with a divergence risk attached. **If Windows still fails after
this change, the rule is still right and bug 001 stays open with a new suspect** —
that outcome is a finding, not a failure of this ADR.

## Decision

1. **No engine code places geometry exactly on a clip boundary.** Not `z == w`, not
   `z == 0`, not `x == ±w`. Where a constant is chosen for "as far forward as
   possible", it is chosen strictly inside the volume with a stated margin.

2. **The element surface's clip-space z becomes `0.9999`**, replacing `1.0`, with the
   exposure written in the comment rather than left to be re-derived.

3. **The margin is justified by arithmetic and not by taste.** Reverse-Z with a
   finite far plane gives depth `d(z) = (n/(f − n)) · (f/z − 1)`. With the dev
   program's camera (`NEAR_PLANE 0.1`, `FAR_PLANE 100`):

   | z constant | nearest world geometry that would occlude the surface | exposed shell |
   |---|---|---|
   | `1.0` | nothing — but it is the boundary | none, and it is the bug |
   | `0.9999` | 0.100010 m | **0.01 mm** |
   | `0.999` | 0.1001 m | 0.1 mm |
   | `0.9` | 0.1111 m | 11 mm |

   `0.9999` is three orders of magnitude clear of the boundary as a float and leaves a
   ten-micrometre shell just past the near plane in which world geometry could cover
   the interface. **Nothing can occupy that shell in practice**, and anything that
   nearly touches the near plane is already clipped or degenerate.

4. **A test pins it, and the test is the durable half.** A plain arithmetic test on
   `voe_render_element_transform` asserting that the composed z is **strictly less
   than** w for the surface's corners. No graphics card: it is a matrix multiply.
   **Without it, the next person who wants the overlay one notch further forward puts
   it back on 1.0 and the bug returns in a form nobody connects to this.**

5. **`0.9` is the diagnostic and must not be shipped.** If the Windows run still
   fails at `0.9999`, trying `0.9` distinguishes *the driver excludes the exact
   boundary* from *the driver has a precision window near it* — a distinction that
   changes the next suspect. It is a probe, reverted before reporting (ADR-0110's
   test for a fix versus an investigation).

## Blast radius

**One constant, one comment and one test, all inside `render`.** No public surface
changes, no pipeline changes, no shader changes, no other folder.

Reversibility: **cheap**, and point 4's test is what makes reversing it deliberate
rather than accidental.

## Consequences

- **`element.c`'s comment becomes wrong and is rewritten.** *"THE NEAR PLANE, WHICH
  IS 1.0 BECAUSE DEPTH RUNS BACKWARDS HERE"* is no longer what the code does. The new
  comment carries the exposure table's reasoning, because the number is otherwise
  unexplainable and would be "tidied" back to 1.0 by a future reader.
- **This may not fix bug 001**, and the card says so plainly so that a coder does not
  read a still-blank Windows screen as their own failure.
- **The rule in point 1 is broader than the one site it changes**, and nothing sweeps
  the engine for other boundary constants. D-211.
- **The consequence I do not like:** a magic number with a rationale is still a magic
  number. **The structural fix is available and was not taken** — clear depth
  immediately before the surface, as ADR-0074's overlay layer already does, and then
  any z beats an empty buffer and no boundary reasoning is needed anywhere. Rejected
  below on cost, but it is the answer if this site ever needs revisiting, and D-212
  carries it rather than leaving it as folklore.

## Rejected options and why

**A depth clear before the surface, then any z.** The structurally clean answer, and
it uses machinery that exists: `voe_render_frame_clear_depth` is public and
`3d/src/draw_system.c:522` already clears between the world and the overlay for
exactly this reason. Rejected for now because a full-screen depth clear is real work
that the draw system's own comment already declines to spend when nothing needs it —
*"a world with nothing above it should not pay for one"* — and this would add a
second one per frame for every program with an interface, to solve a problem one
constant solves. **It becomes the right answer the moment the surface needs to be
robust against arbitrary world geometry rather than against a ten-micrometre shell.**

**Disabling the depth test for the surface's draw.** Cleanest of all in principle: a
screen-filling interface has no depth question. Rejected because the element pipeline
is shared with world panels, which do need the test (ADR-0093, ADR-0074), so this
means a second pipeline or dynamic state from an extension — both far more than a
boundary is worth, and a second pipeline is what ADR-0083 refuses.

**Leaving `1.0` and waiting for the Windows confirmation first.** Rejected because
the change is justified without the confirmation, and because the confirmation
requires somebody to edit the constant anyway — so waiting means doing the same edit
twice and shipping neither.

## Questions this opens

- **D-211** — whether any other engine constant sits on a clip boundary.
- **D-212** — the depth clear as the structural answer, if this site returns.
