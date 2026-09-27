# 21 — The sun in the editor is walked through
folder: editor
decisions: 0168, 0273, 0274
read: feature.md

## Change
Needs cards 19 and 20. Card 18's machine checks passed except the findings those two
fix; the editor's own finding is already fixed. No code unless the suite finds a gap; a gap
in another folder is reported in `## Blocked`, not fixed here.

- Coder: run the whole suite and fix only what it finds in `editor/`.
- Coder: `grep -rn "\.direction = {" --include=*.c . | grep -v "^./history\|^./build"` shows no
  `voe_scene_light` built with a direction (a ray's or the render light's own is fine).

Human, in `examples/coin_game` (`## How to test` in `feature.md`):
1. The sun shows as a circle and an arrow in both views.
2. Clicking the marker selects the sun: the marker takes the outline's colour and the
   Inspector shows colour, intensity, fill colour and fill intensity, and its transform.
3. R: the top bar reads Rotate, rings stand on the sun. Dragging a ring swings the shadows in
   both views while the drag goes on.
4. R again: Move, the arrows are back. Select the player; R; a ring turns it.
5. Colour orange, intensity half: dim and orange at once. Raise the fill intensity: the
   shadowed sides lighten and the shadows stay.
6. Ctrl+Z undoes each step in turn.
7. Change something, Save, close, reopen: it is kept.
8. Play: the game is lit as the editor shows it.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`; the human's eight
steps above pass (the coder hands them over; they are not the coder's to see).
