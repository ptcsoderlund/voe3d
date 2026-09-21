# 22 — dev's main.c header, its tour, moves to the exhibits
folder: dev
decisions: 0168, 0210

## Change
Comments only, by 0210's method, first of two cards on `dev/src/main.c` (2840 lines, header 555 lines today).
This card takes the header's part before the paragraph headed "WHAT IS WRONG IF IT LOOKS WRONG" (about lines
11–308): the paragraphs on each exhibit — the cubes, the see-through quads, the sign and the camera-locked
line, the screen on the left, the readout and its metrics, the model, the sun, the flown camera — each above
the function that builds or drives that exhibit (`add_the_cubes`, `add_the_quads`, `add_text`, `add_a_model`,
`build_the_readout`, `sunlight`, `camera_motion`, `orbit`, `report` and their neighbours; `grep -n "^static\|^int main"`
lists them). What holds for the whole program (lines 1–16 and the loop's order) stays in the header, tightened.
The "WHAT IS WRONG…", "WHAT THERE IS TO TRY" and spinning-core paragraphs are the next card's: leave them
where they are, in the header. Read only around each function you place text above.

`dev/src/src.md`'s `main.c` entry changes only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder dev` prints exactly 2 FINDING lines, `dev/src/main.c`'s header cap, at 300 lines or fewer, and `dev/dev.md`'s opening, untouched by this card; its
product check passes; and 0210's token check prints nothing.
