# 0078. Anti-aliasing is not built, and cut-out edges crawl

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amended 2026-09-06, same day:** the *Text is unaffected* bullet in the decision was overtaken hours after this was written. The principal, shown what edge smoothing costs during the review of card 025, instructed its removal **everywhere** — *"Everywhere. We dont want it, period. It is irrelevant if it makes it look like an engine from 1995"* — and card 026 carried it out. **No part of the decision below changed**; what changed is its reach, which grew to include the one place this ADR had carved out. The bullet is corrected in place and the instruction recorded. ADR-0079 then found that the same sweep point-sampled the glyph sheet, which is a defect and not part of what was instructed.

## Context

D-096 asked **when** anti-aliasing leaves the *later* list and **which kind**. It
had been pointed at three times by three unrelated decisions:

- **ADR-0072** found that transparency's visible quality is capped by the engine
  having none — a cut-out leaf, fence or grate has a crawling edge and nothing
  can soften it.
- **ADR-0075** removed the mipmap chain from the glyph sheet, taking away the
  blur that was hiding small-text degradation (D-104).
- **ADR-0077** declined a sharpening pass partly because sharpening is a
  high-pass filter and aliasing is high-frequency error, so it amplifies exactly
  what the engine cannot fight.

The principal asked the question the register had never put plainly: **what does
skipping it entirely actually cost?**

The honest answer, as argued:

- **One thing genuinely breaks: cut-out edges.** Foliage, fences, grates, hair —
  a hard binary cut per pixel. The real fix is alpha-to-coverage, which
  **requires** multisampling to exist. Resolution does not help; a leaf edge at
  4K crawls finer, not less. **The capability table promises cut-out edges in
  version one.**
- **Thin geometry flickers** — railings, wires, distant poles popping in and out
  as the camera moves.
- **Silhouettes get the staircase**, which is the least of the three and the one
  most helped by rendering at native resolution — now the default under ADR-0077.
- **The distinction that matters: static aliasing is tolerable, moving aliasing
  is not.** A still frame with jaggies looks fine; the same scene with the camera
  moving shimmers, and that is what the eye objects to.

What anti-aliasing would **not** have fixed, so neither counts as a reason for
it: **specular shimmer** on glossy surfaces, because multisampling supersamples
coverage and not shading — that is cured by filtering material maps; and
**texture shimmer**, already handled by the mipmap chain everywhere except the
glyph sheet, where ADR-0075 turned it off deliberately.

What it would have cost: at 4K, four-sample multisampling takes the colour and
depth attachments from about **66 MB to 265 MB**, plus the bandwidth to write and
resolve them, **every frame whether the scene needs it or not** — which is
precisely the always-on cost ADR-0066 exists to refuse. The cheap post-process
kind works by blurring and would fight the exact glyph edge ADR-0076 just bought.
The temporal kind needs motion vectors, camera jitter and a history buffer, and
brings ghosting of its own.

## Options considered

### Option A — multisampling plus alpha-to-coverage
The correct fix for the one thing that actually breaks. Frame-priced, large
attachment memory, and it lands as a setting under ADR-0064 rather than as a
default.

### Option B — a post-process pass
Under a millisecond at 4K, one full-screen quad, no attachment cost. It works by
finding edges and blurring across them, which softens text and fine detail and
would work against ADR-0076. It also does nothing for the crawling that motion
causes, because it has no memory of the previous frame.

### Option C — build none of it
The engine ships with no anti-aliasing of any kind. Picture quality rests on
native-resolution rendering, which ADR-0077 has just made the default and
declined to let anything scale down without the program asking.

## Decision

**Option C.** The principal's reason, recorded in his words: *"if it's not a
problem now we dont fix it now. We can live without it and performance benefits
from not having it."*

- **Nothing is built and nothing is scheduled.** D-096 closes as **no**, not as
  *later* — a row that has been pointed at three times and deferred three times
  is not deferred, it is being avoided, and closing it is more honest than
  carrying it.
