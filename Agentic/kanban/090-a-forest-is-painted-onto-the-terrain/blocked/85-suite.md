# 85 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING editor/src/src.md: entry `main.c` is 319 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING editor/src/src.md: entry `view_passes.c` is 340 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING Agentic/kanban/090-a-forest-is-painted-onto-the-terrain/replans.md: stray; not a place the workflow names
FINDING Agentic/kanban/090-a-forest-is-painted-onto-the-terrain/suite-filed.md: stray; not a place the workflow names
FINDINGS: 4
