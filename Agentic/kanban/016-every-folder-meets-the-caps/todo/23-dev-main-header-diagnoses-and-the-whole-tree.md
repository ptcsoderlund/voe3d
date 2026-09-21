# 23 — dev's main.c header, its diagnoses, and the whole tree passes
folder: dev
decisions: 0168, 0210
read: feature.md

## Change
Comments only, by 0210's method, second of two cards on `dev/src/main.c`. Bring its header to 55 lines or
fewer. What is left to move is "WHAT IS WRONG IF IT LOOKS WRONG", "WHAT IS WRONG IF IT FEELS WRONG",
"WHAT THERE IS TO TRY" and the spinning-core paragraph. An item that diagnoses one exhibit or input goes
above the function that builds or drives it (`add_the_quads`, `add_text`, `camera_motion`, …; `grep -n
"^static\|^int main"` lists them); what is about the running program as a whole — trying it, closing it,
the spin — goes above `main`. Read only around each function you place text above.

`dev/src/src.md`'s `main.c` entry changes only if it points at the header for text that moved.

This is the feature's last card: every other folder was cleared by cards 01–22. A finding outside
`dev` is not fixed here; block the card and name it.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --folder dev` exits 0; 0210's token check prints nothing; and
`bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0` (steps 1–3 of `## How to test`:
it runs `cmake -P check.cmake` and the whole ctest suite).

The human: opens `voe_editor` on a saved level and sees it start, draw both views, and select, move with
the gizmo, undo and redo as before (step 4); opens `ui/include/ui/layout.h` and finds a short top comment
saying what the file is for, the reasoning above the functions it explains (step 5).