- **Cut-out edges stay in version one and their edges crawl.** That is now an
  **accepted and stated limitation**, not an unmet promise. The capability table
  says so in the principal's own view, because a promise that ships with an
  asterisk nobody wrote down is the thing that generates the complaint.
- **This and ADR-0077 support each other.** No upscaling, no anti-aliasing,
  render at native and accept the edges. Both are the same posture: the engine
  spends nothing the program did not ask for, and picture quality comes from
  rendering honestly rather than from correction passes.
- **Text was carved out here and the carve-out did not survive the day.** As
  written, this bullet said text was unaffected because ADR-0076's distance field
  carries its own smoothing, and that it remained the only anti-aliasing in the
  engine. **The principal removed it there too**, hours later and on the
  instruction quoted in the amendment note above, so `smoothstep` across one screen
  pixel became `step(0.5, field)` — a hard cut, no partial coverage, no
  exceptions. **There is now no anti-aliasing anywhere in the engine, text
  included**, and that is the stronger and simpler form of what this ADR decided.
  What the distance field buys is not a soft edge: it is an edge in the right place
  at any size, which is what makes a hard cut look like a letter rather than like
  blocks. **That last clause is load-bearing and was briefly untrue in practice** —
  see ADR-0079.

## Blast radius

**Contained, and cheaper to reverse than it looks.** Multisampling is chosen at
target creation, every pipeline must declare a matching sample count, and a
resolve is needed on the way out. That is **one card in `render`** touching
target and pipeline creation — not a redesign — and ADR-0051's offscreen target
with a separate present step is already exactly the right seam for the resolve.

What would make it expensive is something coming to **depend** on single-sample
targets — a post-process that reads the depth buffer per pixel, for instance,
which behaves differently when the buffer is multisampled. Nothing does today.
**If that ever changes, this decision gets harder to reverse and the ADR that
does it should say so.**

Reversibility: **moderate**, and unusually well-preserved for a decision to build
nothing.

## Consequences

- **Vegetation, fences and grates will look bad in motion, and there is no lever
  to pull.** Stated plainly because it is the one real cost and it is in version
  one. A game built on this engine that is mostly foliage will look worse than
  the same game on an engine that has this, and no setting will change it.
- **Thin geometry flickers.** Railings, wires and distant poles pop in and out
  with camera movement.
- **Small text keeps the degradation D-104 names**, and now has no anti-aliasing
  answer available to it either. D-104's candidate list shrinks to the ones
  inside `text`.
- **The engine's picture quality rests entirely on output resolution**, which is
  the program's to choose and defaults to native. Deciding this immediately after
  ADR-0077 is not a coincidence: having refused to blur the frame, the engine has
  also refused to correct it.
- **This closes the third of three pointers, and closes the subject.** The tech
  lead raised it three times; the correct framing was always that it is a gap for
  exactly **one kind of content**, and the principal has now said that content is
  acceptable as-is. It should not be raised a fourth time on general grounds —
  only on the named condition below.

## Rejected options and why

- **Option A — multisampling with alpha-to-coverage.** Not rejected as wrong; it
  is the correct fix for the one thing that breaks, and it is what this ADR would
  choose if the answer were yes. Rejected on cost against benefit *today*: the
  content that needs it does not exist yet, and the cost is charged to every
  frame of every program including the ones with no cut-outs in them at all.
- **Option B — a post-process pass.** Rejected on quality direction rather than
  cost. It buys smoother silhouettes by blurring, in an engine that has just
  spent two decisions making one class of edge exact. Trading ADR-0076's glyph
  edge for softer polygon edges is the wrong way round.

## Questions this opens

- **D-110** — **the named condition that reopens this**, so it is reopened by a
  fact rather than by taste. The strongest candidate is **headsets**: aliasing is
  far worse in a headset than on a monitor, because the optics magnify every
  pixel and the head never stops moving, so the *moving* aliasing that this ADR
  accepts is the dominant artifact there rather than a minor one. The principal
  has already shaped one decision around headsets (ADR-0074). Secondary
  candidates: a program whose content is majority cut-out, or the editor. Trigger:
  the first of those to become real.
