# 23 — dev's main.c header, its diagnoses, and dev.md's opening
folder: dev
decisions: 0168, 0210, 0211

## Change
Comments and `.md` text only, by 0210's method, second of two cards on `dev/src/main.c`. Bring its header to 55 lines or
fewer. What is left to move is "WHAT IS WRONG IF IT LOOKS WRONG", "WHAT IS WRONG IF IT FEELS WRONG",
"WHAT THERE IS TO TRY" and the spinning-core paragraph. An item that diagnoses one exhibit or input goes
above the function that builds or drives it (`add_the_quads`, `add_text`, `camera_motion`, …; `grep -n
"^static\|^int main"` lists them); what is about the running program as a whole — trying it, closing it,
the spin — goes above `main`. Read only around each function you place text above.

`dev/src/src.md`'s `main.c` entry changes only if it points at the header for text that moved.

`dev/dev.md`'s opening is 891 characters against 400: by 0211, it keeps what the program is for (the
one program a person runs to see what the engine can do; looked at, not asserted on). The list of what
the window holds goes to the exhibits' functions in `main.c` where their paragraphs do not already
say it, and to `main.c`'s header, 55 lines or fewer, what is about the whole.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder dev` exits 0; and 0210's token check prints nothing.
