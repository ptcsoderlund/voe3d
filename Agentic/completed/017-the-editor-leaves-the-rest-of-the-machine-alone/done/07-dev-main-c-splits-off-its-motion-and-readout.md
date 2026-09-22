# 07 — dev's main.c splits off its motion and readout
folder: dev
decisions: 0168

## Change
The second of three cards that bring `dev/src/main.c` under 800 lines. It moves code and changes
nothing the program shows or prints. Each function moves with the comment block and the `#define`s
above it that only it reads; a constant `main()` also reads stays in main.c.

- `dev/src/motion.h`, `motion.c` — new. The flying section: `camera_motion` and its key paragraph,
  `orbit`, `sunlight` and the orbit constants: how the eye and the sun move each frame.
- `dev/src/readout.h`, `readout.c` — new. `struct timing`, `say_what_is_measured`,
  `build_the_readout`, `report` and `REPORT_SECONDS`: what is measured, the on-screen readout and
  the line printed every report interval. The paragraph at the top of main.c on the frame's two
  numbers (today near line 144) goes with them if only they read it.

A moved function loses `static` and takes a `voe_dev_` name; its call sites in main.c follow. Each
new header says what the file owns and why it is its own file. Where main.c's header points at a
paragraph that moved, it points at the new file.

`dev/src/src.md` gains the four entries.

## Done when
`checks.sh --folder dev` exits 0, `wc -l dev/src/main.c` is under 1300, and `voe_dev` flies, orbits
and prints its timing report as before this card.
