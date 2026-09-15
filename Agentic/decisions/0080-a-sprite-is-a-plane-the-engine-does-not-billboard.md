# 0080. A sprite is a plane in the world; billboarding is the program's, not the engine's

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

Card 022 refused to let its implementer choose between **spherical** billboarding
(the quad faces the camera entirely) and **cylindrical** (it rotates about up
only). The card was right that the choice is visible in every scene with a sprite
standing on ground, and right that it is not an implementation detail. It was
wrong about whose choice it is.

The tech lead was about to bring it as a decision with two options. The principal
answered with a third that was not on the list:

> *"You mean XYZ vs Y rotation for 2d sprites? None, that is up to the developer.
> [...] We just do planes with 2d sprites in 3d world, which means Z axis decides
> overlap."*

Constraints already fixed:

- **There is one rendering space and it is 3D** (ADR-0049). No screen-space path.
  That ADR's own wording says *"2D and sprites are billboarded quads in 3D
  space"* — this ADR narrows that sentence, and §Consequences says how.
- **The camera is an ECS component the program owns.** `voe_scene_camera` carries
  eye, yaw, pitch and field of view; the program adds it, places it and moves it
  (`scene/camera_system.h`), and `voe_scene_camera_get` reads it back.
- **`voe_scene_camera_forward` is already public**, and its header says why:
  *"a caller that wants to know where a camera points should not be doing
  trigonometry of its own."*
- **Reverse-Z depth, one blended pass, depth test on and depth write off, sorted
  back-to-front per object** (ADR-0033, ADR-0061). Overlap between quads is
  already resolved by depth and by that sort.
- **The engine spends nothing the program did not ask for** (ADR-0066), and
  **mechanism is the engine's while policy is the developer's** (ADR-0077, on
  resolution and upscaling — the same posture, stated for a different subject).

## Options considered

### Option A — the engine billboards, spherically
Every sprite faces the camera fully. Correct for particles, smoke and impostors;
wrong for anything standing on ground, which visibly leans back as the camera
rises. One behaviour, no knob, and the engine owns a rotation the program cannot
override without fighting it.

### Option B — the engine billboards, with a per-sprite mode
A sprite component carries `none` / `spherical` / `cylindrical`. Covers the cases,
and costs an enum, a branch per sprite per frame, and a permanent piece of
engine-owned transform behaviour that every later feature — animation, physics,
parenting, the editor — has to know about and agree with.

### Option C — the engine does not billboard at all
A sprite is a textured quad with a transform, like every other drawable. If the
program wants it to face the camera it rotates it, in game code, from a camera it
already owns. The engine provides the plane and the depth; the developer provides
the policy.

## Decision

**Option C, and it is the principal's.** The deciding factor: **billboarding is a
rotation, the program already owns every input to it, and an engine that computes
it is an engine that owns part of the transform the program thought was its own.**

- **A sprite is a quad in the world with an ordinary transform.** Nothing about it
  is special-cased in `render` or `3d`. It draws in the pass its alpha mode names,
  through the same camera, under the same depth rules.
- **Overlap is decided by depth**, as the principal put it — the Z axis. That is
  already true and needs nothing built: opaque and cutout write depth and test it,
  blended tests it and is sorted back-to-front per object.
- **The engine offers no billboard mode, no facing flag and no look-at helper.**
  Not `none`, not `spherical`, not `cylindrical`. There is no enum, because there
  is no behaviour to select between.
- **Spherical is the developer building a rotation from the camera's forward
  vector; cylindrical is the developer using its yaw and ignoring its pitch.**
  Both are a few lines of game code against `voe_scene_camera` and
  `voe_scene_camera_forward`, which are public today. **This was verified before
  the ADR was written, not assumed** — *"up to the developer"* is only an honest
  answer if the developer can actually reach it, and here they can, with nothing
  new from the engine.

## Blast radius

**Cheap, and cheap in the direction that matters.** This is a decision *not* to
build, so there is nothing to unbuild. If a later card shows that most programs
write the same twelve lines of facing code, the answer is a helper in `3d` or a
sample in `dev` — additive, breaking nothing, and by then written against real
usage instead of a guess about it.

What would make it expensive is the opposite decision taken first: an engine-owned
billboard mode is a piece of transform behaviour that animation, physics,
parenting and the editor all have to be reconciled with, and removing it later
silently changes where every sprite in every scene points.

Reversibility: **cheap.**

## Consequences

- **Card 022 loses its hardest question and gets smaller.** It was blocked on a
  choice that turns out not to be the engine's to make.
- **ADR-0049's sentence is narrowed, not overturned.** *"2D and sprites are
  billboarded quads in 3D space"* becomes **quads in 3D space**; the load-bearing
  half of that ADR — one rendering space, no screen-space path, `sprite` produces
  geometry rather than its own route to the framebuffer — is untouched and
  reaffirmed. The word *billboarded* was describing the expected use, and it was
  read as a specification.
- **A sprite that should face the camera and does not is a program bug, not an
  engine one.** That is a real support cost and it is accepted deliberately: the
  alternative is the engine guessing which of two wrong answers to give.
- **Sprite behaviour stops being a rendering concern and becomes a game-logic
  one**, which is where the `sprite` module's boundary already put it.
- **The consequence we like least:** the first person to build a 2D game on this
  writes facing code before they see a sprite behave the way they expect, and
  every engine they have used did it for them. The mitigation is a `dev` scene
  showing both rotations, which costs nothing and is now on card 022.
- **Nothing in the engine is billboard-aware, so nothing has to be told when this
  changes.** There is no code path to keep consistent.

## Rejected options and why

- **Option A — spherical only.** Rejected because it is wrong for the case the
  card itself named as the visible one: a sprite standing on ground. Picking the
  behaviour that fails the common case in order to avoid a knob is the worst of
  both.
- **Option B — a per-sprite mode.** The tech lead's expected recommendation, and
  rejected on the principal's better argument. It looks like the complete answer
  and it is the expensive one: it puts the engine permanently in the business of
  computing part of a program's transform, to save game code that is a few lines
  long and that the program is better placed to write — it knows whether the
  sprite is a character, a particle or a health bar, and the engine never will.

## Questions this opens

- **D-113** — whether the engine's **camera projection is the program's choice**,
  and specifically whether an orthographic projection is added. Raised by the
  principal in the same breath as this decision — *"Orthographic or perspective
  camera is also for dev to decide"* — and **deliberately not decided here,
  because it is not a sprite question and it changes a sentence in ADR-0074.**
  Recorded verbatim so the direction is not lost. Trigger: put to the principal
  as its own decision.
