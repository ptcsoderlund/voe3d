# 75 — A frame or card time past its mean is a hitch line
folder: app
after: none
decisions: 0168, 0358, 0458

## Change
Bug 08, the measurement's pure half (0458 points 1 and 2): a frame loop can
tell a stalled frame from its mean and write one line naming what took the
time. No caller yet; card 76 wires the editor.

- `app/include/app/hitches.h` — new public header.
  - `voe_app_stretch { const char *name; double seconds; }` — one named CPU
    stretch of a frame, as the caller measured it.
  - `voe_app_hitches` — zeroed is nothing seen yet: the frame interval's mean
    and sample count, the card total's mean and sample count.
  - `VOE_APP_HITCH_LINE` — the bytes a line needs with its NUL; 256 is
    plenty for five parts of `VOE_RENDER_PASS_NAME`.
  - `bool voe_app_hitches_frame(voe_app_hitches *hitches, double elapsed,
    const voe_app_stretch *stretches, uint32_t count, char *line,
    size_t room)` — folds `elapsed` into the frame mean; true, with the line
    written, when it was a hitch.
  - `bool voe_app_hitches_card(voe_app_hitches *hitches, double total,
    const voe_render_pass_time *passes, uint32_t count, char *line,
    size_t room)` — the same for a measured card frame and its passes.
  - The header's points: the rule of 0458 point 1 (mean of weight 1/60,
    nothing before 60 samples, above 1.5 × the mean taken before this
    sample and above 1/60 s, every sample folded in); why against its own
    mean (a card or scene of any speed); the two lines' format, e.g.
    `frame 52.3 ms (mean 16.6): views 31.0, open 12.4` and
    `card 41.2 ms (mean 9.1): bounce sun shadow 4x 30.1, bounce relight 6.0`
    — the five largest, largest first, one decimal, a name of several
    passes summed with its count; the line is cut to `room`, NUL inside it;
    no line is written when false. Stretches may nest (the caller's choice;
    each is listed as given). A NULL line with a room, or NULL arrays with
    a count, asserts. Writes no file and no stderr: the caller does.
- `app/src/hitches.c` — the two calls; one shared helper ranks the parts and
  writes the line.
- `app/tests/hitches.c` — new, no window, no card. Cases:
  - 60 frames of 16 ms then one of 50 ms with stretches `open` 5, `views`
    40, `end` 5 ms: true, the line starts `frame 50.0 ms` and names `views`
    before `open`.
  - 60 of 16 ms then one of 20 ms: false.
  - 30 of 16 ms then one of 50 ms: false (too few samples).
  - card: 60 totals of 9 ms then 41 ms with passes `bounce sun shadow` 10
    and 20 ms, `bounce relight` 6, `view target 0` 3: true, the line names
    `bounce sun shadow 2x 30.0` first, then `bounce relight 6.0`.
  - seven stretches on a hitch: the line lists five.
  - a room of 16 bytes: the line is NUL-ended within 16.
- `app/app.md`, `app/src/src.md`, `app/tests/tests.md` — an entry each for
  the new header, source and test.

## Done when
Test `app/hitches` passes.
