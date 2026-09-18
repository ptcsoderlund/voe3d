# 21 — dev: index pages for its code subfolders
folder: dev
decisions: 0173, 0168

## Change

`checks.sh` requires a `<folder>.md` in every folder that directly holds code files (decision 0173), and
`dev/src/src.md` is missing. Create it, and change nothing else in `dev/`:

- `dev/src/src.md` — first line `# src`. One entry per `.c` and `.h` file in `dev/src/`.

Move the `src/…` entries down out of `dev/dev.md`, which keeps its preamble and its `src` subfolder line. The
assets sitting beside the code — `logo.png` and the like — are listed on the module page today; list them on the
new page too, because a reader of `dev/src/` needs to know they are there. Card 06 changes `src/main.c` first;
describe it as it is once that card has landed.

Every page: exactly one line beginning `#` and it is the first line, no code block and no table, one entry per
code file directly in that folder written ``- `name` — one sentence saying what that file owns``, bare names
only, and nothing listed that is not there. Two or three lines of prose above the list are welcome where they
orient a reader, as `render/src/src.md` already does; prose is never a second heading.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no line beginning `FINDING dev/`, and
`git status --porcelain` shows nothing changed outside `dev/`.
