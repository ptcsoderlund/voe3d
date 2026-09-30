# 10 — the tank game's Code index entries fit the cap
folder: examples/tank_game/Code
after: none
decisions: 0168

## Change
Only `examples/tank_game/Code/Code.md` changes. Two entries are over the
300-character entry cap:

- `tank_shell_system.c` (348): shorten to one sentence — a shell's flight,
  hit, wreck swap and removal, with its hit and explosion sounds.
- `tank_enemy_system.c` (317): shorten to one sentence — an enemy's drive,
  aim, fire with shot sound, engine hum and removal.

Each file's header already carries the detail dropped from the entry
(read the first ~40 lines of each to confirm; no header change is needed
unless a dropped point is missing there). No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/tank_game/Code`
prints no FINDING naming `Code.md`, and each of the two entries is at most
300 characters.
