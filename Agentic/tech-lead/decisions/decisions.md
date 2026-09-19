# Decisions

One line each: number, title, one sentence. Records 0001–0167 were written under the two earlier workflows and
live unchanged in `history/decisions/`, with their own index; `ADR-NNNN` in code means the record of that number
wherever it lives. Numbering continues here from 0168.

- `0168-the-agentic-workflow-replaces-specs-and-the-engines-rules-are-one-digest.md` — The Agentic workflow replaces specs; the old records move to `history/`; the engine's standing rules are restated once, numbered as code cites them.
- `0169-a-keysym-is-a-number-a-key-has-four-levels-and-altgr-is-a-place.md` — A keysym is a number as often as a name, a key has four levels, and AltGr is a place the keymap names.
- `0170-a-theme-is-read-in-theme-derived-in-ui-and-nearest-wins.md` — A theme is read in `theme`, derived in `ui`, and the nearest one wins.
- `0171-the-palette-is-derived-in-oklab-and-dark-mode-clamps-chroma.md` — The palette is derived in OKLab from one colour and two numbers, and dark mode clamps chroma.
- `0172-a-theme-is-one-file-in-a-themes-folder-and-a-program-remembers-it-by-file-name.md` — A theme is one file in a themes folder, and a program remembers which by its file name.
- `0173-a-code-subfolder-carries-its-own-index-page.md` — Every folder holding code carries `<folder>.md` as an index; `src/` and `tests/` entries move down out of the module page, `include/` gets a thin one.
- `0174-006s-finished-cards-are-merged-back-not-redone.md` — `feature/006-themes-old` is merged into `feature/006-themes` by the next `/drive`, with the four conflicts resolved as written; the planner then re-cuts `blocked/05`.
- `0175-a-new-folders-card-registers-it-in-the-build.md` — The build is the coder's: a card that creates a folder adds its `cmake/voe.cmake` row and root `add_subdirectory` line, and `checks.sh` allows those two files under a named decision.
- `0176-themes-row-names-render-for-its-tests-device.md` — `theme`'s row is 0170's plus `render`, so its test can make the headless device a label needs.
- `0177-to-see-what-was-drawn-render-to-a-png.md` — A coder checks what was drawn by rendering a PNG with `voe_editor --capture` or the headless app. The planner names this decision on cards that change what is drawn.
- `0178-two-built-in-themes-remembered-by-a-reserved-name.md` — The editor has two built-in themes, Near black (entry 0) and Near white (entry 1, light mode); the remembered line is `near_white` for Near white and empty for Near black.
- `0179-the-editors-built-ins-take-pixel-operator-and-a-font-override-is-one-line.md` — Near black and Near white take Pixel Operator; the editor's font override (theme's own, `pixel_operator`, `oxanium`) is one line in `<settings>/voe3d/font`, applied to a copy of the chosen palette.
- `0180-a-wayland-window-draws-at-the-compositors-fractional-scale.md` — On Wayland the window binds fractional-scale and viewporter, draws its buffer at the preferred scale, and reports its size and pointer in buffer pixels; closes D-188's Wayland half.
- `0181-bug-02-of-009-closed-as-the-compositors.md` — Bug 02 of 009 (keyboard held) did not reproduce after a reboot: closed as a transient Wayland/KWin fault, card 04 withdrawn; if it recurs, a new bug on its own feature starting from the four key tests.
- `0182-editor-text-keeps-scaling-and-hard-edges.md` — Editor text keeps scaling with the window (ADR-0104) and hard edges (ADR-0078); the edge cutoff is tuned so no stroke loses all its pixels, one pixel wider is accepted. Display-scale sizing and anti-aliased text were rejected.
- `0183-a-glyph-pixel-counts-if-it-is-within-049-of-a-pixel-of-the-outline.md` — The element pipeline's glyph cutoff moves 0.49 screen pixel outside the outline, measured from the sheet coordinates' derivatives, so no stroke of editor text vanishes; world text is unchanged.
- `0184-the-glyph-cutoff-sits-half-a-pixel-out-less-one-byte-step.md` — The glyph cutoff is `0.5 - 0.5 * field_per_pixel + 1/255`, clamped to 1/512..0.5, so an aligned outline keeps its width despite byte rounding; amends 0183's 0.49 margin.
- `0185-oxanium-is-the-only-font-and-pixel-operator-is-dropped.md` — Oxanium is the only font: Pixel Operator, its licence and the editor's font override are removed; both built-in themes use Oxanium, and any other `font=` falls back to it. Replaces 0179.
