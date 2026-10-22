# 87 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING Agentic/kanban/090-a-forest-is-painted-onto-the-terrain/.replans: stray; not a place the workflow names
FINDING Agentic/kanban/090-a-forest-is-painted-onto-the-terrain/.suite-filed: stray; not a place the workflow names
suite: passed on this code already, not run again (delete .git/agentic-suite-green to force)
FINDINGS: 2
