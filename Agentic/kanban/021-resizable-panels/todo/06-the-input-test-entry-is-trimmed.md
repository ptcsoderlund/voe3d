# 06 — The input test entry is trimmed
folder: platform/tests
decisions: 0168

## Change
Only `platform/tests/tests.md` changes: its `input.c` entry is 322 characters against the cap of 300
(lengthened by card 01). Shorten it to 300 or fewer while it still names what the test proves: a poll
drains motion, wheel and typed text and keeps held keys and the pointer; focus loss releases keys and
pointer loss buttons; overflowing or control code points type nothing; the pointer's shape survives a
poll and both losses. No code or other entry changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder platform/tests` exits 0 and reports no finding
on `platform/tests/tests.md`.
