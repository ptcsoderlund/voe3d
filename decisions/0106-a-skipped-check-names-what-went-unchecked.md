# 0106. A skipped check names what went unchecked

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-196. Opens D-197.

## Context

The coder of card 034 reported a blockage, and it was a real seam rather than a
gap in the card:

> Card 034 requires `ui` tests that run with no graphics card and no window
> system, and one of them is "the same label at `text_scale` 1.0 and 1.5". But a
> label measures through `voe_text_font`, and `voe_text_font_new` requires a
> `voe_render_device` — it uploads the atlas. So a ui test cannot get a font as
> things stand.

He is right about the mechanism. **A font is two things in one object.**
`struct voe_text_font` holds an atlas id, a line height and a table of glyph
metrics; `voe_text_font_new` reads the embedded Oxanium, rasterises every glyph
into a scratch arena and computes every box and advance — **all of it in ordinary
memory** — and then, at `font.c:402`, makes one call to
`voe_render_texture_create`. That call is the only thing in the folder's
construction path that touches a device. Measuring a string never needed a
graphics card; it shares a constructor with the half that does.

The tech lead recommended splitting the constructor — a font that does everything
except the upload, following the precedent `render` already set with
`voe_render_device_new_headless`. **The principal overruled it**, and was right to.

## The correction that decided it, and it was a fact rather than an argument

The tech lead's case rested on the claim that a device-backed check is unrunnable
on a machine with no graphics driver, and that such machines are ones we use. **That
claim was not checked before it was made, and it is false here.** The Linux
workstation has `libvulkan.so.1` and eight ICDs including `lvp_icd.json`; a probe
built in a scratch directory creates an instance and enumerates one physical device.
The software rasteriser answers. **Every machine that runs `check.cmake` today has a
usable Vulkan.**

So the harm the split was buying insurance against does not currently occur, and the
split would have been engine surface built for a hypothetical. That is the
premature-generality refusal this project has made a habit of, arriving from the
tech lead's own side of the table for once.

**It is on file that the recommendation was made without establishing its own
premise.** The measurement took ninety seconds and reversed the recommendation. This
is the second time in a week that a check ran against something nobody had confirmed
— and this one was the tech lead's.

## The principal's rule, which is wider than the card

> If there is no gpu and the check fails because there isnt one. It should just
> output that a gpu is missing. So we are aware to not listen on that check on
> those environments.

**A check that cannot run in an environment says so, and the human discounts it
there.** That is already what `render/tests/offscreen.c` does — *"A MACHINE WITH NO
USABLE VULKAN SKIPS AND SAYS SO … The skip is a pass"* — and this ADR promotes it
from one test's local habit to the standing rule, and adds the one thing it was
missing.

## Decision

1. **A check that cannot run reports a skip, and the skip is a pass.** Unchanged
   from what `offscreen.c` already does. A machine without a driver is not a broken
   checkout and must not read as a failure.

2. **The skip names what went unchecked, not only why it could not run.** This is
   the addition, and it is ADR-0105 applied to a skip: *a measurement names the
   sample it took*, and a skip is a measurement of nothing that must say what
   nothing it measured. `skip: no graphics driver` describes the environment;
   **`skip: no graphics driver — the text-size check did not run`** describes the
   hole in the coverage. Only the second is actionable by the person reading the
   log.

3. **`ui`'s checks may use `voe_render_device_new_headless`**, and card 034's
   text-scale check does. `ui` still names only `render`, `text`, `math` and `base`
   (ADR-0093), and the device is a headless one — **no window, no compositor, no
   window system**, which is the property the card actually cared about and which is
   preserved exactly.

4. **`text` is not split.** `voe_text_font_new` keeps its device parameter and the
   font stays one object. The second constructor is D-197, deferred with a named
   trigger, not refused.

## Blast radius

**Cheap, and it is cheap in both directions.** Nothing is built: point 3 is the
coder writing one existing call, and point 2 is the wording of skip messages, which
are strings. Reversibility is one small card in `text` on the day the trigger fires,
and the seam is one line deep — the upload is the last statement of the constructor.

Reversibility: **cheap.**

## Consequences

- **Card 034's verify list is wrong as written and is amended in flight**, dated at
  the top of the card on card 033's precedent. *"`ui` tests, plain C, no graphics
  card"* is true of every check on that list but one; the text-scale check needs a
  headless device and now says so.
- **The `ui` folder's checks acquire a graphics dependency at the folder level.**
  Measuring is how a widget gets a natural size, so wrapping, fit-to-children, the
  scroll area and every future widget's size check inherit this. That is the cost
  and it is stated rather than discovered later: the day a machine without a driver
  matters, it is that whole category that goes dark, not one test.
- **The consequence I do not like, and it is mine to own:** the engine now has a
  category of check whose green depends on the environment, and the only thing
  standing between us and a silent hole is the wording of a message that nothing
  verifies. That is the same weaker instrument ADR-0105 accepted for the same
  reason — there is no machine check available for whether a human read a line.
- **What makes it acceptable is that the skip is loud and the environment is
  uniform.** Every machine in use answers Vulkan today, so the skip should never
  fire; if it ever does, that is itself the news.

## Rejected options and why

**A second constructor in `text` — a font with metrics and no atlas.** The tech
lead's recommendation. Rejected because its premise did not hold: it buys a check
that runs on machines with no driver, and there are none. It also adds public
surface to a leaf folder for a caller that does not exist, which is the refusal
rule 10 and ADR-0083 both make. **It stays the right answer if the premise ever
becomes true** — see D-197 — and the seam is recorded here so nobody re-derives it.

**A fake font built by the test with invented metrics.** Rejected on two counts: it
would need `text` to expose its internals to a test, and it proves the reflow
arithmetic against numbers nobody will ever ship. The check's value is that it runs
against Oxanium's real advances.

**Leaving the skip message as it is.** Rejected as the whole content of the
principal's rule: *so we are aware to not listen on that check* only works if the
line says which check.

## Questions this opens

- **D-197** — whether `text` gains a metrics-only constructor. Trigger: a machine
  that runs `check.cmake` without a usable Vulkan, or a caller that wants text
  measurements outside a program that draws.
