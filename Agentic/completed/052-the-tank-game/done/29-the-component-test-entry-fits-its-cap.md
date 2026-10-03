# 29 — The component test's index entry fits its cap
folder: ecs/tests
after: none
decisions: 0168

## Change
`ecs/tests/tests.md`: the `component.c` entry is 330 characters, the cap is 300. Shorten it to
one sentence well under the cap: rows in and out, iteration across a removal, a full table and a
type's capacity, an entity's makeup, registration markers, the typed intent round trip, defaults
and unsaid rows, a menu path. Merge or drop the long phrasing rather than items; the header of
`ecs/tests/component.c` holds the detail. Read that header only to confirm each dropped detail is
there; if one is missing (the menu path, say), add it there as a phrase. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder ecs/tests` prints no FINDING naming
`ecs/tests/tests.md`.
