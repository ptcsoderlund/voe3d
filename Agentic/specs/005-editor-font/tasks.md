# 005 Editor font — tasks

- [ ] 1. `text/` — Pixel Operator beside Oxanium, and the face chosen when a font is created
  - Change: put Pixel Operator Regular in `text/fonts/`, unmodified and unrenamed, with its licence
    beside it, and make `voe_text_font_new` take the face it should read.
    - The file: `PixelOperator.ttf`, 17272 bytes, sha256
      `8d805274eaf227855147153182a96a86ff395ddbc7d2d378095af8831b764a3e`, from the author's own zip
      at `https://dl.dafont.com/dl/?f=pixel_operator` (Jayvee Enaguas / HarvettFox96, version
      2018.10.04-1). Its `LICENSE.txt` from the same zip, 7169 bytes, sha256
      `f4e7f373b9b996950337e8d41a4a2939c2d90b7725e9baf3d5084a22717ad328`, goes beside it as
      `PixelOperator-LICENSE.txt` — renamed only because `OFL.txt` already occupies the plain name,
      and CC0 imposes no notice condition to break by doing so. Check both hashes before committing
      either; a mismatch is `BLOCKED`, not a file used anyway. If the machine has no network, that
      is `BLOCKED: the font file has to be handed over` — do not substitute another face.
    - Verified already, so do not re-derive: sfnt 1.0 with `glyf` outlines, `unitsPerEm` 1600, short
      `loca`, 241 glyphs, hhea ascender 1300, descender -300, line gap 72, a format 4 `cmap` at
      platform 3 encoding 1 — everything `src/truetype.c` requires. Of the atlas range 0x20–0xFF it
      carries every letter; 0x7F–0x9F, and `¤ § ª ­ ¯ ² ³ ¹ º ¼ ½ ¾`, it does not, and those draw
      the font's own missing-glyph box, which is the documented answer.
    - The surface: `voe_text_typeface` in `include/text/font.h` with `VOE_TEXT_TYPEFACE_OXANIUM` and
      `VOE_TEXT_TYPEFACE_PIXEL_OPERATOR`, and
      `voe_text_font_new(voe_render_device *device, voe_text_typeface face, voe_base_arena *arena, voe_base_error *error)`.
      No getter for the face, no from-memory create, no second weight — rule 10; add each the day
      something calls it. One font object stays one face and one atlas.
    - The sampling: a Pixel Operator design pixel is 100 units, 0.0625 em, two texels at the present
      `ATLAS_EM` of 32, while `VOE_TEXT_FIELD_SPREAD` is four — a spread wider than the feature it
      describes, which is how a distance field loses a one-pixel gap. Measure it; if 32 cannot hold
      the stems and counters, the sampling resolution and, if the sheet then overflows, the atlas
      dimension become facts of the face rather than constants of the folder, with the number's
      reason in `src/font.c`'s header as `ATLAS_EM`'s already is. Do not raise it for Oxanium.
    - Headers and `text.md`: `include/text/font.h` says "THE FONT IS OXANIUM REGULAR" and "ONE
      WEIGHT, ONE FONT, NO FALLBACK", and `src/font.c` names Oxanium as the embedded file — both
      become two faces, one weight each, no fallback, and say why the set is an enum and not a path
      (ADR-0167). `fonts/` gets its line in `text.md` updated.
    - Tests: `tests/truetype.c` reads the new file too — the header numbers above as an independent
      answer, the `cmap` covering the letters of the atlas range, and one of the characters it does
      not carry mapping to glyph zero. Whatever settles the sampling question is a claim about the
      field, so it belongs in `tests/raster.c`, with that file's header saying why one real glyph
      now appears where every other case is built by hand: the thinnest thing the sheet must hold is
      a fact about a shipped file and cannot be invented.
    - The four call sites the signature breaks, all downstream, all asking for
      `VOE_TEXT_TYPEFACE_OXANIUM` so that nothing changes behaviour in this task:
      `dev/src/main.c:1438`, `editor/src/main.c:474`, `ui/tests/widgets.c:1891` and
      `ui/tests/widgets.c:1958` (ADR-0113). Name them in the report.
  - Covers: 7, 8 (and the half of 1 that is the font existing)
  - Depends on: -
  - Done when: `cmake -P check.cmake` — exits zero on Linux. Faster while working:
    `ctest --test-dir build/debug -R '^text/'` passes after building `voe_text`.

- [ ] 2. `editor/` — the editor's panels in Pixel Operator
  - Change: take a capture first — `voe_editor --capture /tmp/005-before.png --size 1280x800` — then
    make `editor/src/main.c` create its font with `VOE_TEXT_TYPEFACE_PIXEL_OPERATOR`. That is the
    whole behaviour change: one font, set into the interface exactly as now, so the top bar, Scene,
    Inspector, the browser and any notice all follow it. Do not add a second font, a preference file
    or a way to switch — those wait on 006 and are not this task's to invent.
    `main.c`'s header says which face the editor draws in and why it differs from `dev`'s; `dock.c`
    and `inspector.c` measure from the font in hand already (ADR-0090) and must keep doing so — any
    number that turns out to have been tuned to Oxanium's metrics is fixed by measuring, never by a
    constant. `editor.md` says the editor draws itself in Pixel Operator.
  - Covers: 1, 6
  - Depends on: 1
  - Done when: `cmake -P check.cmake` exits zero on Linux, and
    `build/debug/editor/voe_editor --capture /tmp/005-after.png --size 1280x800` exits zero and
    writes a picture that differs from `/tmp/005-before.png` (`cmp` says so) with every panel's text
    legible and unclipped.
