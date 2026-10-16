# 66 — The levels file's header fits its cap
folder: 3d/include/3d
after: none
decisions: 0168

## Change
`3d/include/3d/model_levels.h`: its header comment is 61 lines, cap 60.
Bring it to 57 or fewer without losing a fact; the why of one function moves
above that function's declaration in this same file, joining the comment
already there and not repeating it.

- STALE paragraph: the parenthetical history of the versions (1 to 3 predate
  the strength, the swap and six levels, 0430 point 7, 0432 point 2; 4
  changes no field, only remakes the files made when the default count was
  four, 0433) goes above `voe_3d_model_levels_read`; keep every citation.
  Rewrap the paragraph: one of its lines runs far past the column the others
  keep.
- If still over 57, the STALE paragraph's last two sentences (an index is not
  checked against its part's vertices; the hash ties it to its `.glb`, the
  store holds the vertex counts) go above `voe_3d_model_levels_read` too.
- The comment above `voe_3d_model_levels_make` has one line running past the
  column; rewrap it.

Comments only; no declaration changes. `3d/include/3d/3d.md`'s entry for
`model_levels.h` changes only if it quoted a moved sentence.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/include/3d/model_levels.h`
prints 57 or less and no "over cap".
