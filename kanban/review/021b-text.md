# 021b — text

status: review
claimed-by: claude-code (kanban-coder)
blocked-by: 021a

Split out of the old card 021 (ADR-0068). The old card was thin and provisional
and told the coder to pick between two mechanisms; **that is decided now**, along
with the font, the blending underneath it and how an unlit surface is drawn. This
card is a brief, not a sketch. Four ADRs stand behind it — **0062** (the font),
**0069** (premultiplied output), **0070** (atlas, not panel) and **0071**
(unlit) — and each is summarised where it matters below, so this card can be
implemented without reading them.

## Goal

Text on screen. Written by us — no font library (ADR-0023). A string, in the
world, in Oxanium, anti-aliased, readable.

## What is already decided, so nothing here is a choice

- **A glyph atlas, not an offscreen panel** (ADR-0070). Glyphs are rasterised
  once into one texture; a run of text becomes one mesh of quads sampling it. No
  render-to-texture, no second target. The panel is card 023's, for widgets.
- **One text block is one object: one mesh, one material, one draw.** Not one per
  glyph. This is the load-bearing rule of the card — it is what keeps a sentence
  from becoming hundreds of blended objects with hundreds of sort keys.
- **The font is Oxanium Regular, `.ttf`, embedded** (ADR-0062). It cannot be
  missing and there is no file I/O and no asset path involved.
- **The material is unlit and blended** (ADR-0071, ADR-0061). Unlit is a flag on
  the material that skips the BRDF; blended is the alpha mode card 021a added.
  Text is not lit by the sun.
- **Everything is in 3D space** (ADR-0049). Text is quads in the world or locked
  to the camera. There is no screen-space path and a "just for debug" one is
  exactly what that rule exists to prevent.

## The thing to know before starting: text on this card does not change

`voe_render_geometry_create` waits for the GPU to go idle and appends to a pool
that has no destroy — its own header says streaming geometry in while drawing is
*"a different mechanism and a different card"*. So a text block is **built once,
at startup, and stays**. Rebuilding one per frame would stall the GPU and leak
pool space until it ran out.

**This means the statistics readout stays on the console.** It is the most
obvious consumer of text and the one this card cannot serve, because it changes
every frame. That is D-092 and it is a real follow-up card, not an oversight —
knowing it now is better than discovering it after writing a TrueType reader.

## Scope

### The font in the tree

Oxanium Regular `.ttf` into `text/fonts/`, **with the OFL text beside it,
unmodified, carrying its copyright line** — the licence permits embedding
*provided the notice travels*, so this is not paperwork. The file is neither
renamed nor modified, which is what keeps the reserved-name clause from engaging.
Embedded via `#embed`, the mechanism ADR-0046 already proved with shaders.
**Report D-072 when this lands**: nothing in the three-builds shape surfaces a
third-party notice to an end user yet, and the obligation starts now.

### The TrueType reader, written by us

Read only what this file needs and nothing else (rule 10 — a card specifying
surface nothing uses is over-specified). The tables:

| Table | For |
|---|---|
| `head` | units per em, and `indexToLocFormat` — which decides whether `loca` is 16- or 32-bit |
| `maxp` | glyph count |
| `loca` | where each glyph's outline is |
| `glyf` | the outlines — **simple and composite, see below** |
| `cmap` | character to glyph. **Format 4 is enough**: Oxanium covers Adobe Latin 3, which is entirely inside the basic multilingual plane |
| `hhea`, `hmtx` | advance widths, ascender, descender, line gap |

A table that is missing, or a version this reader does not know, is
`VOE_BASE_ERROR_UNSUPPORTED` naming which — the same shape `assets` already uses.
The font is embedded and known, so a failure here is a bug in the reader rather
than a bad file, and it should say so.

### Rasterisation into the atlas

- **Quadratic Bézier curves**, which is what `glyf` outlines are — flattened to
  line segments at a tolerance the code states.
