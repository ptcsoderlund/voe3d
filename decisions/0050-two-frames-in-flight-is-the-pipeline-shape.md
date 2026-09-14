# 0050. Two frames in flight, and every per-frame resource is indexed by frame slot

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Amends:** —
- **Superseded by:** —

## Context

The principal's direction: *"We want double buffering and maybe even triple
buffering, early. It should be standard in our render pipeline (double)."*

**"Double buffering" is three independent knobs in Vulkan**, and conflating them
is the usual way this gets built wrong. As shipped by the cleared-frame and
triangle cards:

| Knob | What it is | Today |
|---|---|---|
| Swapchain image count | How many images the window rotates through | `minImageCount + 1` — 3 on most drivers, 2 at worst. **Done.** |
| Present mode | Whether presentation waits for the display | `FIFO` — every driver must support it, so no fallback exists to write |
| **Frames in flight** | How many frames the CPU may have submitted and unfinished | **One.** |

Only the third is architectural, and `render/src/frame.c` says so at the top in
its own words: *"ONE FRAME IN FLIGHT, AND THAT IS THE WHOLE OF THE
SYNCHRONISATION... Anything more — two frames in flight, a pool of command
buffers — is frame pacing, and frame pacing is not this card."* The coder scoped
it out and named it rather than guessing, which is what ADR-0034 asks for.

One frame in flight means the CPU submits, then waits for the GPU to finish
before starting the next frame. It is correct and it is slower than it needs to
be: the two take turns instead of overlapping.

**Why this cannot wait.** N frames in flight means every resource whose lifetime
is one frame must exist N times, in an array indexed by the frame slot: command
buffers, fences and acquire semaphores today; uniform buffers, descriptor sets,
staging buffers and any offscreen render target as soon as real geometry arrives.
Build the renderer with one of each and retrofitting N later means touching every
resource-owning file in `render`. Establish the pattern now and every later card
follows a shape that already exists. `render` is currently five source files.

## Decision

**Two frames in flight, as a named constant, and no per-frame resource is ever
single-instanced.**

1. `VOE_RENDER_FRAMES_IN_FLIGHT` is 2. **The number is not the decision** — the
   decision is that the code reads the constant everywhere and never assumes its
   value, so triple buffering is a one-line change with nothing else to find.
2. **Every resource whose lifetime is one frame lives in an array of that
   length**, indexed by the current slot. The frame loop advances the slot
   modulo the constant.
3. **Frames in flight is independent of the swapchain image count and the code
   may not conflate them.** They are different numbers for different reasons and
   nothing may index one array with the other's index.
4. **The two semaphore kinds keep different lifetimes**, because this is the
   classic bug and the current code already has it right: the fence and the
   *acquire* semaphore are **per frame slot**; the *rendering-finished* semaphore
   that present waits on is **per swapchain image**. An acquire semaphore reused
   before its frame completed is a race that validation does not always catch and
   that appears as an intermittent hang on one driver only.
5. **Present mode and frame pacing are deliberately not decided here.** FIFO
   stays. Whether to prefer `MAILBOX` for latency — the other thing people mean
   by "triple buffering" — is a pacing question that needs honest timing numbers
   to argue from, and the frame-loop-and-timing card is where those first exist.

## Blast radius

**Cheap now, expensive in proportion to how much renderer exists when it is
done.** Today it is one file's worth of arrays and a slot counter. After model
loading, textures and materials it is every one of those plus their descriptor
sets and uniform buffers.

Reversibility of the *number*: trivial, by design — that is what point 1 buys.
Reversibility of the *pattern*: it is not something you reverse, it is something
you would have to add.

## Consequences

- **A card that adds a per-frame resource has one more thing to get right**, for
  the life of the project. That is the cost, it is permanent, and it is the point:
  the alternative is one big card that finds every one of them at once, later.
- **`voe_render_frame` gains a slot index** and the frame loop advances it. The
  fence wait stops being "wait for the last frame" and becomes "wait for the
  frame two ago", which is where the overlap comes from.
- **The CPU and GPU overlap**, which is the whole gain, and it will not be
  visible as a number until the timing card exists. Accepted: this is taken on
  structure, not on a measurement, and ADR-0020's "no measured gain means it
  stays on the CPU" does not apply — this is not moving work to the GPU.
- **A latency cost that is real and worth knowing**: two frames in flight means
  input is up to one extra frame old on screen. Standard, and the reason `MAILBOX`
  and pacing exist as a later question.
- **Slightly more memory**, twice per per-frame resource. Unnoticeable at v1
  scale, and the same trade ADR-0024 already accepted for a second render target.
- **It makes the offscreen-frame-target question cheaper to say yes to** (register
  row D-053): under this ADR an offscreen target is just another per-frame
  resource following an established pattern, rather than new plumbing.
- **A debug build should assert the invariant**, not merely follow it — nothing
  may index a per-slot array with a swapchain image index. Bevy's automatic
  parallelism is safe because Rust proves it; C23 proves nothing, so the check
  has to be written. The register's standing note on that applies here.

## Rejected options and why

**Leave it at one frame in flight until something is measurably too slow.** This
is normally the right instinct and it is what ADR-0020 says about GPU work. It is
rejected here because the cost being avoided is not compute, it is a pervasive
structural property: the measurement would arrive long after the renderer was
large enough to make the change expensive. Deciding early is exactly the case
ADR-0034 carves out — *"taking a decision early is right whenever it changes what
existing code looks like."*

**Three frames in flight now.** Rejected as the *default* only: the principal said
double, three adds a third frame of latency, and point 1 makes the change a
single line when there is a reason. Nothing is lost by starting at two.

**Tie frames in flight to the swapchain image count.** Rejected: it is a
coincidence that they are often both 2 or 3, and code that assumes it breaks on
the first driver that reports a different minimum. Point 3 forbids it explicitly.

## Questions this opens

- **Present mode and frame pacing** — FIFO versus `MAILBOX`, and what the frame
  loop does when it has spare time. Needs the timing card first. Register row
  D-054.
- Whether the per-slot invariant is asserted in debug builds or only documented.
  Recommended asserted; decided by the card that writes it.
