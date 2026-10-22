# 93 — A frame says how long it waited
folder: app
after: none
decisions: 0168, 0215, 0462, 0463

## Change
Bug 12, the wait in app (0463 point 2): a caller can tell 0215's pace
wait from the rest of `voe_app_frame_open`, the clock read and the poll.

- `app/include/app/app.h`
  - `voe_app_frame` gains, last, `double waited`: the seconds this open
    spent in the pace loop before its clock read; nought when focused,
    when the loop drew at once, and on a headless app.
  - `voe_app_frame_open`'s comment: one point on `waited`, what it holds
    and what it does not (the poll after the loop), and that a caller
    timing the open can subtract it to see the poll (bug 12).
- `app/src/app.c` — `wait_for_pace` returns the seconds it spent, read on
  the platform clock before and after its loop; `voe_app_frame_open`
  writes them into the frame. A headless open does not call it and leaves
  nought. The comment above `wait_for_pace` says what it returns.
- `app/app.md` — the `app.h` entry names the wait a frame reports, as a
  phrase. `app/src/src.md` — the `app.c` entry likewise.

## Done when
`app` builds and its tests pass, and
`grep -c "waited" app/include/app/app.h` prints at least 2.