- **Non-zero winding**, which is what TrueType means. **Contour direction is
  load-bearing**: get the sign wrong and the counters fill in — the holes in `o`,
  `e`, `a`, `p` go solid. It is visible in the first word rendered, which is the
  good kind of bug.
- **Coverage into the alpha channel, white in the colour channels — including in
  the transparent gaps between glyphs** (ADR-0069). That is what keeps a filtered
  glyph edge from picking up whatever was left beside it. Upload as
  `VOE_RENDER_TEXTURE_COLOUR`: white stays white through the sRGB decode, and
  alpha is never decoded, so the coverage arrives untouched.
- **Do not pre-multiply the atlas.** The shader multiplies colour by alpha once,
  at output (ADR-0069). An atlas that is already premultiplied would be
  multiplied by its coverage twice, and the result is text that looks thin and
  washed out rather than obviously broken.
- One atlas, one texture, built when the font is first asked for — not at device
  creation (ADR-0066: nothing costs a frame in a program that draws no text).

### Layout

- Advance widths from `hmtx`; line height from `hhea`'s ascender, descender and
  line gap.
- **Kerning: look at the file before promising it.** The card asks for kerning
  only if the font carries a legacy `kern` table. A modern Google Fonts release
  usually puts kerning in OpenType Layout (`GPOS`) instead, and parsing that is a
  substantial project sitting behind a one-word requirement. **If there is no
  `kern` table, drop kerning, say so in the notes, and do not start on `GPOS`** —
  that is a card of its own and this one does not become it by accident.
- Refused here, by name: shaping, ligatures, bidirectional text, vertical
  scripts, hyphenation, word wrap, rich text, tab stops.

### The mesh and the material

Four vertices and six indices per glyph, one mesh per text block, UVs into the
atlas. The material is the atlas as base colour texture, a tint as base colour
factor, `unlit` set, alpha mode `blended`.

### `dev`

One static string in the world, and one locked to the camera, so both placements
are shown. Keep it small — this is the card's *Verify*, not a demo.

## Where this card is likely to go wrong

- **Composite glyphs, and this is the trap of the card.** Most accented
  characters in Adobe Latin 3 are stored as *references to other glyphs* with an
  offset, not as outlines. A reader that handles only simple outlines renders
  `é`, `ü`, `å` and most of the promised coverage as **blanks** — and looks
  perfectly correct on the first English test string. Test with an accented
  character before believing the reader works.
- **Contour direction and the fill rule**, above. Counters filling in.
- **The Y axis.** Font units are Y-up, texture rows go down, and the engine is
  Y-up with one viewport flip already in place (ADR-0033). Three conventions
  meeting in one place; state which way each goes in a comment rather than
  flipping signs until it looks right.
- **Units per em.** Font units are not pixels and are not world units. One scale,
  named once, applied in one place.
- **Double multiplication by coverage**, above. It looks like text that is simply
  too faint.
- **The first string is English.** Every trap on this list except the fill rule
  survives an English test string.

## Verify

- A string in the world reads correctly, at an angle and from both sides of the
  camera's orbit.
- A string locked to the camera stays put as the camera flies.
- **An accented character renders** — this is the composite-glyph test and it is
  the one most likely to fail.
- The counters in `o`, `e`, `a`, `B`, `8` are holes and not filled.
- Glyph edges are smooth, and text is **not** noticeably fainter than the tint
  colour asks for — that is the double-multiply.
- The text is not lit: it does not change as the sun in `dev` goes round.
- The atlas is built once; drawing a second string with the same font does not
  build a second one.
- A program that draws no text builds no atlas.
- The OFL file is in the tree beside the font, unmodified. D-072 reported.
- Whether the font had a `kern` table, and what was done about it, is in the
  notes.
- `cmake -P check.cmake` exits zero. Windows is the principal's.

## Refused, and each is a later card rather than a judgement call

