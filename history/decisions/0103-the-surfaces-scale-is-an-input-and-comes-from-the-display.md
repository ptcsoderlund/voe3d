# 0103. The surface's scale is an input, and its default source is the display

- **Status:** Superseded by ADR-0104
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0100
- **Superseded by:** ADR-0104 — **the parameter survives and the policy does not.** `render` still takes pixels-per-millimetre and holds no policy; the source went back to the window's height with `ui_scale` as the only calibration, because reading a display is per-platform code the principal declined to carry
- **Closes:** D-177, D-183's sibling question about where the number comes from

## Context

Six hours after ADR-0100 fixed the anisotropic stretch, the principal:

> Surface should take its size from monitor height instead of window height.
> Preferrably DPI calculated. Dont you think? For non camera stuff. Maybe even
> better min(screen_height,screen_width)?

**ADR-0100 got the shape right and the source wrong.** Its pins that stand: one
uniform scale so nothing is deformed, the width in millimetres following from the
scale, text scaling by the same number, world panels untouched, form-factor
branching left to the developer, wrapping unbuilt until something is too narrow.
**What moves is one sentence: where the scale comes from.** ADR-0100 took the
*window's* height, so enlarging a window enlarged the interface. He is asking for a
scale that does not move when the window does.

**That is D-177 being decided.** This morning's answer to *different stretch modes on
text* was that pinning the surface's height and pinning its scale are the same two
numbers with the known one swapped — pin the height and everything is a fraction of
the window; pin the scale and a bigger window holds *more* content at the same size,
which is what every editor does. He has now arrived at the same fork from the other
side and asked for the second one as the default.

Constraints already fixed:

- **ADR-0089 — a GUI unit is one millimetre, and a millimetre is physical.** On a
  panel in the world that has always been literally true. On screen it was not:
  ADR-0100 had to confess a duality — *physical in the world, proportional on the
  screen, same authoring numbers* — and record that a reader not told would assume
  wrongly.
- **ADR-0088 — the style target is Windows 10**, which scales its interface by the
  user's chosen factor and does **not** grow when a window is enlarged.
- **ADR-0091 — a runtime UI scale is a push/pop of scale**, already decided, already
  the per-subtree version of this same knob.
- **`platform` exposes nothing about the display.** `platform/include/platform/window.h`
  has the window's pixel size, its native handle and whether it is decorated — no
  DPI, no scale factor, no monitor size, nothing about physical dimensions.
- **ADR-0093's leaf reasoning** — a folder that cannot know a thing is *given* it as
  a plain value rather than reaching for it.

## The two proposals in one sentence, and why only one can be had honestly

**"Monitor height" and "DPI calculated" are the same idea; `min(screen_height,
screen_width)` is a different one.** Under a physical scale the aspect ratio never
enters the arithmetic, so a `min` has nothing to guard. Under a scale proportional to
the monitor, `min` is exactly the right guard — it stops a portrait or ultrawide
display from producing an absurd scale. So the sentence contains both modes, and
they want different answers.

**And "DPI calculated" cannot mean physics, because physics is not reliably
available.** Wayland reports a display's physical size in millimetres and it is
frequently wrong or zero — virtual machines, televisions, projectors, KVM switches.
Windows' `GetDpiForWindow` does not report physics at all: it reports **the scale the
user chose**, relative to 96 per inch.

**Which is better than physics, and this is the part worth stating.** ADR-0089's own
argument was that millimetres convert an unanswerable question — *how do we keep a
hairline crisp* — into an ordinary physical one: *at what viewing distance is this
legible*. Viewing distance and eyesight are exactly what a user's chosen scale
encodes and what a display's physical dimensions cannot. A 4K television reporting
its true DPI would give a physically correct interface that is unreadable from a
sofa.

## Options considered

### Option A — the scale is a parameter; `render` holds no policy

The surface helper takes **pixels per millimetre** and the target's pixel size, and
returns the surface's millimetre size. Both modes then exist without the engine
choosing: a physical caller passes the display's number, a proportional caller
passes `target_height ÷ desired_millimetres`.

### Option B — `render` reads the display and applies the physical policy itself

The engine asks the platform for a scale and uses it. One way to do it, nothing for
a caller to get wrong, and the proportional mode has to be added back as a flag.

### Option C — keep ADR-0100 and add a mode flag

A surface says *proportional* or *physical* and the engine branches.

## Decision

**Option A, with the physical scale as the documented default.** Two things, and the
first is what makes the second cheap.

**1. The scale is an input.** `render` is handed pixels-per-millimetre; it does not
know where the number came from and cannot know — it is a fact about a display and a
user's preference, and this is the same reasoning ADR-0093 used to keep `ui` from
reading input. **Both of the principal's modes are then caller-side arithmetic on one
mechanism**, which is what this morning's answer already said they were, and neither
needs a flag in the engine.

**2. The default policy is physical, from the display, not from the window.** A
millimetre on screen becomes a millimetre, ADR-0089's name becomes literally true,
and the duality ADR-0100 had to confess disappears. Enlarging a window then changes
**how much you see, not how big it is** — which is what an editor does, and it is the
trigger ADR-0100 already handed to wrapping and scrolling.

**3. What the engine asks the platform for is the scale the user controls**, not
physics: Windows' per-window DPI setting, Wayland's output or fractional scale.
Computed physical DPI is the *fallback* for a platform that will only report
millimetres, and **1.0 is the fallback when it reports nothing at all** — which is a
case that must exist, not an error path.

