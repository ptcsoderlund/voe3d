# 42 — The game's model loader header fits its cap
folder: game/include/game
after: none
decisions: 0168

## Change
`game/include/game/models.h`: its header comment is 63 lines, cap 60. Bring
it to 57 or fewer without losing a fact; the why of one function moves above
that function's declaration in this same file, joining the comment already
there and not repeating it.

- LEVELS paragraph: its last sentence (`_prepare` makes the file alone, kept
  when fresh for the editor's import, made again for its Make levels, 0429)
  goes above `voe_game_models_prepare`, whose comment already says most of
  it; keep the 0429 citation. Rewrap the paragraph: one of its lines runs
  far past the column the others keep.
- MATERIALS paragraph: "Call it after a watch too, so a reloaded model wears
  its parts" goes above `voe_game_models_materials`.
- LANDSCAPES paragraph: the game loading its cooked table before its first
  update goes above `voe_game_models_landscapes`.
- If still over 57, the WATCH paragraph's list of what is not watched (the
  dot, a landscape, a material, with their reasons) goes above
  `voe_game_models_watch`.

Comments only; no declaration changes. `game/include/game/game.md`'s entry
for `models.h` changes only if it quoted a moved sentence.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines game/include/game/models.h`
prints 57 or less and no "over cap".