Hinting — it is a bytecode interpreter, and writing it ourselves makes it a very
expensive yes. Signed distance fields (D-091). Subpixel/LCD rendering, which is
wrong the moment a quad turns. Subpixel positioning. Atlas eviction — the atlas
is built once and holds what it holds. A second weight, italics and faux-bold
(ADR-0062 embeds one weight). Font fallback and glyphs Oxanium does not carry
(D-075). Fonts loaded from disk (D-074). Text that changes after it is built
(D-092).

## One question this card answers by existing

**D-084** — whether `text` adds a phase to the frame or shares one. It arrives
from the frame-loop decision (ADR-0065) and names this card as its trigger. This
card builds text at startup and draws it through the existing draw phase, so the
honest answer is likely *no new phase*; say which, in the notes, because the next
folder beside `3d` inherits it.

## What was done

### `text`, a new folder — `render`, `math`, `base`

The card puts the font in `text/fonts/`, so the `#embed` that reads it is in
`text/src/` and the reader with it. The folder sits beside `3d` rather than under
it, which is where CLAUDE.md's folder tree already had it planned: it turns a
string into a mesh and a texture and knows nothing about entities or files.

- `src/truetype.h` / `.c` — `head`, `maxp`, `loca`, `glyf`, `cmap`, `hhea`,
  `hmtx` and nothing else. Every read goes through one bounds-checked cursor with
  a single `ok` flag; a directory entry past the end, a `loca` entry outside
  `glyf` or a point count that does not fit is `MALFORMED` at the read that
  noticed. `UNSUPPORTED` for an sfnt version that is not TrueType outlines and
  for a `cmap` with no format 4 in it.
- **The composite walk is an explicit stack (rule 14)**, limit
  `VOE_TEXT_COMPOSITE_DEPTH`, deeper is refused. A component that is scaled, or
  positioned by matching points rather than by an offset, is `UNSUPPORTED` by
  name rather than silently mis-drawn — Oxanium uses neither, which was checked
  against the file before deciding not to implement them (rule 10).
- `src/raster.h` / `.c` — quadratics flattened to a stated tolerance in *pixels*,
  non-zero winding, coverage anti-aliased with eight sub-scanlines per row and
  exact fractional spans in x. **The Y flip between font space and an image is
  one function**, `to_bitmap`, and is the only negation in the folder.
- `src/utf8.h` / `.c` — one character at a time. Overlong sequences, surrogate
  halves and anything above U+10FFFF are `U+FFFD`, and it never consumes zero
  bytes, so walking a mangled string ends.
- `include/text/font.h` / `src/font.c` — the atlas built once and the layout.
  Four vertices and six indices per glyph, one mesh per block, wound corner for
  corner the same way `dev/src/quad.c` already proves survives culling.

### The atlas

1024 x 1024, **eighty pixels to the em**, U+0020 to U+00FF plus the font's own
missing-glyph box for anything outside that. Shelf-packed; the range fills about
seven hundred rows, so there is room for it to grow. White in the colour channels
everywhere — the gaps between glyphs included — with the coverage in alpha, **not
premultiplied**, uploaded as `VOE_RENDER_TEXTURE_COLOUR`.

It is built by `voe_text_font_new` and nowhere else, so **a program that draws no
text builds no atlas**, and a second block with the same font builds no second
one. The three scales (font units, atlas pixels, metres) and the three Y axes are
each named once, in `font.c`'s header.

### `unlit`

`voe_render_shading_values.reserved_b` became `uint32_t unlit` — **same offsets,
same size, `render/src/descriptors.c` untouched**, the same trick card 021a used
on `reserved_a`. `draw.slang` gained one early return beside the one a missing
normal already takes, and its header says the two mean different things.
`voe_3d_material` carries it and `material_component.c` uploads it. `assets` does
not read glTF's unlit extension and was not asked to: an unlit material is one a
call site built.

### `dev`

A sign three lines high above the cubes and one line locked to the camera. The
sign is **two entities sharing one mesh**, one of them turned half a turn about
Y: a text block is one face and the engine culls back faces, so a single sign
would vanish for half the lap. The heads-up line is placed by a transform intent
every frame from where the camera actually is — an ordinary object in the world,
because there is no screen-space path and there is not going to be one.

