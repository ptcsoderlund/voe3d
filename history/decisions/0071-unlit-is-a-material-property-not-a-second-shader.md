# 0071. Unlit is a material property, not a second shader

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

Every drawn thing in this engine goes through one shader,
`render/shaders/draw.slang`, and that shader lights everything with the sun: a
metalness-roughness BRDF with a required directional light, asserted on by
`3d/draw_system.h` — *"a world that is drawn needs exactly one light, and none
asserts."*

**Text must not be lit.** A glyph shaded by a directional light changes brightness
as the sun moves and goes dark when it faces away, which is not a stylistic
opinion about text — it is wrong in the way that looks like a renderer bug. The
same is true of the debug and UI surfaces that follow it.

So card 021b needs a way to draw something that ignores the light, and there is
none. This is the last thing standing between the text card and being writable.

Constraints already fixed:

- **One shader, compiled offline and `#embed`ded** (ADR-0046), whose `slangc`
  invocation is `voe_render_shaders()` — *deliberately folder-specific and not a
  general mechanism*. A second folder with shaders opens D-052.
- **glTF is the vocabulary** (ADR-0061 for alpha modes, ADR-0069 for what the
  shader outputs). glTF has a name for this already: `KHR_materials_unlit`.
- **`voe_render_shading_values` has `uint32_t reserved_c[3]`** free, after the
  texture ids — ADR-0069 takes `reserved_a[2]` for the alpha mode and cutoff and
  leaves this block untouched.
- **The first non-trivial pipeline variation is where a variant explosion starts
  if nobody watches it** (ADR-0061's own consequence).
- **No premature generality** (ADR-0008) and **implement on demand** (ADR-0034).

## Options considered

### Option A — an unlit flag on the material, branching in the one shader
A boolean in the shading record. Where it is set the fragment stage skips the
BRDF and outputs the base colour times the base-colour texture, and nothing else
changes: same pipeline, same descriptor layout, same `slangc` invocation, same
alpha modes on top. Costs a branch on a uniform — coherent across a draw, which
is the cheap kind — and one more field in a record that has room for it.

### Option B — `text` gets its own shader
A shader in `text/shaders/`, its own pipeline, its own descriptor layout. It is
the obvious shape if you think of text as a different kind of drawing. It opens
D-052 immediately (where the `slangc` invocation lives once a second folder has
shaders), adds a third pipeline to a renderer that has one, and duplicates the
alpha-mode and premultiplied-output logic that ADR-0069 just made a single
contract — which is precisely how two conventions start to drift apart.

### Option C — abuse emission: black base colour, glyph colour in emissive
No new field at all. It fails twice: emission is *stored and not read* today
because nothing has asked for it and it clips without tone mapping, and the
diffuse term would still be there multiplying by the light. It would also make
"unlit" mean something different from what every other engine and glTF mean by
it.

## Decision

**Option A.** The deciding factor: text needing to ignore the sun is a property
of the *material*, which is where glTF puts it and where the engine already
carries every other thing a material says — making it a second shader would spend
a pipeline, a descriptor layout and an open question on a boolean.

- **`unlit` is a field on the material**, spelled the way glTF's extension spells
  it, and carried through `voe_3d_material` into
  `voe_render_shading_values.reserved_c`.
- **The shader branches on it and skips the BRDF.** An unlit surface is its base
  colour factor times its base-colour texture, in linear space, with the alpha
  mode applied and the premultiplied output multiply as ADR-0069 requires. No
  light, no normal, no eye direction, no occlusion.
- **It is a property in its own right, not a text feature.** Sprites, debug
  drawing and UI want it too, and each of them wanting it is not a new decision.
- **The engine still needs exactly one light in a world that draws anything**, and
  this does not change that. An unlit material does not make the sun optional; a
  world of nothing but unlit text is not a case anyone has, and inventing a rule
  for it now would be generality with no consumer.
- **`text` gets no shaders**, so **D-052 stays shut** and `voe_render_shaders()`
  stays folder-specific and honest.

**Not decided:** whether the glTF importer reads `KHR_materials_unlit` from a
file. The reader refuses unknown required extensions today and this ADR does not
change that — the flag exists for the engine's own materials. A file asking for
it is a card, not an assumption.

## Blast radius

**Cheap.** A field, a branch and a line in the importer, all reversible in an
afternoon.

The part worth guarding is the count of shaders, not this flag: **the answer to
"this surface draws differently" is a material property until someone can show
that a branch is the wrong tool.** That is the rule this ADR establishes and the
thing that gets expensive if it is abandoned quietly — three shaders is not much
worse than one, and nine is a different engine.

## Consequences

- **A second branch in `draw.slang`**, alongside the alpha mode's. Two uniform
  branches in one shader is fine; this is also the second entry in a list that
  should be watched, and the watch ADR-0061 asked for now has something in it.
- **Card 021b needs no shader and no pipeline**, which is most of what made it
  look like a large card.
- **Unlit is available to card 022's sprites for free**, which is what a sprite
  usually wants.
- **The engine has a way to draw something exactly as authored**, which is the
  first thing wanted for debug geometry and for anything that must not be
  interpreted.
- **An unlit material still needs a world with a sun in it**, which will read as
  odd the first time someone writes a text-only program. Named here so that it is
  a known edge rather than a surprise.

## Rejected options and why

- **Option B — a shader in `text`.** Rejected on what it opens rather than on
  what it costs to write: a second shader folder, D-052 forced open, a third
  pipeline, and two copies of the premultiplied-output and alpha-mode rules that
  would be free to diverge. The day a drawing folder genuinely needs a shader of
  its own, that is a decision worth taking on its merits — and it should not be
  taken sideways, by a text card that needed a boolean.
- **Option C — emission.** Rejected because it means something else, keeps the
  diffuse term, and reads as a trick in a codebase that explains itself in
  paragraphs.

## Questions this opens

- **D-093** — how many uniform branches `draw.slang` carries before the engine
  wants pipeline variants or shader specialisation instead. Two after this ADR
  (alpha mode, unlit). Trigger: the fourth.
