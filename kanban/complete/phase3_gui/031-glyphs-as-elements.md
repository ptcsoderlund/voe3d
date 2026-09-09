# 031 — glyphs as elements

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it follows
030 and takes the next number. **030 has landed and been accepted**, so this card
is written against the element record that exists rather than the one that was
planned — every field and file it names is in the tree today. It is independent of
032 and of 033 and can be worked beside either.

The decisions behind this card are **ADR-0092** (one draw for the whole GUI, text
included), **ADR-0090** (a size is measured from the font in hand, never
hard-coded) and **card 026/027's** findings (nothing is antialiased; the glyph
sheet is the one filtered texture, and why that is not antialiasing). Everything
you need from them is restated here.

## Goal

**The element path draws text.** A second element kind reads the glyph sheet as a
distance field, and `text` says where each character sits so a caller can place
characters itself.

When this lands, **a rectangle and a letter are the same draw command.** That is
ADR-0092's central claim, it is the one thing this card exists to prove, and the
verification is a draw count of one over a picture holding both.

## What already exists, so that nothing here is guessed

Read these before you start. They are the card's real specification.

- **`voe_render_element` in `render/include/render/device.h`** — eighty bytes,
  `bounds`, `clip`, `colour`, `kind`, `reserved_a[3]`, `reserved_b`. Its comment on
  the reserved words names this card: *"A glyph kind needs the rectangle of a sheet
  it reads — four floats — and the index of the texture it reads them from, and
  card 031 adding those may not move anything above or change this size."*
- **`voe_render_element_kind`** — `VOE_RENDER_ELEMENT_SOLID = 0`, and its comment
  says the field exists so that adding a kind moves nothing.
- **`render/shaders/elements.slang`** — the record is declared there field for
  field, and `reserved_b` is commented *"Where card 031's glyph rectangle goes.
  Sixteen bytes on a sixteen-byte boundary, so consuming it moves nothing."*
- **`render/src/descriptors.c` asserts the record's size and every offset.** A
  member that moves is a build error rather than an interface drawn in the wrong
  place. That is your safety net; do not work around it.
- **Set 0, binding 1 is `Sampler2D voe_render_textures[64]`** — every texture at
  once, indexed by a number out of a record (ADR-0018). The element pipeline
  **already has it bound**: it shares the pipeline layout and the descriptor set
  with `draw.slang`, which is why `frame.c` binding the set once at the top of the
  frame survives an element draw. Sampling a texture from the element fragment
  stage therefore needs no new descriptor, no new binding and no new set.
- **`voe_render_median` at `render/shaders/draw.slang:403`** and the threshold at
  `:475` — `step(0.5, median)`. D-166 names that line as the one place the
  threshold lives.
- **`text/src/font.c`** — the atlas is created `VOE_RENDER_TEXTURE_DATA` with
  `VOE_RENDER_SAMPLING_FIELD`, and neither is optional; the comment says why.
- **`struct glyph` in `text/src/font.c:165`** — `x0 y0 x1 y1` the box relative to
  the pen **in ems with +y up**, `u0 v0 u1 v1` the atlas rectangle, `advance`, and
  `drawn`. Its comment says the box is *"the box the sheet actually holds — which
  is ATLAS_MARGIN texels wider on every side than the outline, because the field
  carries on past the outline and the quad has to be big enough to show where it
  says the edge is."* **Read that sentence twice**; see *Where this card is likely
  to go wrong*.
- **`voe_text_font` also holds `line_height`**, from `hhea`, in ems.

## Scope — `render`

### The kind

- `VOE_RENDER_ELEMENT_GLYPH = 1`. A glyph element is a rectangle whose coverage
  comes from the sheet instead of being solid.
- It consumes **`reserved_b` as the sheet rectangle** and **one word of
  `reserved_a` as the texture index**. Two words stay reserved and stay named as
  such.
- **Give the sheet rectangle the record's own `xy`-plus-`wh` shape**, the way
  `bounds` and `clip` are written, rather than a min/max pair — one convention in
  one struct. `struct glyph` holds a min/max pair, so the conversion is at the
  caller and you say so in the header.
- The fields get real names and real comments. `reserved_b` stops being reserved
  and the comment pointing at this card goes away — that is how the record's own
  header stays true.
- **Nothing above may move and the record stays eighty bytes.** If your layout
  cannot honour that, stop and report rather than growing the record: every draw
  built against it changes with it, which is the cost ADR-0092 said was the
  load-bearing one.

### The shader