## Two folders the card did not name, and why each was touched

- **`cmake/voe.cmake`** — a row for `text` (`render math base`) and `text` added
  to `dev`'s. voe.cmake's own header says adding an edge belongs in a card; this
  is that card, because it puts the font in `text/fonts/` and CLAUDE.md's folder
  tree already names `text` as planned, on `render`. Worth a look on the
  principal's pass, since it is the one architecture-shaped line in the diff.
- **`math`** — `voe_math_quat_mul`, with a test. Locking a string to the camera
  needs the camera's yaw and pitch as one rotation and `math` had no way to
  compose two; `math/tests/quat.c` already carried a paragraph saying "if a later
  card adds `_mul`, this is the check it has to agree with", and the new case
  agrees with it. The alternative was writing the Hamilton product out at a call
  site, which is not what `dev` is for.

## The questions this card was asked to answer

- **The `kern` table: there is none.** Oxanium Regular carries `GPOS`, `GSUB` and
  `GDEF` and no legacy `kern` at all — checked against the file before writing
  anything. So there is **no kerning**, per the card, and no start was made on
  `GPOS`. `include/text/font.h` says so where a reader will find it.
- **D-084 — no new phase.** Text is built at startup and drawn through the
  existing draw phase: a block is a `voe_render_geometry` and the atlas is a
  `voe_render_texture`, so an entity wearing them goes through
  `voe_3d_draw_system_run`'s blended pass like any other blended object. The next
  folder beside `3d` — sprite, ui — inherits that.
- **D-092 stands.** A block is built once and cannot change, so the statistics
  readout stays on the console. Nothing here moves it.
- **D-072, reported.** `text/fonts/OFL.txt` is now in the tree, unmodified,
  carrying its copyright line, beside `Oxanium-Regular.ttf`, which is also
  unmodified and not renamed. **Nothing in the three-builds shape surfaces that
  notice to an end user, and the obligation starts now** — the font is in the
  binary from this card on. That is the principal's to place.

## Verified — Linux (Wayland, NVIDIA RTX 4070 Laptop, Vulkan 1.4.341)

`cmake -P check.cmake` **exits zero**: 11 standalone folder configures, the root
build, the four guards, the include check, **31 tests passed** (the three new
`text` ones among them), and the analyser over 86 files with no finding.

Three tests, none of which needs a graphics card:

- `text/tests/truetype.c` — the reader against the shipped font, with the numbers
  taken out of it by a separate tool first so they are not this reader's own
  output written down. **The composite-glyph case is the one that matters**: `é`,
  `ü` and `å` come back with more contours than a plain `e` and reaching higher
  than it, which is the accent placed by its offset rather than dropped.
- `text/tests/raster.c` — the fill rule as the one pair of cases that tells
  non-zero from even-odd: two squares wound opposite ways leave a hole, the same
  two wound the same way do not. An `o` on its own would pass with the wrong rule
  implemented, which is why it is not the test.
- `text/tests/utf8.c` — every malformed shape, and that walking a mangled string
  ends.

Looked at, in `dev`, screenshots through the orbit and through the sun's lap:

- **A string in the world reads correctly, at an angle and from both sides of the
  orbit.** The sign is lettered on both faces, so it is readable for the whole
  lap rather than half of it.
- **A string locked to the camera stays put.** Same place on screen, frame after
  frame, while the camera orbits.
- **Accented characters render.** `ÅNGSTRÖM · éüåÇ` is on the sign for exactly
  this reason and every one of them is right.
- **The counters are holes.** `O`, `D`, `e`, `a`, `o`, `b`, `ö`, `å`, `ü` — all
  open, none filled.
- **Edges are smooth and the text is not faint.** Magnified three times there is
  no stepping, no colour fringe and no dark halo — the last of those is what a
  black-gapped atlas gives. The tint arrives at the colour it asks for, so
  nothing is multiplied by coverage twice.
- **The text is not lit.** Screenshots at two points in the sun's lap: the cubes,
  the figure and the quads go from nearly black to fully lit between them, and
  both strings keep exactly the same colour.
