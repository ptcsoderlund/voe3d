# 012 — two frames in flight

status: todo
claimed-by: -
blocked-by: -

Decided by ADR-0050, at the principal's direction. Today `frame.c` says so itself:
*"ONE FRAME IN FLIGHT, AND THAT IS THE WHOLE OF THE SYNCHRONISATION... two frames
in flight, a pool of command buffers — is frame pacing, and frame pacing is not
this card."* This is that card.

**This card's real deliverable is a pattern, not a frame rate.** Every card after
it adds per-frame resources — depth targets, uniform buffers, descriptor sets,
staging buffers — and each one will follow the shape this card sets. Getting the
shape right matters more than the two.

## Goal

The CPU may have two frames submitted and unfinished. It stops waiting for the
graphics card to finish frame N before starting frame N+1.

## The rules, from ADR-0050

- `VOE_RENDER_FRAMES_IN_FLIGHT` is **2**. **The number is not the decision** — the
  code reads the constant everywhere and never assumes its value, so triple is a
  one-line change with nothing to hunt for. A literal `2` anywhere is a bug.
- Every resource with a one-frame lifetime lives in an **array of that length,
  indexed by the frame slot**. The loop advances the slot modulo the constant.
- **Frames in flight is not the swapchain image count.** They are different numbers
  for different reasons. Nothing indexes one array with the other's index — that
  they are often both 2 or 3 is a coincidence that breaks on the first driver
  reporting a different minimum.

## The trap, and it is the whole card

**The two semaphore kinds have different lifetimes and the current code already
has it right. Do not flatten them.**

    fence, acquire semaphore    →  per frame SLOT
    rendering-finished semaphore →  per swapchain IMAGE

An acquire semaphore reused before its frame finished is a race that validation
layers do not reliably catch. It shows up as an intermittent hang on one driver
and not on others, which means the machine you test on may never see it.

Assert the invariant in debug builds rather than only documenting it. Nothing may
index a per-slot array with an image index. C proves nothing on its own, so the
check has to be written.

## Verify

**This card changes nothing you can see, so say how you know it worked.**

- Validation layers silent across many frames, and across resize and minimise.
- The fence wait is now "wait for the frame two ago", not "wait for the last one".
  Show that in the code or a comment — it is where the overlap comes from.
- `check.cmake` exits zero. `render` still configures and builds standalone.
- Windows is the principal's to run, as always.

## Not this card

Present mode and frame pacing. FIFO stays. `MAILBOX` versus FIFO is the *other*
thing "triple buffering" means, it needs honest timing numbers to argue from, and
those arrive with the frame-loop-and-timing card.
