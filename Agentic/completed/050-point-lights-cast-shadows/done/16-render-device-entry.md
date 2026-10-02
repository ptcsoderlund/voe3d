# 16 — The render public index entry for device.h fits its cap
folder: render/include/render
after: none
decisions: 0168

## Change
In `render/include/render/render.md`, the entry for `device.h` is 344
characters, cap 300. Cut it to one sentence that says what the header
offers. Whatever it loses that the header comment of
`render/include/render/device.h` does not already say goes into that header
comment; change only the comment, no declaration (other folders include it).
Change no other entry.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render/include/render`
reports no finding for `render/include/render/render.md`.
