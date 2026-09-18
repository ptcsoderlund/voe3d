# 0077. Resolution and upscaling belong to the program, not the engine

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal asked whether the engine should render only at HD and upscale from
there — *"HD is great on everything from 7 inch to 65. 4K just adds some extra
sharpness and no hardware is ready for 8K."* He then proposed the concrete
mechanism: render to a texture at 1080p, stretch it to the display, and run a
sharpening pass over the result. That is FSR 1.0's and NVIDIA NIS's shape and it
is a real, shipped technique.

What the investigation found, before any of it was argued:

- **The mechanism already exists.** `render/src/device_internal.h` says the
  resolution "is the window's size today and it is not the swapchain's: what
  reconciles the two is the blit at the end of a frame, **which scales**. That is
  what makes rendering at a different resolution from the window a change to this
  one number later on." Rendering at 1080p on a 4K window is one number today.
- **It was already anticipated as a setting.** ADR-0064's knob list names two
  visible knobs, and **resolution scale** is one of them, credited to ADR-0051's
  offscreen target.
- **The presentation step is already planned to grow.** ADR-0051 says blit now,
  full-screen quad when the first post-process arrives — which is exactly where a
  sharpening pass would live.

So the question was never capability. It was whether the engine should *make the
choice* — cap the resolution, and own an upscaler.

The tech lead argued against a ceiling on four grounds (capping costs new
clamping code where not capping costs nothing; the engine has **no
anti-aliasing**, so output resolution is currently doing anti-aliasing's job;
ADR-0076 just bought an exact glyph edge that a fixed-1080p stretch would spend
again at presentation; and headsets and the editor are the two consumers a
resolution cap hurts most — the first being the principal's own reason for
overruling an orthographic overlay in ADR-0074). It argued the sharpening
proposal was the better shape but had an unmet prerequisite: **sharpening is a
high-pass filter and aliasing is high-frequency error**, so it amplifies exactly
the artifact this engine cannot currently fight.

**The principal decided against building any of it**, on a cleaner principle than
the one being argued: *"Upscaling and resolution should be up to the dev using the
engine. The renderer should be able to render in any way, shape or form
requested."*

## Options considered

### Option A — cap the render resolution at HD and always scale to the window
Predictable frame cost independent of the player's monitor, which is the single
biggest performance lever in real-time rendering and is why consoles do it. Costs
new clamping code, gives 1080p-grade aliasing magnified onto a 4K panel in an
engine with no anti-aliasing, spends ADR-0076's exact glyph edge at the
presentation step, and is hostile to headsets and to a text-heavy editor.

### Option B — build the stretch-and-sharpen presentation step
Edge-directed upsample and a contrast-adaptive sharpen, in the full-screen quad
ADR-0051 already plans. Cheap — a fraction of a millisecond at 4K — genuinely
effective on large high-contrast edges, and architecturally aligned with work
already anticipated. It cannot recover detail that was never sampled, and it
requires an anti-aliased input the engine does not have.

### Option C — build neither; the mechanism stays, the policy is the program's
Resolution remains one number that defaults to the window. The engine ships no
upscaler, no sharpener and no cap. A program that wants to render small and
stretch sets the number and gets what the blit gives it; a program that wants
something better builds it, or asks for it as an opt-in feature later.

## Decision

**Option C.** The deciding factor, in the principal's words: **the renderer's job
is to render what it is asked to render.** Choosing a resolution and choosing
whether to upscale are the game's decisions, and an engine that makes either of
them for its user has taken a policy position where it should have provided a
mechanism.

- **No resolution ceiling.** Resolution stays one number, defaulting to the
  window. Nothing clamps it.
- **No built-in upscaler and no built-in sharpener.** The presentation step stays
  a blit until a post-process needs the quad, and that quad is not an upscaler by
  default when it arrives.
- **`render` grows toward "render as asked", not toward a set of blessed modes.**
  This is the general form of what ADR-0060 already said about its public surface
  and what CLAUDE.md already says about `render` being the GPU layer only.
- **This is a decision not to build something.** No card follows it. Its whole
  value is that the next person to write the graphics-settings card, the
  post-process card or a VR card does not assume an upscaler was wanted because
  every other engine has one.

## Blast radius

**Cheap, because nothing is built.** Reversing it means adding a feature to a
place that is already designed to receive one — the full-screen quad is already
planned and the resolution is already a variable.

What is load-bearing is the **posture**, not the code: `render` provides
mechanism and the program provides policy. Reversing *that* is what would be
expensive, because a renderer with opinions about resolution is one that also
grows opinions about quality presets, and those reach into everything.

## Consequences

- **There is nothing to do.** No card, no work, no change to any file.
- **A program that wants 1080p on a 4K panel can have it today**, with one
  number, and gets a bilinear stretch. That is the honest current state and this
  decision says it is enough until someone with a real program says otherwise.
- **The engine's picture quality at high resolution is native rendering and
  nothing else**, which is the correct default and the one that costs no code.
- **The anti-aliasing gap is now pointed at by three separate things** and still
  has no card: ADR-0072 found cut-out foliage has crawling edges nothing can
  soften, ADR-0075 removed the mipmap chain that was hiding small-text
  degradation, and any future sharpening pass would amplify aliasing rather than
  hide it. This ADR does not move it — the principal has not been asked — but it
  is now the most-pointed-at gap in the engine and that should be said out loud
  rather than accumulating quietly in a register.
- **The overlay shares the world's render target**, so any resolution scale a
  program applies takes the interface down with the world. Every engine that
  ships resolution scaling renders its interface at native and composites, and
  this one currently cannot, because ADR-0074 deliberately put the overlay in the
  same target and card 023's second target does not exist. **That is a
  consequence of ADR-0074 that nobody recorded** and it is now D-108. It is not a
  fault today, because nothing scales resolution.

## Rejected options and why

- **Option A — the HD ceiling.** Rejected by the principal on the principle
  above, and the tech lead's four objections stand as supporting rather than
  deciding reasons. Worth recording that the *instinct* was sound: fixed internal
  resolution genuinely is the biggest performance lever there is. It is available
  as a knob and always was; what was rejected is the engine choosing it for the
  program.
- **Option B — stretch and sharpen.** **Not rejected as a bad technique.** It is
  FSR 1.0's shape, it ships in real games, it is cheap, and it is the natural
  evolution of the presentation step ADR-0051 already planned. Rejected because
  it is not the engine's job, and secondarily because its precondition is unmet:
  sharpening amplifies aliasing, and the engine has no anti-aliasing at all. If
  it is ever built, it is built after that and it is opt-in under ADR-0066.

## Questions this opens

- **D-108** — **that the overlay cannot be rendered at native resolution while
  the world is scaled**, because ADR-0074 put both in one target. Not a fault
  until something scales resolution, and the fix is a second target, which is
  card 023's and is already going to exist for the offscreen panel. Recorded so
  that whoever first uses resolution scale is not surprised by blurry text.
  Trigger: the first program to render at a resolution other than the window's.
