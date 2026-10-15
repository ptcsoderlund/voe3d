# 15 — The store's header fits its cap
folder: 3d/include/3d
after: none
decisions: 0168

## Change
`3d/include/3d/models.h`: its header comment counts 62 lines against a cap
of 60. Bring it to 57 or fewer without losing a fact: the why of one function
moves above that function's declaration, in this same file.

- The `.material` paragraph's last two sentences (what `_material_set` marks,
  what `_material_frame` rewrites inside a frame so a drag needs no upload)
  go above `voe_3d_models_material_set` / `voe_3d_models_material_frame`,
  joining whatever comment is already there, not repeating it.
- The `.landscape` paragraph's sentences on brushes, `_put`, the dirty rect
  and `_landscape_frame`'s budget a frame (an upload between frames would
  stutter a drag) go above `voe_3d_models_landscape_frame`, likewise.
- If still over, the `_parts_set` sentence in the "A PART WEARS" paragraph
  goes above `voe_3d_models_parts_set`.

Comments only; no declaration changes. `3d/include/3d/3d.md`'s entry for
`models.h` changes only if it quoted a moved sentence.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines 3d/include/3d/models.h`
prints 57 or less and no "over cap".
