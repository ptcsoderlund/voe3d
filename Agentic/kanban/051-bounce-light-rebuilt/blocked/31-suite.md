# 31 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING 3d/tests/tests.md: entry `bounce_scene.c` is 393 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING 3d/tests/bounce_scene.c: header comment is 62 lines, cap 60; what it does, how it is used, its constraints — the why of one function goes above that function (a header that is the thing itself says 'header-cap: <n>' in its own text; --header-cap raises every file's)
FINDINGS: 2
