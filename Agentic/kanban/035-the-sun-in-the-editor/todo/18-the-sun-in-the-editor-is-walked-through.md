# 18 — The sun in the editor is walked through
folder: editor
decisions: 0168, 0273, 0274
read: feature.md

## Change
Needs cards 01–13, 16 and 17 (card 14 was replanned as 16 and 17). No code unless the walk-through finds a gap; a gap in another folder is
reported in `## Blocked`, not fixed here.

- Coder: run the whole suite and fix only what it finds in `editor/`.
- Coder: `grep -rn "\.direction = {" --include=*.c . | grep -v "^./history\|^./build"` shows no
  `voe_scene_light` built with a direction (a ray's or the render light's own is fine).
- Coder: `editor/editor.md` still describes the folder (the gizmo's two modes and the sun
  marker are in `src/src.md`; the folder line may name "rotate").

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
steps above pass.
