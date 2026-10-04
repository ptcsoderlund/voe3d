# 08 — session.h's header and its src.md entry fit their caps
folder: editor/src
after: none
decisions: 0168

## Change
Prose only; no code changes.

- `editor/src/session.h`: the header comment is 61 lines, cap 60. Tighten it
  to 60 or fewer without dropping a rule: e.g. rewrap the over-long line in
  the NEW/OPEN paragraph ("project, its row chosen (0343). SAVE NEVER
  ARMS...") and merge short lines so the paragraph loses at least one line.
  Keep every point, the ADR numbers and the capitalised lead phrases.
- `editor/src/src.md`: the entry for `session.h` (line ~105) is 335
  characters, cap 300. Cut it to one sentence under 300: the project being
  worked on with its Play, Refresh, Ship and open prefab, and the
  refuse-once rule. Any detail the cut drops must already be in (or move
  into) session.h's header.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints
`FINDINGS: 0`.
