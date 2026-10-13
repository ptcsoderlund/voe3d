# 18 — A parts file follows renames and travels with its glb
folder: editor
after: 17
decisions: 0168, 0378, 0420

## Change
0420 point 1's last two sentences: renaming a `.material` must not undress a model, and a `.glb` must
not lose its `.parts`. Read the headers of `editor/src/assets_walk.h` and `editor/src/assets_manage.h`.

- `editor/src/assets_walk.c`, `editor/src/assets_walk.h`: `.parts` joins the one list of followed and
  asked texts, so a renamed or moved `.material` is followed in every `.parts`, and Delete's question
  names a `.parts` that uses it.
- `editor/src/assets_manage.c`, `editor/src/assets_manage.h`: a `.glb` renamed or moved takes
  `<stem>.parts` beside it to the new stem, when there is one (refused before anything moves if that
  name is taken, as a move is); a duplicated `.glb` gets a copy of its `.parts` under the copy's stem;
  a trashed `.glb` sends its `.parts` to the trash too. The tables are read again after each, as now.
  The header's paragraphs say so.
- `editor/src/src.md`: the two files' entries follow (each at most 300 characters).

## Done when
`voe_editor` builds in the folder's check with no warning. The human: rename an imported model's
material in Assets; the placed model still wears it. Rename the `.glb`; its `.parts` is renamed beside it
and the model still shows its materials.
