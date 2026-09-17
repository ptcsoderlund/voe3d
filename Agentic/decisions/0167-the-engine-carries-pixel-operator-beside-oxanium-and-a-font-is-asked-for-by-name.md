# 0167 The engine carries Pixel Operator beside Oxanium, and a font is asked for by name

Status: accepted
Date: 2026-09-17

Spec 005 wants the editor's own panels drawn in Pixel Operator while the dev program and anything
built on the engine keep Oxanium, and spec 006 wants a theme to name its font — from the fonts the
program carries, since nothing loads one from disk (D-074 is still deferred). `text` embeds exactly
one face and `voe_text_font_new` takes no say in which, so the first question is where a second face
lives and how a caller asks for it. ADR-0062 embedded Oxanium and left a second face to whatever
needed one; ADR-0090 made the GUI font-agnostic and named the typeface a theme role.

## Decision

**Both faces are embedded in `text/fonts/`, and a font is created for one of them by name.**
`voe_text_font_new` takes a `voe_text_typeface` — `VOE_TEXT_TYPEFACE_OXANIUM`,
`VOE_TEXT_TYPEFACE_PIXEL_OPERATOR` — and one font object stays one face, one atlas, one weight and
no fallback. A program that draws in two faces creates two fonts; `ui` is handed one of them and
still names none (ADR-0090).

**The set is an enum, and neither a path nor a caller's bytes.** The property ADR-0062 bought is
that a font cannot be missing, and it is the property 006 needs when a theme file names a font: the
theme names something from this enum, so a theme naming a font is a lookup that cannot fail at a
file's mercy. A create taking bytes is the runtime-loading shape and belongs to the day something
loads a font.

**Pixel Operator Regular ships under CC0 1.0 Universal** — the dedication is in the font's own
copyright string — unmodified and unrenamed, with its licence text beside it as OFL.txt is beside
Oxanium. CC0 asks for no notice; it travels anyway, because a reader of `fonts/` should not have to
look up which of two files is under what.

**How finely a face is sampled into its sheet is a fact of the face.** `ATLAS_EM` is 32 texels to
the em because Oxanium's thinnest stem is about 0.08 em; Pixel Operator's whole design grid is
0.0625 em, so its stems and its counters are two texels against a field spread of four. Where that
does not hold, the resolution — and the atlas dimension, if the sheet then overflows — moves for the
new face alone, and the reason goes in `src/font.c` beside the existing one.

**ADR-0062 stands and is not superseded.** It decided Oxanium is the default and that a second face
arrives with whatever needs one; this is that arrival.

## Rejected

- **The editor embeds the file itself and `text` grows a from-memory create** — the spec's own
  wording, and it puts the runtime-loading surface in the tree for an editor-only reason while
  leaving 006 with no way to let a theme name the face. Seventeen kilobytes in a game's binary is
  the whole cost of not doing it.
- **Swap Oxanium for Pixel Operator everywhere** — criterion 7 says the dev program keeps Oxanium,
  and Oxanium is the logo face.
- **A font-name string rather than an enum** — a name is spelled wrong at run time and answers with
  a failure or a silent fallback, neither of which a face that is in the binary should ever have.
- **A second weight, or bold** — nothing asks (rule 10), and Pixel Operator's bold is in the same
  zip on the day something does.
- **Fetching the file at configure time** — the fetched side of the build is empty and stays empty.

## Consequences

- Every binary linking `text` carries both faces, about seventeen kilobytes more, whether it draws
  in one of them or both.
- A program drawing in two faces pays for two atlases — a megabyte each at 512 square — because a
  font is one face. Nothing shares a sheet between faces and nothing should start.
- `text`'s public surface changes for every caller: four call sites move with the signature, and
  each one now says out loud which face it draws in, which is what makes criterion 7 checkable.
- Pixel Operator does not carry `¤ § ª ¯ ² ³ ¹ º ¼ ½ ¾` of the atlas range; each draws the font's own
  missing-glyph box, per ADR-0062's no-fallback rule. An interface that needs one of those has to
  say so.
- The face and its sampling being separable makes a third face cheap and makes it obvious that
  adding one is a measurement, not a copy of a constant.