- **The lit path still works.** Same pair of screenshots — replacing `reserved_b`
  with `unlit` changed nothing about the cubes, the models or the blended quads.
- **The atlas is built once.** Three text entities, one font, one texture; the
  font is what holds it and `voe_text_font_new` is the only thing that builds
  one.
- **A program that draws no text builds no atlas.** By construction: nothing in
  `render` or `3d` builds one, and `voe_text_font_new` is the only call that
  does.

**Windows untested.** One machine, one operating system on it.

### Markers

None. No `DEVIATION:` and no `BLOCKED:`.

### Suggestions, not in the diff

- **Nothing tests the layout.** The advance accumulation, the newline and
  `block.size` are only checked by looking at `dev`, because
  `voe_text_block_create` uploads to a device and `text` cannot read pixels back
  — that needs `render`'s internals, which `render/tests/offscreen.c` reaches and
  nothing above it can. Either a `text` test on a headless device that checks the
  vertices before they are uploaded, or a way to lay out without uploading. Worth
  a card either way.
- **A character outside U+00FF is the missing-glyph box.** An em dash stood in
  `dev`'s heads-up line until it was looked at, and it renders as a box —
  correctly. If the range should grow, or a second atlas page should exist, that
  is D-075's card and not a line here.
- **The atlas is one size for ever.** Eighty pixels to the em, magnified above
  that. Text filling a screen goes soft, which is the trade ADR-0070 made and
  what D-091's signed distance fields would change.
- **`#embed` is invisible to CMake's dependency scanning**, so nothing rebuilds
  `text/src/font.c` if the font file changes. The same is already true of `dev`'s
  picture and its two models; `render` solves it with `OBJECT_DEPENDS`. The font
  will not change, so this is a note rather than a bug.

## After the principal's first look

**The line locked to the camera lagged the camera by one frame, and it read as
jitter.** Reported on the first run. It was placed with the other intents, above
the systems, so `facing_the_camera` read the camera the *previous* frame's camera
system had written, while the frame was drawn with this one's.

Fixed by moving that one submission down, into the gap between
`voe_scene_camera_system_run` and `voe_scene_transform_system_run`: it is now
derived after the camera has moved and drained before the frame is drawn, so the
line and the view matrix come from the same camera. Both systems still run once
and nothing else moved.

**The comment that was there was wrong twice, and both halves are worth writing
down.** It said the alternative would need the transform system to run twice — it
does not, because the camera system already runs first and the gap between them
was always there. And it said one frame was invisible at any frame rate worth
having, which is exactly backwards: a mouse delivers motion in lumps, so at a
thousand frames a second most frames turn the camera by nothing and the
occasional one turns it by a whole lump. The error was never a fraction of a
millimetre — it was one whole mouse movement, for one frame — and mailbox shows
whichever frame is newest when the display asks, so a person sees some of them.
Drawing faster made it worse. The replacement comment in `dev/src/main.c` says
that at the site.

Not caught by anything automated, and nothing here could have caught it: the
placement is a call site's ordering and `dev` has no tests. In the orbit the lag
is under two millimetres and invisible, which is why the screenshots did not show
it — it needed a hand on the mouse.

## Blurry text, and it was one of my constants rather than the design

Reported on the second look: the text is soft, and sharp-but-pixelated would be
preferred. **The atlas-not-panel decision is not what caused it.** `ATLAS_EM` is,
and I set it too high.

Measured rather than guessed. In `dev` at the window the compositor gives, one em
of the heads-up line lands on about thirty-two screen pixels and one em of the
sign on about twenty-six. The atlas stored them at **eighty**. So both were
minified by two and a half times, which means the sampler reads them out of the
mipmap chain — somewhere between the half-size and the quarter-size copy, blended
between the two — and that blend is blurrier than either. Drawing the same line
at eighty pixels to the em came out crisp, which is what proved it was the
minification and not the rasteriser.

