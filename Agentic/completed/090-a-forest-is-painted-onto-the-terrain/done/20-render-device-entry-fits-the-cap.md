# 20 — The render device.h entry fits the cap
folder: render/include/render
after: none
decisions: 0168

## Change
The whole suite found index entries in `render/include/render/render.md` over the 300-character cap. Shorten these entries:
- `device.h` (338 characters), header `render/include/render/device.h`

Each entry becomes one sentence of at most 300 characters (the cap counts the whole `- `name` — ...` line). Any point the entry drops that the file's own header comment does not already make goes into that header, kept under its 60-line cap (measure with `checks.sh --header-lines <file>`). Comments and `.md` only; no code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure 2>&1 | grep -F 'FINDING render/include/render/'` prints nothing.
