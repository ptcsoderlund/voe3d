# 0075. Sampling is chosen at texture creation from a named set, not one global sampler

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal reported text as blurry. Two independent faults were producing it
and this ADR is the first of them; ADR-0076 is the second.

`render` creates **exactly one sampler** and hands it to every texture in the
engine (`render/src/texture.c`, `device->sampler`, used by every descriptor
write). It was written for the case that existed at the time — a cube being flown
around — and it is right for that case: `VK_FILTER_LINEAR` magnification and
minification, `VK_SAMPLER_MIPMAP_MODE_LINEAR` over a full generated chain,
`REPEAT` addressing, anisotropy off, `maxLod` unclamped. Every texture also gets
a mipmap chain generated at upload, whenever the format supports linear blitting.

The glyph atlas is handed that sampler, and it is close to the worst filter that
could be chosen for it:

- **Any minification reads a pre-averaged copy** of the sheet, and trilinear
  blends *two* of them, which is blurrier than either level alone.
- **Nothing is ever texel-to-pixel aligned.** ADR-0049 puts everything in 3D
  space in metres through a perspective camera, and ADR-0074 keeps camera-locked
  text there too. There is no screen-space path and no pixel snapping, so text is
  permanently a fractional distance into the mip chain rather than sitting on
  level zero.
- **Anisotropy is off**, so text at an angle takes its level of detail from the
  more foreshortened axis and is softened along *both*.

The atlas builder already diagnosed this from the wrong end. `text/src/font.c`
carries a long comment working out that a sheet finer than the screen is minified
into the chain and comes back blurrier, and the sheet was dropped from eighty
pixels to the em to forty on that reasoning, with "eighty was tried first and is
visibly soft" recorded in the code. **The reasoning is correct and the fix was
applied to the wrong object**: it tuned the content to dodge a filter that text
should never have been given.

Constraints already fixed:

- **`render`'s public surface is by id and grown on demand** (ADR-0060,
  ADR-0018). A texture is created through one call and referred to by id, and the
  descriptor set already carries a `sampler` field **per slot** — the shape for
  more than one sampler exists and only ever had one value put in it.
- **Capability is frozen at device creation in named tiers** (ADR-0063).
  Anisotropic filtering is a device feature that must be requested there, and
  nothing has requested it.
- **Cost is opt-in** (ADR-0066) and **implement on demand** (ADR-0034).
- **Texture kind is already chosen at creation** — `VOE_RENDER_TEXTURE_COLOUR`
  against `VOE_RENDER_TEXTURE_DATA`, deciding sRGB decode. There is a precedent
  for "how this texture is read" being a property of the texture.

## Options considered

### Option A — keep one sampler, keep tuning content around it
What happens today. Free, and it is why the atlas is forty pixels to the em. It
makes every texture that is not a picture on a surface — the glyph sheet now, a
sprite sheet and a distance field next — pay for a filter chosen for something
else, and it hides the cost inside whichever folder is currently being tuned.

### Option B — a sampler per texture, created alongside it
Each texture carries its own `VkSampler`, configured at creation. The descriptor
shape already allows it, so this is nearly free to build. It makes filtering
fully per-texture, and it opens a configuration surface — six or seven knobs per
texture — that nothing in the engine has anyone to set, and multiplies sampler
objects by texture count for no gain when almost all of them want the same thing.

### Option C — a small named set of sampling modes, chosen at creation
`voe_render_texture_create` takes a sampling mode beside the kind it already
takes. `render` creates one sampler per mode, once, and a texture slot points at
the one its mode names. Two modes to begin with, grown when something asks:

- **`SMOOTH`** — what exists today: linear, full mip chain, repeat. Every texture
  in the engine keeps it and nothing about the cubes or the models changes.
- **`SHARP`** — linear magnification and minification, **no mip chain generated
  and none sampled**, clamp to edge. For a sheet that is an atlas rather than a
  picture, where a neighbouring texel belongs to a different glyph and averaging
  the two is meaningless.

## Decision

