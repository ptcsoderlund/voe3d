# 021b — text

status: todo
claimed-by: -
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