**Bigger is not sharper, and that is the opposite of what the number looks like.**
Eighty was chosen for headroom and headroom is exactly what makes it worse. The
sharpest atlas is the one whose texels are about the size of the pixels the text
lands on.

Changed to **forty pixels to the em in a 512 atlas** — one megabyte instead of
four, about four hundred rows of five hundred and twelve used, so the range still
has room to grow. `GLYPH_PIXELS` came down with it. Three sizes were built and
photographed side by side: eighty is visibly soft, forty is sharp, thirty-two is
marginally sharper still. Forty was taken because thirty-two is already 1:1 at
this window and would go soft again on a bigger one. `font.c`'s header now says
all of that at the site, because the next person to reach for that number will
reach upwards.

**This does not make text sharp at every size, and nothing here can.** A fixed
atlas is crisp at one distance: the sign still minifies as the camera pulls away
and would magnify if it were walked up to. That is the standing consequence of
ADR-0070 and it is what D-091's signed distance fields exist to remove.

### The other lever, not taken, and it needs a card in `render`

Clamping the sampler to mip 0 for the atlas would be sharp at **any** size, which
is literally what was asked for. It is not in this diff for two reasons. It is a
`render` change and not a `text` one — `voe_render_texture_create` generates a
full chain for everything and there is one sampler — so it wants a card. And the
cost is not "pixelated", it is **shimmering**: minified text sampled from mip 0
drops thin strokes and crawls as the camera moves, which in motion is usually
worse than the softness it removes. Worth doing if the sizing above is not
enough; worth seeing the sizing first.

## The heads-up line is hidden by geometry, and this one is not mine to fix

Reported on the second look: fly into a cube and the writing disappears behind
it. **That is this card working exactly as written, and it is worth saying so
before anything else** — the card's own *What is already decided* section says
"Everything is in 3D space (ADR-0049). Text is quads in the world or locked to
the camera. There is no screen-space path and a 'just for debug' one is exactly
what that rule exists to prevent." `dev/src/main.c`'s header says the same in the
other direction: that flying into a cube puts the cube in front of the writing.
So there is no defect here to repair.

