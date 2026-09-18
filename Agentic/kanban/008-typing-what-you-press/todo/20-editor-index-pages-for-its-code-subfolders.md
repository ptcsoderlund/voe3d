# 20 — editor: index pages for its code subfolders
folder: editor
decisions: 0173, 0168

## Change

`checks.sh` requires a `<folder>.md` in every folder that directly holds code files (decision 0173), and
`editor/src/src.md` is missing. Create it, and change nothing else in `editor/`:

- `editor/src/src.md` — first line `# src`. One entry per `.c` and `.h` file in `editor/src/`, twenty-three of them.

Move the `src/…` entries down out of `editor/editor.md`, which keeps its preamble and its `src` subfolder line.
`editor.md` names only the twelve headers today, so the eleven `.c` files need an entry written from each
file's own header comment. At 145 lines `editor.md` is the longest module page in the tree and this is what
brings it down.

Every page: exactly one line beginning `#` and it is the first line, no code block and no table, one entry per
code file directly in that folder written ``- `name` — one sentence saying what that file owns``, bare names
only, and nothing listed that is not there. Two or three lines of prose above the list are welcome where they
orient a reader, as `render/src/src.md` already does; prose is never a second heading.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no line beginning `FINDING editor/`, and
`git status --porcelain` shows nothing changed outside `editor/`.
