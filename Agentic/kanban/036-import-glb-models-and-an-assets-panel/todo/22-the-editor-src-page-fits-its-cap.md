# 22 — The editor src page fits its cap
folder: editor/src
decisions: 0168

## Change
In `editor/src/src.md` two entries are over the 300-character cap an entry may have: `dock.h`
and `entities.h`. Shorten each to under 300 characters and rewrap its lines to the page's width.
Keep for `dock.h`: four panels with Scene over Assets on the left, splits holding a side panel's
length or the views' share, the seams and the lit one, the walk. Keep for `entities.h`: Add
entity, a dropped model's thing, duplicate, delete, components given or taken, all through the
structural queue, and that its header says the id and name rules. Change no other entry and no
code.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`.