- The fragment stage already has the element-space position `at` and the record's
  `bounds`. The sheet coordinate is where `at` falls inside `bounds`, mapped into
  the record's sheet rectangle. **Compute it; do not add a vertex attribute** —
  there is no vertex buffer on this path and `at` is passed precisely so a fragment
  need not invert anything.
- Sample `voe_render_textures[texture]`, take the **median of the three channels**,
  and threshold it. **That is `draw.slang`'s answer and it must not be re-derived
  here.** If the median ends up written in two files, both say so and name each
  other; a small shared shader include is better if the build takes it without new
  machinery. Say which you did and why. (D-052 is not triggered by this: both
  shaders are still `render`'s own.)
- **A threshold, not a smoothstep.** Card 026 removed antialiasing; the field is
  filtered so that the 0.5 crossing lands where the distances say it lands, not so
  that the edge is soft. `render/include/render/device.h`'s `VOE_RENDER_SAMPLING_FIELD`
  comment says it in one line: *"This mode moves the edge; it does not blur it."*
- **The record's colour is the glyph's colour**, multiplied by the field's alpha.
  A glyph element has no material, no lighting and no shading record — that is
  exactly what lets a letter and a fill share one draw.
- The premultiply stays where it is: **once, at output**, on the alpha the field
  produced. The file's header states the rule; keep the arithmetic in one place.
- **The clip test applies to a glyph exactly as to a solid**, unchanged. Prove it:
  a clipped letter is what a scroll area will be made of on card 035.

### Capacity

A frame's element capacity is now spent on letters as well as fills — a forty
character label is forty elements. Say in your report what that means for
`voe_render_capacities`' `elements`, and whether its comment needs a sentence.
Do not quietly raise the exhibit's number to make room.

## Scope — `text`

- **Make the per-character metrics public**: the box relative to the pen, the sheet
  rectangle, the advance, and whether the character draws at all — a space
  advances and draws nothing. `struct glyph` is already exactly this, so the work
  is deciding **what of it is public and in what shape**, and that is a header
  question to argue out loud rather than a copy.
- **Make the line height public**, since a caller placing lines needs it and the
  font already holds it. Ems, as it is held.
- **The box stays in ems with +y up, and the header says so loudly.** That is a
  fact about a font, and `text` does not know what an element is. The caller
  multiplies by its own millimetres-per-em — ADR-0090's rule that a size is
  measured from the font in hand — and turns the direction round once, where it
  emits. **A GUI surface runs Y down from its top-left corner** (ADR-0099, decided
  2026-09-09, and `voe_render_element`'s header says it too), so that conversion is
  real and it is exactly one function in one file on card 034 — not in this folder,
  and not in `render`.
- **No layout call, no measure call, and no elements.** `text` must not learn what
  an element record is: laying a string out as elements is `ui`'s work on card 034,
  and a `text` that emitted records would be the start of a second GUI inside the
  font folder. A per-character lookup and the metrics is the whole of this half.
- **`voe_text_block_create` is untouched and stays.** Mesh text for the world is a
  real path with real callers; this card adds the second one deliberately. **Two
  text paths now exist and that is correct — a third would not be.** Write that
  sentence in the header, because it is the thing a later card will be tempted to
  get wrong.

## Scope — the exhibit

`dev/src/elements.c` gains writing. Its own header explains what it is for: a
place a person can look at what the element path does, with the draw count printed
beside it.

- A line or two of text on the panel, at a size where the field's crispness is
  visible, and one label small enough to show what the sheet cannot hold (see the
  `text` header's *no analytic curves* paragraph — it thins rather than blurring,
  and that is worth being able to see).
- **The printed draw count must still say one** for the whole panel, with letters
  and rectangles in it. That number is the exhibit's real output.
- `VOE_DEV_ELEMENTS` is asserted against what is actually submitted; keep that
  arrangement working when the count becomes text-dependent.
- **Do not move the exhibit onto a panel component.** Its header says card 032 does
  that; this card leaves it as the full-screen overlay it is.

## What must not change

State in your report that you checked each of these:

- **No new module edge.** `render` names nothing new. `text` names nothing new —
  it already depends on `render`, which is how it can hold a texture at all.
- **The record's size and every offset above the reserved words.**
- **The mesh text path**, and the atlas's kind and sampling mode.
- **`draw.slang`'s text arithmetic.** Card 025 tuned it and D-166 depends on where
  it lives; this card reads it, matches it, and does not improve it.
- **There is still exactly one Y flip in this engine** (ADR-0033, point 6), and it
  is in the viewport; `voe_render_element_transform` owns the element path's sign.
  This card adds no negation anywhere.
- **No `ui`.** Nothing here knows about widgets, layout or panels.

## Where this card is likely to go wrong

- **Using the outline box instead of the sheet box.** The atlas holds a margin of
  field beyond the outline and the quad has to cover it, or the shader is asked
  where the edge is at a place it was never told about — letters come out with
  clipped stems and it looks like a bad font rather than a wrong rectangle. This is
  the most likely bug on the card and `struct glyph`'s comment is the warning.
- **A glyph record that forgot its texture index.** Slot 0 is the one-pixel white
  texture every unclaimed slot points at, so such a record samples opaque white,
  medians to white, thresholds to one and draws **a solid box** — a plausible
  looking rectangle rather than a visible failure. Make the test catch it.
- **Premultiplying twice.** The field's alpha multiplies the record's alpha; the
  premultiply happens once at output. Three multiplications in the wrong order
  give text that is too faint, which reads as a colour somebody chose badly.
- **Ignoring `drawn`** and emitting a zero-area element for every space, which
  spends capacity on nothing.
- **A zeroed clip rectangle on a glyph.** A zeroed rect clips everything away — the
  record's header says so and it is the one field a zeroed record gets wrong. A
  glyph that is not meant to be clipped is given the whole surface.
- **Interpolating the sheet rectangle.** It is one rectangle for the whole element,
  like `clip`; if it becomes a vertex output it is `nointerpolation`, for the reason
  the file already gives about `clip`.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **`render/tests/elements.c` grows the claims this card adds**, in that file's own
  style — it reads the picture back because nothing on this path can be checked any
  other way:
  - **solids and glyphs in one frame, and the frame holds exactly one draw
    command.** This is the card's headline claim and the assert that proves it.
  - a glyph drawn large enough that its interior reads as the record's colour and
    the paper outside its stems does not.
  - **a glyph clipped by its clip rectangle**, half gone, exactly as the solid
    clip case does it.
  - a glyph element whose texture index is `VOE_RENDER_TEXTURE_NONE` — assert what
    you decided it does, so that the white-box failure above cannot land silently.
  - paint order across kinds: a solid over a glyph and a glyph over a solid.
  - the record is still eighty bytes, said out loud in a test rather than only in
    `descriptors.c`.
- The test needs a graphics card, as that file already does. Say so in its header
  if the arrangement changes.
- **A screenshot of the exhibit** with text and rectangles together, and the draw
  count printed beside it. That is what the principal looks at.
- Windows is the principal's — nothing here is platform code and it should simply
  work.

## Report when this lands

- The record's final layout, field by field, with the words still reserved named
  as reserved and the two comments that pointed at this card removed.
- What `text` made public, in what shape, and what you refused to make public.
- Whether the median is shared between the two shaders or written twice, and why.
- What the exhibit's element count became and what that says about a sensible
  default capacity for a real interface.
- What you deliberately did not build, so the next person knows the glyph kind is
  finished and the GUI is not: no string layout, no measure, no widgets.

## Notes (coder, 2026-09-09, Linux/WSL, claude-opus-5)

**Implemented in full. `cmake -P check.cmake` exits zero — all steps, 37 test
programs, analyser clean over 99 files.** Not in place, for the reason cards 029
and 030 already record about this machine: no `ninja`, `slangc`,
`wayland-scanner` or `pkg-config`, no sudo, and a 9p mount CMake refuses to build
inside. A private toolchain was assembled in scratch (Ninja 1.12.1, Slang
2025.10.3, and Ubuntu `.deb`s for wayland and pkgconf extracted without
installing), the tree was mirrored onto ext4, and the unmodified script was run
on the mirror. The graphics card is llvmpipe, so every picture below was really
drawn and really read back.

**The Khronos validation layer was fetched the same way and put in force over all
six `render` tests and over `dev`, with `VK_LAYER_VALIDATE_SYNC=1`.** The message
set was captured before and after this card's changes and compared: **no new
validation message of any kind.** See *One thing found and not repaired* below
for the two that were already there.

### The card's headline claim, measured

    elements   40 rectangles of 40 colours and 40 letters in 1 draw command; the whole frame took 30

That is `dev`'s own line, with the count read either side of the exhibit and
subtracted rather than asserted. The headless test asserts the same thing
directly — a solid and a glyph in one frame, `draw_count == 1`.

### The report the card asks for

- **The record's final layout.** Still **80 bytes**, and nothing above the spare
  words moved. `bounds` (float4, 0), `clip` (float4, 16), `colour` (float4, 32),
  `kind` (uint, 48), **`sheet_texture`** (uint, 52 — was `reserved_a[0]`),
  **`reserved_a[2]`** (56, still spare and still named so), **`sheet`** (float4,
  64 — was `reserved_b`). `descriptors.c` now asserts on 52 and 64 by their real
  names, and `render/tests/elements.c` says 80 out loud as well. **Both comments
  pointing at this card are gone**: `voe_render_element`'s "card 031 adding those
  may not move anything" and `elements.slang`'s "where card 031's glyph rectangle
  goes".
- **The sheet rectangle is `xy`-plus-`wh`**, as the card asked — but the
  conversion turned out to belong in `text` rather than at every caller, so
  `voe_text_glyph.sheet` already hands it over in that shape and
  `dev/src/elements.c` assigns it straight across. The min/max pair stays private
  to `text/src/font.c`, and both headers say so.
- **What `text` made public.** `voe_text_glyph` — `low` and `high` (the box
  relative to the pen, in ems, +y up), `sheet` (the atlas rectangle, corner and
  size), `advance`, `drawn` — plus `voe_text_font_glyph()` and
  `voe_text_font_line_height()`.
  **The shape was argued rather than copied, and the two halves went different
  ways on purpose.** The box is a low-and-high pair because a corner plus a size
  has to name *which* corner, and naming one would be `text` deciding which way y
  runs — which is exactly what it must not do. The sheet rectangle is a corner
  plus a size because a texture's direction is already settled in this engine
  ((0,0) is its top-left), so there is no decision to duck, and because it makes
  the v-flip a fact stated once in the folder that knows the font instead of a
  trap every caller rediscovers. `sheet.xy` pairs with the box's `(low.x,
  high.y)`, and the header says that loudly, because it is the one thing a caller
  will get wrong.
  **Refused:** the atlas resolution, the em the sheet was measured at, the
  margin, the glyph table, the notdef slot, and anything resembling layout or
  measurement. `voe_text_block_create` is untouched; the header now says in as
  many words that two text paths exist, that this is correct, and that a third
  would not be.
- **The median is written twice, not shared, and both copies name each other.**
  The build takes a shared include only with new machinery, and it needs two
  pieces of it, not one: `voe_render_shaders()` globs `shaders/*.slang` and
  compiles each as a module of its own, so an include-only file would be handed
  to `slangc` on its own; and its `DEPENDS` names only the shader itself, so
  editing a shared file would leave both dependents silently stale — which is a
  wrong picture, not a build error. Both are edits to `cmake/voe.cmake` and
  neither is this card's. So `elements.slang` holds
  `voe_render_element_median()`, `draw.slang` keeps `voe_render_median()`, and
  each header names the other and says they must stay identical.
- **The exhibit's count went from 40 to 80** — forty rectangles and forty
  letters, which is seventeen and twenty-three drawn characters of two lines.
  `VOE_DEV_ELEMENTS` is now two named numbers added together and the exact
  `==` assert is kept on both halves, so changing a string without changing the
  number is still a stop rather than a letter quietly missing. **Nothing was
  quietly raised to make room.**
  **What it says about a real interface:** two short lines of writing cost as
  much as the whole rest of the exhibit. An interface is counted in characters,
  not in widgets. `voe_render_capacities`' comment **did need a sentence** and has
  one: a label is one element per drawn character, a screen of a few dozen labels
  wants thousands rather than hundreds, and at eighty bytes each a thousand is
  eighty kilobytes a frame slot — a number to be generous with.
- **What was deliberately not built.** No string layout, no measure, no wrapping,
  no alignment, no widgets, no panel component, and no UTF-8 decoding on the
  public surface. `dev` walks a pen over an ASCII string itself, in twenty lines,
  and says in its header that this is not the real thing. **The glyph kind is
  finished; the GUI is not.** Card 034 owns the one function that turns a string
  into elements and card 032 owns the panel — the exhibit is deliberately left as
  the full-screen overlay it was.

### Two judgement calls worth knowing about

- **The sheet coordinate is built in the vertex stage, not recovered in the
  fragment stage.** The card says "compute it; do not add a vertex attribute" —
  no vertex attribute was added and none could be, since no vertex buffer is
  bound. It is a vertex *output*: `sheet.xy + corner * sheet.zw`, which is
  algebraically the identical affine map to sending `at` back through `bounds`
  (w is 1 across an element, so the interpolation is exactly linear), and it
  saves a divide per fragment and four flat floats of interpolant. The card
  describes the fragment stage as already having `bounds`; it did not, and adding
  it would have been the more expensive of the two ways to get the same number.
- **The sheet is sampled with an explicit level and the branch on `kind` is kept.**
  `draw.slang`'s rule — every sample before any branch — exists because an
  implicit-level `Sample` needs quad derivatives. There are no mipmaps in this
  engine at all, so `SampleLevel(uv, 0)` asks for the only level there is, needs
  no derivatives, and makes the branch free of that rule entirely. A solid then
  samples nothing rather than fetching a white texel it would discard, and a
  solid record carrying a junk index never reaches a descriptor.

### One thing this card had to add that it did not name

**`shaderSampledImageArrayNonUniformIndexing`, in `render/src/device.c`.** The
element pipeline's texture index comes out of the record the *instance index*
chose, so it differs between fragments of one draw — which is precisely what
`shaderSampledImageArrayDynamicIndexing` does **not** cover. `draw.slang`'s own
comment predicted this ("it would be needed the day the index came from ... the
instance index") and that day is this card. So the subscript is wrapped in
`NonUniformResourceIndex` and the device asks for the matching feature, queried
and not assumed, with a message for the card that lacks it — the same shape the
dynamic one already had. Without both, a panel drawn from two sheets is undefined
behaviour that happens to work. Confirmed in the SPIR-V: `ShaderNonUniform`
capability and `NonUniform` decorations on the access chain and the load.

### One thing found and not repaired

**`spirv-val` rejects the `voe_render_textures` declaration, and it did so before
this card.** The validation layer reports *"Invalid explicit layout decorations
on type ... the UniformConstant storage class has a explicit layout from the
ArrayStride decoration"* for `Sampler2D voe_render_textures[64]` —
`ArrayStride 8` on a `UniformConstant` array, which this `slangc` emits and the
spec forbids. **Proven pre-existing:** a pristine tree built from `HEAD` reports
it too. Declaring the same array in `elements.slang` — which the card requires,
and which is copied verbatim from `draw.slang` — adds a third instance of it.
`-fvk-use-scalar-layout` and `-fvk-use-gl-layout` make no difference, so it is
not spellable around from inside this card; fixing it properly means changing how
that array is declared in `draw.slang` too, which this card is explicitly told
not to improve. Drivers accept the module (llvmpipe does, and every test and the
exhibit pass), but a stricter one need not. **Reported, not repaired — it wants a
card.**

### What was checked against the card's "what must not change"

Each of these was checked against the diff, not from memory:

- **No new module edge.** No `CMakeLists.txt` changed at all, so no `DEPENDS`
  line moved. `render` names nothing new; `text` names nothing new; `dev` already
  depended on `text`.
- **The record's size and every offset above the reserved words.** 80, and
  `clip`/`colour`/`kind` at 16/32/48, asserted in two places now.
- **The mesh text path, and the atlas's kind and sampling mode.** `text/src/font.c`
  is purely additive — the diff removes no code line — and neither
  `VOE_RENDER_TEXTURE_DATA` nor `VOE_RENDER_SAMPLING_FIELD` appears in it.
- **`draw.slang`'s text arithmetic.** Only comment lines changed in that file;
  the code diff is empty. The two comments added are the reciprocal median note
  the card asked for and one correcting the now-stale non-uniform sentence.
- **Exactly one Y flip.** No negation was added anywhere in `render` or `text` —
  the only subtraction in the added code is `u1 - u0`, a width. The one direction
  change is `baseline - g.high.y * em` in `dev/src/elements.c`, at the caller,
  where the card says it belongs.
- **No `ui`.** That folder is not in the diff.

### The screenshot

Rendered headless at 960x540 through `voe_dev_elements_submit` — the same call
`dev`'s loop makes — because this workstation's compositor offers no screenshot
protocol (`grim` refuses: no `wlr-screencopy`). The capture program lives in
scratch, not in this repository.

What it shows, and what to look at:

- The large line, magnified four times: **hard edges, straight stems, square
  corners, no grey pixels anywhere.** That is a threshold through a distance
  field and it is what "nothing is antialiased" looks like when it is right. A
  smoothstep would show a soft fringe and a double premultiply would leave the
  letters grey rather than near-white.
- The small line at two and a half millimetres to the em, magnified eight times:
  **stems thinning and strokes dropping out** — `t`, `h` and `l` lose parts of
  themselves. That is the sheet running out of resolution to describe the shape,
  it is what `text`'s "no analytic curves" paragraph describes, and the card
  asked for a label small enough to show it. **It is the limitation being
  demonstrated, not a bug.**
- The clipped bar is still exactly half drawn, the six alphas still ramp, and the
  thirty-two squares are still thirty-two colours — the letters cost the
  rectangles nothing.

### Not checked here

**Windows.** Nothing in this card is platform code — no `_win32` or `_wayland`
file is touched — and the one Vulkan feature it adds is queried before it is
asked for, so a card that lacks it starts and says so. Per the one-platform rule
(2026-09-04), whatever Windows turns up is a new card.
