# 0138. A drain corrects a bad value or keeps the last valid one, and says so

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-242**, opened by ADR-0134: *what an owning system does with a replace intent
carrying a value that breaks its invariant — keep the old row, correct the value, or
assert.* ADR-0134 point 5 moved a system's checks into its drain, because the inspector
submits generically and never calls a component's typed functions. ADR-0136 makes the
first such value real: a dragged rotation slowly drifts out of unit length.

What is on disk, read the same day. **A drain cannot tell who submitted**: the editor and
game code share one queue. `VOE_BASE_ASSERT` survives every build (`base/assert.h`), and
`3d/src/projection.c` uses it on a field of view outside (0, π), a near plane at or behind
the eye and a far plane not beyond the near one — so a person dragging a near plane to zero
**would stop the editor at draw time**, whatever the camera's drain did. **The engine
already corrects in two places**: `scene/src/camera_system.c` clamps pitch short of straight
up, and `3d/src/normal_matrix.c` answers a zero scale — *"a thing a file may contain"* —
with the identity rather than asserting. The light asserts on a zero-length direction in
its add and typed submit, and promises readers a unit direction. **A cooked game loads rows
by copy** (ADR-0128) and never drains them. **There is no logging facility**: `render`,
`assets` and `ui` write a line to stderr where the problem is, and `base/assert.h` says how
recoverable failures are reported *"is not decided yet"*.

## Options considered

### Option A — keep the old row
Any intent breaking an invariant is dropped. A rotation drifting by rounding is refused and
freezes mid-drag.

### Option B — correct where there is one right answer, otherwise keep the old row
Normalise, clamp to the nearest valid value, and keep the last valid row where no nearest
value exists. Typed calls keep asserting.

### Option C — assert, and the editor prevents
Descriptions carry ranges the inspector enforces. Cannot express a rule across fields — far
beyond near — so those still stop the editor.

## Decision

**Option B, with the principal's amendment**: *"clamp or keep latest correct. Print warning,
info or error so dev can see they did wrong. If possible."* The deciding factor: **a drain
cannot tell a program's value from a person's, and the only answer right for both is the one
that always leaves a valid row behind** — which is what pitch and the normal matrix already
do.

1. **A component's typed calls keep asserting.** Its add and its typed submit are called by
   program code, and a bad value there is the program's bug (rule 13). They stay loud.
2. **Its drain settles every intent before applying it.** Where one nearest valid value
   exists it applies that — a rotation or direction normalised, a value clamped into its
   range. **Where none exists it keeps the last valid row** — a zero-length direction has no
   nearest direction.
3. **Each rule is written in the component's header** with its answer: the correction, or
   *keeps the last valid row*.
4. **A settled intent is reported as one line on stderr**, the way the engine already
   reports recoverable problems. It starts with **`warning:`** when the value was corrected
   and **`error:`** when the last valid row was kept, and names the system, the entity — by
   its identity's name where it has one (ADR-0137), else its index and generation — the
   field, the value submitted and what was applied.
5. **Rounding is not a mistake and is not reported.** Restoring unit length within a
   tolerance the component states is silent. Only a correction beyond it warns.
6. **It is reported on the edge, not per intent.** A system prints when a run of settled
   intents begins, and a count when the run ends; a run is consecutive drains that settled
   at least one. A drag through a bad value prints two lines, not sixty a second. The flag
   and the count are the system's diagnostic state, not component data.
7. **No facility and no further levels.** `info` has no caller here; a real reporting
   facility is its own question (below).

## Blast radius

**Moderate.** Every drain of an editable component is written this way, and point 4's line
is the contract a later reporting facility must replace rather than sit beside. Moving to A
is deleting corrections; moving to C adds ranges to every description.

## Consequences

- **The editor cannot stop the program with a value**, including a lens that would have
  tripped `projection.c`, once the camera's drain clamps into what the projection accepts.
- **What these drains let through is what ships**, because a cooked game never drains. For
  authored data this is the only check there is.
- **The light's check splits in two**: asserting in its typed calls, settling in its drain.
- **The transform's drain normalises its rotation**, so `math` gains a quaternion normalise
  on that card (rule 10). Its scale has no rule; the normal matrix already copes with zero.
- **A loader must bring authored values in through the same settling**, not through the
  asserting typed calls — a text scene is hand-editable (ADR-0073). The load card's to do.
- **A program that goes around the typed calls** — a bridge in another language submitting
  generically — **gets warnings rather than a crash.** Accepted: it is still told.
- **Two new words on stderr** with no facility behind them. Deliberately small; see below.

## Rejected options and why

**A — keep the old row.** It refuses the case the editor meets first, a rotation off by
rounding, and freezes the drag for a value with an obvious correction.

**C — assert, and the editor prevents.** Rules across fields cannot be ranges on one field,
so the editor would still stop on the most ordinary lens mistake, and every description
would grow constraints to buy that.

## Questions this opens

- **D-247** — how the engine reports a recoverable problem: whether `base` gains a reporting
  facility with levels, where its lines go — stderr, a file, the editor's own console — and
  whether a shipped game prints at all.
