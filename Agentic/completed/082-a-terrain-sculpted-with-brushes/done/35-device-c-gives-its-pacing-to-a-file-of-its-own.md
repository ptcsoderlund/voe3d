# 35 — device.c gives its pacing to a file of its own
folder: render/src
after: none
decisions: 0168

## Change
`render/src/device.c` is 834 lines and card 36 changes it. Split by function
first; no behaviour changes, bodies move verbatim.

- New `render/src/pacing.c`: the whole "timing and pacing" section of
  device.c moves here — `learn_timing`, `learn_present_modes`,
  `voe_render_present_set`, `voe_render_present_get`,
  `voe_render_frame_gpu_time`, with their comments. The two statics become
  `voe_render_timing_learn` and `voe_render_present_modes_learn`, same
  arguments. It includes `startup.h`. Its header comment: what the file
  holds (the card's clock and the present modes asked once at startup, the
  present mode set and read, the frame's GPU time); that it was cut from
  device.c for length; that open_device still calls the two learns, and
  where in its order.
- `render/src/startup.h`: declare the two learns, one comment line each;
  the header's opening list of steps that live beside device.c gains them.
- `render/src/device.c`: the section is gone; open_device calls the new
  names at the same place. Header comment: "This file keeps ... timing,
  present modes ..." drops those two and names pacing.c among the files
  beside it.
- `render/src/src.md`: the `device.c` entry drops "timing, present modes";
  a `pacing.c` entry is added after `startup.h`'s, one phrase on what it
  holds.

## Done when
- `test $(wc -l < render/src/device.c) -lt 740` exits 0.
- `grep -n "static void learn_" render/src/device.c` prints nothing.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
  prints FINDINGS: 0.