**The mechanism, confirmed by putting the line nine metres out instead of one:**
it vanishes completely behind the cubes and the figure. The blended pipeline
tests depth and does not write it (ADR-0061, card 021a — "Depth test stays on.
Only the write goes off"), so any opaque surface nearer than the line wins the
test. Nothing about text is involved; a blended quad would do the same.

**But the requirement behind the report is real, and the decision it needs does
not exist yet.** So this is reported, not worked around, and the card stays in
`review/` with `dev` untouched.

### The reframe, because "screen space" is probably not what is wanted

"Always on top" and "screen space" are two different things and only one of them
is refused. A heads-up line can stay a quad in the world, with a transform, a
metre in front of the eye — and simply be drawn in a pass that does not test
depth against the scene. **That is still entirely in 3D and ADR-0049 survives
it.**

What the engine is actually missing is a **layer**: which pass an object is drawn
in. It has two of the three axes already — the alpha mode says how a fragment
blends (ADR-0061), and `3d/depth_sort` says in what order the blended ones go
(card 021a) — and nothing at all says whether an object belongs to the world or
to something drawn over the top of it. That is the gap, and it is an ADR, not a
line of code.

### What the shapes are, and what each costs

- **A third pipeline: blend on, depth test off, drawn after everything.** Small —
  one pipeline in `render` beside the two card 021a built, one more walk in
  `voe_3d_draw_system_run`, and some way for an entity to say it belongs there.
  Keeps every other decision intact.
- **A layer with its own depth, cleared between passes.** More machinery, and it
  is what a GUI actually wants: see the catch below.
- **A screen-space orthographic overlay.** The simplest thing for a GUI and the
  one ADR-0049 refuses by name. It also means text needs two paths rather than
  one, because text in the world is still wanted.
- **Not an option: drawing it nearer.** Anything can be got in front of it, and
  the near plane clips whatever is closest.

**The catch that decides between the first two.** Turning the depth test off does
not only stop the scene occluding the layer — it stops the layer's own objects
occluding each other. One line of writing does not care. A GUI of overlapping
panels does, and it would be back to relying on the order the draws are issued
in, which is the blended sort again inside the layer. So card 023 probably wants
the second shape and not the first, and picking the first now is picking it for
023 as well.

### Who this belongs to

**Card 023 is blocked on this and card 022 probably is too**, so it is not
something to settle inside a text card. The root's rule is that the tech lead
resumes when a card consumes an open question; this is one, and it wants an ADR
before either of those two moves. I am the coder and I have not decided anything
here — the options above are input to that conversation, not a recommendation.

## One change in `dev` that is not this card's, asked for directly

**Escape now closes the window when the camera is not being flown.** Flying, it
hands the camera back as it always did; with the camera already back, it closes.
Two presses and not one, because the first thing anybody wants out of a locked
pointer is the pointer — a key that released the pointer and quit in the same
press would quit every time somebody wanted their mouse back.

**It had to become an edge, and that is the part worth reviewing.** Escape was
level-triggered, which was harmless while it only ever handed the camera back:
holding it just kept handing back. With two meanings, held down it would hand the
camera back on one frame and close the window on the very next, so a single long
press would look like the program exiting for no reason. It now works the same
way Tab and P already do, off `escape_was_down`.

The startup line and the two paragraphs in `dev/src/main.c`'s header that
described the old behaviour are updated with it.

**It is in this diff and it is nothing to do with text.** It arrived as a direct
request while the card sat in `review/`, and it is four lines in a call site, so
it was done rather than turned into a card — but it is called out here so the
review is not surprised by a key binding in a text card.

**Not exercised: the keypress itself.** There is no way to send a key to a
Wayland client from this session — no `ydotool`, no `wtype`, no `/dev/uinput` —
so what was checked is that it builds under `-Werror`, that the `break` leaves
the frame loop into the existing cleanup rather than any nested one, and that the
program still starts, draws and prints the corrected line. Pressing the key is
the principal's.

## Answered — the tech lead, 2026-09-06, and nothing here changes this card's scope

**The escalation above was right in every part and it is now decided: ADR-0074.**
This note is appended so the review is not read without the answer beside it. **No
scope moves onto this card**, `dev` stays as you left it, and the work is card
024.

**Your reframe was the decision.** *"Always on top" and "screen space" are two
different things and only one of them is refused* — that sentence is what the
question turned on, and *"what the engine is actually missing is a layer"* names
the third axis correctly beside the alpha mode and the sort. Both are quoted in
the ADR.

**Your catch decided the mechanism.** The tech lead's own recommendation was a
flat orthographic layer in pixels, and the principal overruled it — on headsets,
which no record mentioned until that moment: an orthographic panel has no position
in space, so there is nothing to hand two eyes, and it would have been the easy
default long before anyone found that out. That left your first two shapes, and
**the catch you named picked between them**: turning the depth test off buys
ordering against the world by giving up ordering *within* the layer, which is free
with a depth clear and is exactly what card 023's overlapping panels need. So the
overlay is **a depth clear between the world and the layer**, and your prediction
that 023 wants the second shape is the reason.

**What was decided, in short:** an object says whether it is world or overlay;
overlay content keeps real positions in metres and is seen through the same
camera; depth is cleared between the two; inside the layer the ordinary blending
and sorting rules apply unchanged. Both placements are first-class and neither is
the default. The layer decides order and nothing else — unlit stays a material
property, exactly as you have it.

**One thing you were right about that was nearly missed.** *"There is no defect
here to repair"* is correct and the card is not at fault: it built what had been
decided, and the gap it exposed was a question knowingly left open. Reporting it
and leaving `dev` untouched was the right call and is worth more than a
workaround would have been.

**Not yours, and not card 024's either:** `facing_the_camera` stays for now.
Making *"I sit this far in front of the camera"* the engine's job is decided in
principle, but its shape is still open and it gets its own card.
