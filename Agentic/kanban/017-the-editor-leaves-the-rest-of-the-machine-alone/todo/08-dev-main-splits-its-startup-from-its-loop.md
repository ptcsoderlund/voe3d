# 08 — dev's main() splits its startup from its loop
folder: dev
decisions: 0168

## Change
The last of three cards that bring `dev/src/main.c` under 800 lines; after 06 and 07 what is left
is its header, the shared constants and `main()` itself, about 800 lines of it. Behaviour is
unchanged.

`dev/src/startup.h`, `startup.c` — new. `main()`'s startup, everything before its `while (true)`
(the arenas, `voe_app_new`, the present request, the world, every exhibit's build, the monitor):
moved into `bool voe_dev_start(struct voe_dev_program *program)`. `struct voe_dev_program`, in
startup.h, holds what the loop then reads — the app, the arenas, the world and the entities and
ids the loop names — as plain members, one per former local, with the comments those locals had.
False when anything refused, printing exactly what main() printed. startup.h's header says the
struct is the program's state, written at startup and by the loop in main.c, and read nowhere
else.

`dev/src/main.c`: `main()` declares the program, calls `voe_dev_start`, runs the loop reading
`program.` fields, and tears down as today. The loop stays in main.c: its order is what main.c's
header gives (ADR-0135). The header's list of what is where is brought up to date.

`dev/src/src.md` gains the two entries; `main.c`'s entry says it is the loop.

## Done when
`checks.sh --folder dev` exits 0, `wc -l dev/src/main.c` is under 800, and `voe_dev` starts, draws
every exhibit, takes its keys and closes as before this card.
