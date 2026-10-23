# 11 — The editor loop's splash waits get a file of their own
folder: editor
after: 09
decisions: 0168

## Change
`editor/src/main.c` is 798 lines, and card 12 adds a fourth wait to its loop. Split it by function first.
Nothing changes in behaviour.

- New `editor/src/waits.h` / `waits.c`:
  - `[[nodiscard]] bool voe_editor_waits_run(voe_editor_waits *waits)`. It is the loop's three "due"
    blocks, moved as they are: `session.prepare_due` (an import's or Make levels' prepare behind the
    splash, then the reload), `session.reload_due`, and `session.load_due` (New or Open's load behind the
    splash). They sit near lines 450–504 of `main.c`, between `voe_editor_frame_hitches_begin` and
    `voe_editor_session_step`.
  - It returns false where the block today ends the program, which is a false load wait. `main.c` keeps
    what it does then.
  - `voe_editor_waits` is a struct of pointers to what the blocks touch: the app, the ui context, the
    frame scratch, the splash, the session, the scene, the models, the device and the worker's arena.
    Take whatever else the moved code reads.
  - The header comment says these are the loop's splash waits between frames, and points at `loading.h`
    for the works.
- `editor/src/main.c`: the blocks are replaced by one call. Its header comment, where it lists what the
  loop does, points at `waits.h`.
- `editor/src/src.md`: an entry for `waits.h`/`waits.c`.

## Done when
`test $(wc -l < editor/src/main.c) -lt 760` exits 0, and `grep -q voe_editor_waits_run editor/src/main.c`
exits 0.
