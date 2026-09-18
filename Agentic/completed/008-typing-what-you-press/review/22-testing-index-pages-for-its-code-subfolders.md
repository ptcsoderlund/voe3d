# 22 — testing: index pages for its code subfolders
folder: testing
decisions: 0173, 0168

## Change

`checks.sh` requires a `<folder>.md` in every folder that directly holds code files (decision 0173), and
`testing/include/testing/testing.md` is missing. Create it, and change nothing else in `testing/`:

- `testing/include/testing/testing.md` — first line `# testing`. One entry: `test.h`, the check macros a test links as `voe::testing`.

`testing/testing.md` is nine lines and keeps its `include/testing/test.h` entry as it is; the new page is the
thin index beside the header.

Every page: exactly one line beginning `#` and it is the first line, no code block and no table, one entry per
code file directly in that folder written ``- `name` — one sentence saying what that file owns``, bare names
only, and nothing listed that is not there. Two or three lines of prose above the list are welcome where they
orient a reader, as `render/src/src.md` already does; prose is never a second heading.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no line beginning `FINDING testing/`, and
`git status --porcelain` shows nothing changed outside `testing/`.
