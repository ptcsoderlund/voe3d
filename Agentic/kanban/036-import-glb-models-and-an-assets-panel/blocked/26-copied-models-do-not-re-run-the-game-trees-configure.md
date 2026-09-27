# 26 — Copied models do not re-run the game tree's configure
folder: cmake
decisions: 0168, 0277, 0266

## Change
Needs card 25. Bug 02's second half: every build after a new `.wav` or `.glb` prints
`GLOB mismatch!` and re-runs CMake, because the `CONFIGURE_DEPENDS` recursive glob in
`cmake/game.cmake` walks `Build/`, where `game_files` has just put its copies, and only drops
those paths after the glob has recorded them. Change these files and no others:

- `cmake/game.cmake`, the sounds-and-models block: list the project folder's top level once
  (`file(GLOB ... LIST_DIRECTORIES true CONFIGURE_DEPENDS)`); take its own `.wav` and `.glb`
  files, and for every directory in it other than `Build` and `Cache` a `GLOB_RECURSE
  CONFIGURE_DEPENDS` of `*.wav` and `*.glb` inside that directory. The `^(Build|Cache)/` test on
  relative paths goes; each copy, install and `game_files` stay as they are. A top-level folder
  added later is still picked up (the top listing is itself a configure dependency).
- The header's "Sounds and models" paragraph: drop "The glob walks those folders too ... would
  lift that cost"; say instead that the glob is per top-level folder so the copies under
  `Build/` never count as a change and a build re-runs CMake only when the project's own files
  change.

## Done when
`cmake -P check.cmake` exits 0, and in the tank game tree the editor wrote while this bug was
reported (its scene names both tank models; `ls` it first, and if it is gone this step is the
human's) this exits 0:
`b=examples/tank_game/Build/debug; d=$(mktemp -d); cmake --build $b && rm -rf $b/Assets && cmake --build $b
>$d/a.log && cmake --build $b >$d/b.log && ! grep -q 'GLOB mismatch' $d/a.log $d/b.log
&& test -f $b/Assets/tank_body.glb && test -f $b/Assets/tank_head.glb`
(the first build compiles card 25's `scene.h` and takes in the new `game.cmake`).

Then `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0 and prints `FINDINGS: 0`.

For the human, 036 How to test step 8 on `examples/tank_game` with both tank models placed:
Play shows them lit and shadowed as in the editor, `Build/build.log` has no error and no
`GLOB mismatch`; Ship's game shows them too.

## Blocked
The `game.cmake` change is in and works: the Done-when sequence run with `--target game` exits 0, shows no `GLOB mismatch` and copies both tank models back, and `checks.sh --all` gives `FINDINGS: 0`. The card's plain `cmake --build examples/tank_game/Build/debug` fails in another folder: the game tree builds the engine's tests with descriptions off, and `physics/tests/collider.c:70` calls `voe_physics_collider_description()`, which does not exist in that build. A physics card that guards or moves that test (or a game tree that does not build engine tests) would unblock it.
