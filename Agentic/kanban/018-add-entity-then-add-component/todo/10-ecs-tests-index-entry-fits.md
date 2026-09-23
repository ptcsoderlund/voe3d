# 10 — The ecs tests index entry for component.c fits its cap
folder: ecs/tests
decisions: 0168

## Change
Only `ecs/tests/tests.md`. Its entry for `component.c` is 305 characters against a cap of 300, grown by card
01's "a menu path". Shorten that one entry to at most 300 characters, keeping every promise it lists (a row in
and out, iteration across a removal, a full table, an entity's makeup, runtime-only registration, the typed
intent round trip, a default row, a menu path); merge or tighten phrases, drop none. No other entry and no
code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --agentic` prints no finding for `ecs/tests/tests.md`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --all` reports no finding in `ecs/`.
