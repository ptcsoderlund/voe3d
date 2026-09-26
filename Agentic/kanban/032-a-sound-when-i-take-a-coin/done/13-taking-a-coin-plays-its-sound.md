# 13 — Taking a coin plays its sound
folder: examples/coin_game
decisions: 0168, 0251, 0266
read: feature.md

## Change
The code is already in the tree (card 08's commits): `pickup.wav`, the Coin's `sound` field
defaulting to `pickup.wav`, each take playing it through the step's mixer, `Code/Code.md` and
`coin_game.md` entries. Cards 09–12 cleared the other folders' findings. This card proves the
feature whole; change a file in `examples/coin_game/` only if a proof below fails there, and
never `main.scene` (0251).

`authoring`'s scene reader warns once per coin, `[N.coin] does not say sound; loaded as ...`,
because `main.scene` predates the field; that is expected until the sponsor saves it, and is
excluded from the stderr check.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0,
   `$p/Build/editor/loaded/project-1.so` exists, and
   `grep -iE 'sound|audio|wav' $p/err | grep -v 'does not say sound; loaded as'` prints
   nothing; then
   `CC=clang cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game`
   exits 0 and `$p/Build/debug/pickup.wav` exists; then
   `CC=clang cmake -S $p/Build/game -B $p/Build/release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build $p/Build/release --target game && cmake --install $p/Build/release --prefix $p/out`
   exits 0 and `$p/out/pickup.wav` exists.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test`, steps 1 to 6, in the editor on Linux
   (step 4 with another `.wav` put in the project folder; step 6 as in 031).
