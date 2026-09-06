# 021a — alpha modes and the blended pass

status: todo
claimed-by: -
blocked-by: 019

Split out of the old card 021 and placed ahead of it (ADR-0068). Everything here
was decided by **ADR-0061** and **ADR-0069** — this card implements those two and
nothing else. It is deliberately the whole subject of its own card: the first
blended pixel in the engine should be checked against a cube, not against a
glyph.

## Goal

The engine can draw see-through things. A blended quad in front of a cube, with
the cube visible through it, correct from every camera angle.

## Why this exists and 019 did not do it

ADR-0061 put this work on card 019 and the card text was never amended, so 019
was implemented and completed without it. Three comments in the tree still say
the engine is opaque — `render/src/device.c:787`,
`render/shaders/draw.slang:371`, and the header block in
`3d/include/3d/draw_system.h`. **All three are part of this card's scope**: a
comment that says the engine does not blend is as wrong as code that does not,
and this card is what makes them false.

## Scope

### The vocabulary — three words, spelled as glTF spells them

`opaque`, `cutout` (with a cutoff), `blended`. This is the engine's vocabulary
and not a glTF import detail: text and UI materials will use the same three
words. glTF's own names are `OPAQUE`, `MASK` and `BLEND`; the mapping is
one-to-one and the engine's spelling is the one above.

### `assets` — the reader carries what it currently refuses to

`assets/include/assets/model.h` has a paragraph saying the alpha mode is
deliberately **not** carried, because *"a field that is read and ignored reads as
a feature"*, and naming the card that blends as the card that adds it. **This is
that card.** Add the mode and the cutoff to `voe_assets_material`, and rewrite
that paragraph — it is now about `doubleSided` alone, which stays uncarried
(see *Refused*).

glTF's defaults are `OPAQUE` and a cutoff of `0.5`, and a file that omits the
field means opaque. A file naming a mode this reader does not know is
`VOE_BASE_ERROR_UNSUPPORTED` with the mode printed, in the style the rest of the
reader already uses.

### `3d` — the material component and the sort

- `voe_3d_material` gains the mode and the cutoff, and `3d/src/import.c` fills
  them from the reader.
- **The sort is its own module with its own test**, in the shape
  `3d/normal_matrix` already set: a pure function over view-space depths, so the
  card's ordering claim is checkable on a machine with no graphics card. The key
  is **the view-space depth of the object's origin** — one point per object, not
  per triangle.
- `voe_3d_draw_system_run` walks the tables twice: opaque and cutout in table
  order as today, then blended, **furthest first**. Mind the sign — the engine is
  right-handed with −Z forward (ADR-0033), so "further away" is *more negative*
  view-space Z, and getting this backwards produces a picture that is right from
  half the angles.
- **The sort needs scratch memory and the draw system has no arena** (rule 11:
  working memory is an arena passed in, and there is no default one). Expect to
  add a `voe_base_arena *` parameter to `voe_3d_draw_system_run` and rewind it
  per frame. If a better fit presents itself, report it rather than allocating
  behind the rule.

### `render` — a second pipeline and the blend state

- A second pipeline: **depth test on** (reverse-Z, `GREATER` — same as the opaque
  one), **depth write off**, blending on.
- **The blend state is premultiplied** (ADR-0069): source factor one, destination
  factor one-minus-source-alpha, for colour and alpha both.
- The mode and the cutoff go in `voe_render_shading_values.reserved_a[2]`, which
  exists on the right boundary for exactly this. **No size change and no change
  to the asserts in `render/src/descriptors.c`** — if either moves, something has
  been put in the wrong place.
- The entry point to draw with the second pipeline is added under ADR-0060 by its
  caller in `3d`. No new decision, and no new public concept beyond the mode.

### `render/shaders/draw.slang` — three paths and one multiply

The alpha mode decides alpha, then the shader's **last act is
`colour.rgb *= colour.a`** (ADR-0069):

- **opaque** — alpha forced to `1`, ignored entirely, which is what the glTF
  specification says an `OPAQUE` material's alpha means. **This line is what
  makes the unconditional multiply safe**; without it an opaque material carrying
  `a = 0.5` writes half its colour into a pass that does not blend and simply
  goes dark.
- **cutout** — `discard` below the cutoff, then alpha forced to `1`.
- **blended** — alpha as computed.

Write the contract into `draw.slang`'s header and into `render`'s public header:
**the colour target holds premultiplied colour, and anything that writes into it
outputs premultiplied colour.** Nothing enforces this; the next shader author
reading it is the whole mechanism.

### `dev` — something to look at

A blended quad in front of the existing cubes, and a second one at a different
depth so the ordering claim is visible rather than asserted. Keep it small: this
is the card's *Verify*, not a demo.

## Where this card is likely to go wrong

- **Opaque going dark.** The single most likely bug, and it looks like a lighting
  regression rather than an alpha one. See the opaque line above.
- **The sort's sign.** Correct from one side of the scene and wrong from the
  other. The unit test is what catches it; write it before the pipeline.
- **Depth write off is not optional and not a detail.** It is what makes the sort
  load-bearing rather than cosmetic. With depth writes on, blended objects hide
  each other and the sort appears to work while doing nothing.
- **Base-colour alpha is already linear and needs no decoding.** A Vulkan sRGB
  format decodes the three colour channels and passes alpha through untouched, so
  the alpha of an sRGB base-colour texture is correct as sampled. Do not "fix"
  it.
- **A blended object behind an opaque one must still be hidden.** Depth test
  stays on. Only the write goes off.

## Refused by name, so nothing adopts them in passing

Order-independent transparency. Per-triangle or per-fragment sorting. Transparent
objects casting or receiving shadows. Refraction and thickness. Dual-source
blending. Alpha-to-coverage — there is no MSAA, so it is not even available. A
second render target for transparency. A spatial structure for the sort.
**Two-sided materials**: the engine culls back faces whatever a material says,
`doubleSided` stays uncarried, and the first thing that needs a leaf or a sheet
of cloth is the card that opens it (D-089).

## Verify

- A blended quad in front of a cube: the cube is visible through it, and the
  blend tracks the quad's alpha.
- **Two blended quads at different depths look identical whichever order the
  tables hold them in.** This is the sort, and it is the one that matters.
- An opaque material whose base colour carries `a = 0.5` is **neither see-through
  nor darker** than it was before this card.
- A cutout material has a hard edge, and something behind it is hidden — cutout
  still writes depth.
- A blended object behind an opaque one is hidden by it.
- The sort's unit test passes on a machine with no graphics card.
- `cmake -P check.cmake` exits zero. Windows is the principal's.
