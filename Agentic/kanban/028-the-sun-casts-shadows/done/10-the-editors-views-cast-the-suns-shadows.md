# 10 — The editor's views cast the sun's shadows
folder: editor
decisions: 0168, 0258, 0238
read: feature.md

## Change
Proof only; this card finishes the feature. The editor change of card 08 is already committed
(e231387: `editor/src/view_passes.h`, `editor/src/view_passes.c`, `editor/src/src.md` — shadow
passes before each view's pass and the preview's, capacities grown). Card 08 blocked only on a
finding in `game/tests`, fixed by card 09. Change nothing unless a proof below fails inside
`editor`; then fix it there.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder editor` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0 and `$p/err` is
   empty.
3. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
4. The human's: `feature.md`'s `## How to test` steps 1–9 on `examples/capsule/` — shadows in
   both views; turning the light swings them; the ledge shades the capsule; in Play the jump's
   shadow stays below and shrinks on landing; walking, edges stand still; far out distant things
   keep shadows and close up the edge is crisp; 100 km along X looks the same in editor and
   game; deleting the light unlights, undo restores; the game runs as smoothly as before.