**Option C.** The deciding factor: how a texture should be filtered is a property
of **what the texture is**, which the caller already knows and already states
once — the same thing `VOE_RENDER_TEXTURE_COLOUR` against `_DATA` is — and the
set of answers is small and closed, so a named set says it in one word where a
per-texture sampler would offer seven knobs with nobody to turn them.

- **The mode decides mip generation, not only mip sampling.** `SHARP` generates no
  chain at all. Clamping `maxLod` alone would leave the levels built, uploaded and
  resident for nothing.
- **`SHARP` addresses clamp-to-edge.** An atlas is not tiled, and `REPEAT` means a
  glyph on the sheet's edge can wrap into one on the far side. Padding hides that
  today; clamping removes it.
- **Anisotropy stays off** and stays unrequested. It is a device feature under
  ADR-0063 and it is moot for a texture with no chain to select from. It comes
  back for world textures as D-102.
- **Address mode is bundled into the mode rather than being a second axis.** Two
  named answers, not a matrix. If something ever wants clamp with mips, that is a
  third named mode and one enum value.
- **The glyph atlas becomes `SHARP`.** This is what the ADR is for.

## Blast radius

**Cheap, in the direction that matters.** Adding a mode later is one enum value,
one sampler created at startup, and no change to any existing call site — the
existing modes are unaffected, so this does not get more expensive with time.

What is load-bearing is the smaller claim underneath it: **sampling is fixed when
the texture is created and cannot be changed per draw or per material.** A
material cannot ask for the same texture filtered two ways. That matches
by-id resources (ADR-0018) and nothing wants otherwise today, but reversing it
means sampler state reaching the draw call, which is a different shape.

## Consequences

- **Text stops being read out of a mipmap chain**, which is the blur that should
  never have been there.
- **`font.c`'s forty-pixels-to-the-em reasoning is obsolete.** The long comment
  explaining that a finer sheet is blurrier is a correct description of a fault
  this removes. It must be replaced rather than left standing, or the next reader
  will re-derive a workaround for something that no longer happens. ADR-0076
  replaces the number entirely.
- **This must not ship alone, and that is the consequence we like least.** A
  coverage bitmap with no mip chain does not blur when minified — it **aliases**.
  Text at a distance would go from soft to crawling and shimmering, which is a
  worse fault than the one being fixed. It is safe only because ADR-0076 lands
  with it: a distance field recovers its own edge width from screen-space
  derivatives and degrades gracefully where a coverage bitmap does not. **The two
  belong on one card.**
- **Every existing texture keeps `SMOOTH` and nothing in the world changes.** The
  cube that motivated the original sampler still gets the sampler it wanted.
- **`render` gains a second sampler object and a mode enum**, which is the whole
  of the public-surface growth. Descriptor writes already read a per-slot
  sampler, so the descriptor path is untouched.
- **A sprite sheet will want `SHARP` too**, for the same reason a glyph sheet
  does — neighbouring texels belong to different sprites. That card inherits this
  and does not have to argue it.

## Rejected options and why

- **Option A — one sampler, tuned content.** Rejected because it has already cost
  us once, invisibly: the atlas is at forty pixels to the em, and the principal
  read the result as blurry text rather than as a filter fault. A workaround that
  reads as a considered constant in a comment is worse than the fault it hides.
- **Option B — a sampler per texture.** Rejected on surface area, not on cost.
  It is nearly free to build and would work. It offers every call site a filtering
  configuration when the engine has two real answers, and ADR-0034 says the time
  to build the general form is when something needs it.

## Questions this opens

- **D-102** — whether anisotropic filtering is enabled, and whether it arrives as
  a graphics setting under ADR-0064 rather than as an always-on cost. It is a
  device feature under ADR-0063 so it must be requested at device creation, which
  means the decision is taken before any program can ask. Trigger: the first
  world texture seen at a grazing angle that anyone complains about.
- **D-103** — whether sampling mode and address mode stay bundled. Two named
  modes cover what exists; a texture that wants clamping *with* a mip chain would
  make the bundle a matrix and is the point to reconsider. Trigger: the third
  mode.
