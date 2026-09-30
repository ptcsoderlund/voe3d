# 09 — whole suite failed
folder: .

## Change
Make the whole suite pass. The findings below say what is wrong and where.
Split this into cards, one per folder named, in the order the folders depend
on each other.

## Done when
`checks.sh --all` prints FINDINGS: 0.

## Blocked
FINDING examples/tank_game/Code/Code.md: entry `tank_shell_system.c` is 348 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDING examples/tank_game/Code/Code.md: entry `tank_enemy_system.c` is 317 characters, cap 300; one sentence, the rest belongs in the file's header (--entry-cap to raise)
FINDINGS: 2