**4. The proportional mode stays**, as a caller's choice, because a heads-up display
on a television genuinely wants it — and **`min(width, height)` is where that guard
belongs** if it is ever wanted. It is not needed in the physical mode and is not
built.

**5. Everything else ADR-0100 pinned carries over unchanged**, including that no
part of this touches a panel standing in the world, and that there are no
breakpoints anywhere in the engine.

**6. `platform` learns to report the number in its own card, and until then the
program passes a constant.** Nothing waits on that, because the parameter is the same
shape either way.

## Blast radius

**Smaller than ADR-0100's, because the policy leaves the engine.** What could be
wrong afterwards is a number a caller passes, not a rule compiled into `render`. The
helper's signature is the permanent part, and it has one caller today.

Reversibility: **cheap.** Changing the default policy later changes what `dev` and
the platform card compute, not what `render` does.

## Consequences

- **Two ADRs in one day, the second superseding the first, and the record should say
  so plainly.** ADR-0100 fixed a real defect — width and height were scaled
  independently and everything, text included, was deformed by a resize — and that
  fix stands. Only the source of the scale moved, and it moved because the editor
  case arrived six hours later. **The first one lasted an afternoon; that is what a
  register is for.**
- **The engine now trusts a number it cannot verify.** A virtual machine reporting a
  nonsense physical size makes the whole interface the wrong size and nothing can
  detect it. The mitigation is the user-settable scale, which is D-186, and a
  fallback that is documented rather than clever.
- **A physically-sized interface is too small on a television seen from a sofa.**
  That is not a defect of this decision, it is why the proportional mode stays and why
  a user's scale is the real knob.
- **The exhibit's behaviour on resize changes again**, from *the same content
  stretched* to *the same content at a fixed size with more or less room around it*.
  Card 032 carries both changes, so it changes once.
- **The consequence I do not like:** until the platform card lands, the demo passes a
  constant, which means the engine's own program is *pretending* to know the display.
  That is honest only because it is written down here and in the card.

## Rejected options and why

**Option B — `render` reads the display itself.** It would put a platform question
inside the folder that draws, and `render` naming `platform` for a window handle is
one thing while `render` asking about monitors is another. It also makes the
proportional mode a flag, which is a policy switch inside a folder that has no
business holding a policy.

**Option C — keep the window-derived scale and add a mode flag.** Two policies inside
the engine where zero will do. Under Option A the modes are one multiplication apart
at the call site, and the engine never learns that there were two.

## Questions this opens

- **D-186 — whether the engine carries a user-settable UI scale, and where it is
  persisted.** ADR-0091 already has the push/pop mechanism; what is undecided is
  whether the engine owns the *setting*, which is adjacent to D-080 (persisted
  settings) and D-160 (where a theme file lives). Trigger: the settings panel, card
  037.
- **D-187 — what to do with an implausible physical size from a platform.** Clamp,
  ignore and fall back, or trust it. Trigger: the platform card, or the first
  virtual machine that reports nonsense.


## Amendment · 2026-09-09 · no physics, and `ui_scale` is the knob

**The principal, hours later:** *"We skip dpi. dev or end user have to set ui_scale.
Is there a system wide ui_scale setting we can use and then our internal as a
fallback? Windows have Settings->Make everything larger."*

**Adopted, and it narrows this ADR rather than reversing it.** Point 3 above made
computed physical DPI the fallback for a platform that reports only millimetres.
**That fallback is deleted.** Nothing in this engine reads a display's physical size,
ever: Wayland's millimetres are never asked for, and the plausibility question that
came with them (D-187) is **moot rather than deferred**.

**The chain is now, in order:**

1. **The system's UI scale**, where the platform reports one — a courtesy default so
   that an interface is the right size on a scaled display without anybody
   configuring anything.
2. **`ui_scale`, the program's own number**, which multiplies it and is the whole
   answer when the system says nothing. Default 1.0.
3. **Nothing else.** No physical size, no environment variables, and **no D-Bus** —
   the desktop portal does expose a text-scaling factor on some Linux systems and it
   is exactly the sort of dependency the build promise forbids hunting for.

**What that costs, stated plainly: a millimetre becomes nominal.** With the display's
physical density gone, a millimetre is defined against a reference density of 96 per
inch — so 1 mm is about 3.78 pixels at 100%, times the scales above. **The web made
the same trade for the same reason**: CSS defines an inch as exactly 96 pixels
because real physical size is neither knowable nor, on a television, desirable.
ADR-0089's argument survives — authoring in integers that mean a physical size — but
the unit is now *nominally* physical and scaled by choice, and the header must say
so. That is a smaller lie than a physical size read from a monitor that is guessing.

**And the coupling that decides where the reading lives, which is Windows-specific
and important.** A process that declares no DPI awareness — which is this engine
today, deliberately — is **told 96 by `GetDpiForWindow` no matter what the user
chose**, because Windows virtualises the whole window instead. So on Windows the
system's scale *cannot be read* without declaring awareness, and declaring awareness
is D-188.

**The other half of that coupling is the good news: the system scale is already being
applied to us.** An unaware window on a 150% display is virtualised and stretched by
Windows, and a Wayland surface with no buffer scale is stretched by the compositor —
so the interface is *already the right physical size today*, and what it costs is
sharpness, not size. **Reading the number and rendering sharply are therefore one act,
not two**, and reading it has no value until we do the second. Card 042 reports what
it can see without changing awareness — which on Wayland is the real output scale and
on Windows is 96 — and **that asymmetry in its report is the evidence D-188 wants**,
measured rather than argued.
