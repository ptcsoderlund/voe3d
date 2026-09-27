# 20 — The 3d headers page lists the model headers
folder: 3d/include/3d
decisions: 0168

## Change
`3d/include/3d/3d.md` has no entry for two headers earlier cards of this feature added. Read the
header comments of `3d/include/3d/model_component.h` and `3d/include/3d/models.h`, then add one
entry each to `3d/include/3d/3d.md`, in the page's spelling, each under 300 characters:

- `model_component.h` — which model a thing wears, as one path field a saved scene keeps.
- `models.h` — the store of loaded models: loaded by path, re-read when the file changes, a failed
  file kept as failed, cleared, and handed to drawing, picking and outlines.

Say what the header says it owns; the phrases above are the points, not the wording. Change no
code.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d/include/3d` prints `FINDINGS: 0`.
