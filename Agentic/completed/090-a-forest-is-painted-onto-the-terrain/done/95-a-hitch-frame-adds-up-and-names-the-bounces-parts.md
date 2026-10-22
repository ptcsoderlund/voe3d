# 95 — A hitch frame adds up and names the bounce's parts
folder: editor
after: 93, 94
decisions: 0168, 0215, 0458, 0461, 0462, 0463

## Change
Bug 12, the measurement in the editor (0463 points 1, 2, 4): a hitch
frame's stretches lie inside it, `pace` tells the wait from the poll, the
bounce is named by its parts, and the line says whether a light bounced.
Read `voe_app_frame` in `app/include/app/app.h` and `voe_3d_shadow_times`
in `3d/include/3d/draw_system.h` for the fields used.

- `editor/src/frame_hitches.h`, `editor/src/frame_hitches.c`
  - `VOE_EDITOR_FRAME_HITCHES_STRETCHES` 16 → 24.
  - The watch gains `double began`, `_begin`'s clock reading.
  - `_end`'s `double elapsed` becomes `double waited`, the frame's
    `voe_app_frame.waited`. `_end` reads the clock and hands
    `voe_app_hitches_frame` the seconds since `began`, not the tick's.
  - `_end` lists, beside the ones it lists now and a part at 0 left out:
    `pace: wait` (`waited`); `bounce: fit`, `bounce: stale`,
    `bounce: begin`, `bounce: captures`, `bounce: suns`,
    `bounce: relight` from `parts`.
  - A frame line, when written, ends with `, light bounces` when
    `parts.bounced` is above 0, else `, no light bounces`, as the card
    line ends with its counts.
  - The header: the frame is the watch's own span, `_begin` to `_end`,
    and why (bug 12: the tick's span put `pace` past its frame and left
    13 s unnamed); what runs after `_end` is in no frame; `pace: wait`
    within `pace`, the rest of `pace` the clock read and the poll; the
    bounce's parts within `shadows: bounce`, each what 0463 point 3 says;
    the line's bounce ending; the usage example's `_end` call follows.
- `editor/src/view_passes.c` — `shadows_watched` adds the six new seconds
  and `bounced` from `frame->shadow_times` to `watch->parts`, as it adds
  the five.
- `editor/src/main.c` — the `_end` call hands `opened.waited` in place of
  `opened.tick.elapsed`. Nothing else; the file stays at 800 lines or
  under.
- `editor/src/src.md` — the `frame_hitches.*` entries follow, as a phrase.

## Done when
The editor builds, and `grep -c "opened.waited" editor/src/main.c` prints 1
and `grep -c '"bounce: captures"' editor/src/frame_hitches.c` prints 1.

The human, in the editor (bug 12's How to reproduce):
1. Start the editor from a terminal, open lumberhill's `Hill.landscape`
   scene, fly in among the painted trees and keep moving between them,
   the editor focused. In every `editor: hitch: frame` line the named
   stretches (all but those after a colon) add up to about the frame, none
   is longer than it, and the line ends saying whether a light bounces.
2. Click the terminal for a few seconds: frame lines show `pace: wait`
   near 235 ms, nearly all of `pace`.
3. File the lines of step 1 as a new bug of 090 (0463 point 5): the
   `bounce: <part>`, or `pace` beyond its wait, that holds the stall is
   what its fix is planned from.
