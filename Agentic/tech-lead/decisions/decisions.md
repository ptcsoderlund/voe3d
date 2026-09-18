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
