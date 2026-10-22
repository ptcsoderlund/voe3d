# 11 — A held drag scatters or erases foliage
folder: editor
after: 10
decisions: 0168, 0379, 0442

## Change
0442 point 3: with kind SCATTER or ERASE the held drag stamps foliage, and its release records
the copies it added or removed as one undo step. No Inspector control chooses them yet (card 12).

- `editor/src/sculpt.h` —
  - `voe_editor_sculpt` gains the foliage brush's numbers, never saved or undone like the
    rest: `picked` (bit k: kind k), `density`, `size_vary`, `turn_vary`, `lean`, and `uint64_t
    seed` (nonzero); and for a held foliage stroke `voe_assets_landscape_copy *changed`,
    `changed_count`, `changed_room`, malloced at the press, grown by doubling, freed at the
    release as `before` is.
  - Ranges and defaults beside the sculpt ones, as 0442 point 3 gives them: density 0.1–50
    (4), size vary 0–1 (0.2), turn vary 0–π (π), lean 0–1 (0).
  - Header: the held drag's paragraph says Scatter and Erase stamp copies and the release's
    step carries them; the constraints line names the changed list's growth. Keep it short:
    the header is at 29 lines.
- `editor/src/sculpt.c` (393 lines; if it passes ~550, move the foliage stamp and release to
  `editor/src/sculpt_foliage.c` with its header, declared in `sculpt.h`) —
  - `voe_editor_sculpt_start` sets the defaults and a fixed nonzero seed.
  - With kind SCATTER or ERASE: the press copies no grid; each stamp along the drag (the same
    spacing and seconds share as a sculpt stamp) builds a `voe_3d_foliage_brush` from radius,
    softness and the numbers above and calls `voe_editor_models_scatter` or
    `voe_editor_models_erase` (room: the changed list's free room after growing it to hold
    VOE_3D_FOLIAGE_TRIES more, or for erase the landscape's copy count), appending what changed;
    the release, when anything changed, hands `voe_editor_stroke_new_foliage` to the undo line
    as a sculpt stroke is, and marks the project edited; nothing changed, no step. Escape and
    choosing none end it as for sculpting. If `sculpt.c` switches over the kind, both join it.
- `editor/src/src.md` — the `sculpt.*` entries mention scattering and erasing.

## Done when
The editor builds, and `grep -n "voe_editor_stroke_new_foliage" editor/src/sculpt*.c` prints a
line.
