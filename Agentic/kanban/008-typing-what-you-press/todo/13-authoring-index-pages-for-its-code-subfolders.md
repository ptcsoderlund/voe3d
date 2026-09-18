# 13 — authoring: index pages for its code subfolders
folder: authoring
decisions: 0173, 0168

## Change

`checks.sh` requires a `<folder>.md` in every folder that directly holds code files (decision 0173), and none of
this folder's three has one. Create them, and change nothing else in `authoring/`:

- `authoring/include/authoring/authoring.md` — first line `# authoring`. A thin index: one entry per public header there. The fuller
  entries stay on `authoring/authoring.md`; this page is one line each, so a reader who has opened the folder can pick a file.
- `authoring/src/src.md` — first line `# src`. One entry per `.c` and `.h` file directly in `authoring/src/`.
- `authoring/tests/tests.md` — first line `# tests`. One entry per test program in `authoring/tests/`, saying what claim it
  makes.

Card 04 changes `voe_authoring_scene_write` first. Describe `scene_write.h` and
`src/scene_write.c` as they are once that card has landed.

Move the `src/…` and `tests/…` entries down out of `authoring/authoring.md` into the two new pages, rewording each into a
bare-name entry. The module page keeps its preamble, its `include`, `src` and `tests` subfolder lines, and
its `include/authoring/…` entries, which are its own duty under decision 0173. Where the module page has no entry for a
file, write one from that file's own header comment; do not drop a sentence you cannot replace.

Every page: exactly one line beginning `#` and it is the first line, no code block and no table, one entry per
code file directly in that folder written ``- `name` — one sentence saying what that file owns``, bare names
only, and nothing listed that is not there. Two or three lines of prose above the list are welcome where they
orient a reader, as `render/src/src.md` already does; prose is never a second heading.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no line beginning `FINDING authoring/`, and
`git status --porcelain` shows nothing changed outside `authoring/`.
