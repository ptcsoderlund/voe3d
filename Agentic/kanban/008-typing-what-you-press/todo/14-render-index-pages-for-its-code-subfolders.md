# 14 — render: index pages for its code subfolders
folder: render
decisions: 0173, 0168

## Change

`checks.sh` requires a `<folder>.md` in every folder that directly holds code files (decision 0173), and
`render/include/render/render.md` is missing. Create it, and change nothing else in `render/`:

- `render/include/render/render.md` — first line `# render`. One entry: `device.h`, which is this folder's whole public surface.

`render/src/src.md` and `render/tests/tests.md` already exist and already pass; they are the model this
repository's other folders are being brought to (decision 0173), so read `render/src/src.md` before writing the
new page and do not change either of them. `render/render.md` keeps its `include/render/device.h` entry: the
new page is the thin one. `render/vulkan/` and `render/shaders/` are untouched — `vulkan/` is exempt and
`shaders/` holds no code the check counts.

Every page: exactly one line beginning `#` and it is the first line, no code block and no table, one entry per
code file directly in that folder written ``- `name` — one sentence saying what that file owns``, bare names
only, and nothing listed that is not there. Two or three lines of prose above the list are welcome where they
orient a reader, as `render/src/src.md` already does; prose is never a second heading.

## Done when

`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no line beginning `FINDING render/`, and
`git status --porcelain` shows nothing changed outside `render/`.
