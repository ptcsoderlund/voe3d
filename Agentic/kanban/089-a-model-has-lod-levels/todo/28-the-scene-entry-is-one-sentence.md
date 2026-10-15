# 28 — The Scene panel's index entry fits its cap
folder: editor/src
after: none
decisions: 0168

## Change
`editor/src/src.md`: the entry for `scene.c` is 311 characters, cap 300.
Cut it to one sentence under 260 characters: the selection and its targets,
the open material's close, follow and refresh, Delete, Duplicate and the
Inspector's switch, and the Scene panel's rows, asked after the frame ended.
Any detail dropped (the gizmo's mode, the colour picker and dropdown targets,
folds, the reveal's unfold and scroll) must be in `editor/src/scene.c`'s
header comment; it already names most of them, add any it does not, keeping
the header within its cap. No code changes.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --structure` prints no
FINDING line naming `editor/src/`, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines editor/src/scene.c`
prints no "over cap".
