# 27 — The game target builds without a GLOB mismatch
folder: cmake
decisions: 0168, 0277, 0266

## Change
Replaces blocked card 26, whose change is already committed (f877777): `cmake/game.cmake`
globs the project folder per top-level folder, skipping `Build` and `Cache`, and its header's
"Sounds and models" paragraph says so. Card 26 blocked only because its proof ran a plain
`cmake --build`, which also builds the engine's test executables with descriptions off, and
`physics/tests/collider.c` does not compile that way. The editor never builds that: Play and
Ship build `--target game` (`editor/src/game_tree.c`, `voe_editor_game_tree_build`). So the
proof builds the target the editor builds.

Open `cmake/game.cmake` only if a step below fails; the fix then stays in its
sounds-and-models block and header, as card 26 described. Change no other file.

## Done when
`cmake -P check.cmake` exits 0, and in the tank game tree the editor wrote while bug 02 was
reported (its scene names both tank models; `ls examples/tank_game/Build/debug` first, and if
it is gone this step is the human's) this exits 0:
`b=examples/tank_game/Build/debug; d=$(mktemp -d); cmake --build $b --target game && rm -rf $b/Assets
&& cmake --build $b --target game >$d/a.log && cmake --build $b --target game >$d/b.log
&& ! grep -q 'GLOB mismatch' $d/a.log $d/b.log
&& test -f $b/Assets/tank_body.glb && test -f $b/Assets/tank_head.glb`

Then `bash ~/.claude/skills/checks/scripts/checks.sh --all` exits 0 and prints `FINDINGS: 0`.

For the human, 036 How to test step 8 on `examples/tank_game` with both tank models placed:
Play shows them lit and shadowed as in the editor, `Build/build.log` has no error and no
`GLOB mismatch`; Ship's game shows them too.
