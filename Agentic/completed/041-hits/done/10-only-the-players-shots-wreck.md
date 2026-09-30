# 10 — Only the player's shots wreck
folder: examples
after: 09
decisions: 0168, 0294, 0296

## Change
Bug 02: enemy tanks turn into wrecks while the player holds fire, because
the swap in `tank_shell_system.c` wrecks any breakable any shell hits, and
enemy shells aimed at the player hit other enemies on the way. The fix is
in the swap, its one owner (0296). Files under `examples/tank_game/`:
`Code/tank_shell_system.c`, `Code/tank_shell.h`, `Code/tank_breakable.h`,
`Code/Code.md`. Read `Code/tank_lives.h` (the row, `tank_lives_key`) and
`scene/include/scene/parent_component.h` (`voe_scene_parent_within`).
Never edit `main.scene` or any prefab (0272).

- `tank_shell_system.c`: each step, find the player's hull once: the
  entity of the `tank_lives` row, none when there is no row. On a hit the
  shell still stops, is removed and its shot row records `hit` and
  `target` as today, so lives still count enemy hits on the player. The
  breakable swap runs only when the shell has a shot row, there is a
  player's hull, and that hull is within the shot's `owner`
  (`voe_scene_parent_within(world, hull, owner)`); otherwise nothing is
  swapped. Header: THE SWAP names the rule and 0296.
- `tank_shell.h`: its sweep paragraph says only the player's shots swap a
  breakable; others stop and change nothing.
- `tank_breakable.h`: first paragraph says a shell the player fired (0296);
  an enemy's shell stops on it and nothing changes.
- `Code.md`: the `tank_shell_system.c` and `tank_breakable.h` entries say
  only the player's shots wreck; each under 300 characters.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -q tank_lives_key examples/tank_game/Code/tank_shell_system.c && grep -q
voe_scene_parent_within examples/tank_game/Code/tank_shell_system.c` exits 0.

For the human, in the editor on `examples/tank_game`: Play, hold no fire,
let enemies shoot for a minute near each other: none turns into a wreck.
Then fire at one: it becomes its wreck (feature step 1).
