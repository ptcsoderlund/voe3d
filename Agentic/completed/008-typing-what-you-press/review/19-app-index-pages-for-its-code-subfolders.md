# 19 — app: index pages for its code subfolders
folder: app
decisions: 0173, 0168

## Change

`checks.sh` requires a `<folder>.md` in every folder that directly holds code files (decision 0173), and none of
this folder's three has one. Create them, and change nothing else in `app/`:

- `app/include/app/app.md` — first line `# app`. A thin index: one entry per public header there. The fuller
  entries stay on `app/app.md`; this page is one line each, so a reader who has opened the folder can pick a file.
- `app/src/src.md` — first line `# src`. One entry per `.c` and `.h` file directly in `app/src/`.
- `app/tests/tests.md` — first line `# tests`. One entry per test program in `app/tests/`, saying what claim it
  makes.

Move the `src/…` and `tests/…` entries down out of `app/app.md` into the two new pages, rewording each into a
bare-name entry. The module page keeps its preamble, its `include`, `src` and `tests` subfolder lines, and
its `include/app/…` entries, which are its own duty under decision 0173. Where the module page has no entry for a
file, write one from that file's own header comment; do not drop a sentence you cannot replace.

Every page: exactly one line beginning `#` and it is the first line, no code block and no table, one entry per
code file directly in that folder written ``- `name` — one sentence saying what that file owns``, bare names
only, and nothing listed that is not there. Two or three lines of prose above the list are welcome where they
orient a reader, as `render/src/src.md` already does; prose is never a second heading.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no line beginning `FINDING app/`, and
`git status --porcelain` shows nothing changed outside `app/`.
