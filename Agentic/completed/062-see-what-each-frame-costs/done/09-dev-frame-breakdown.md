# 09 — The frame breakdown in dev
folder: dev
after: 03
decisions: 0168, 0358, 0367

## Change
- New `dev/src/breakdown.h` / `dev/src/breakdown.c`: `voe_dev_breakdown` (shown flag, the shown
  `voe_render_pass_time` rows, at most 64, the total, the label lines, seconds since refresh);
  `voe_dev_breakdown_take(breakdown, gpu, seconds)`, at most four times a second, from
  `voe_render_frame_pass_times` and `voe_render_frame_gpu_time`
  (`render/include/render/device.h`, timing section); and `voe_dev_breakdown_draw(ui, breakdown)`:
  an anchored panel at the surface's top right, a title row, a row per pass with its name and
  milliseconds to two places, and the total. Header: what it shows, why it lags, its cost.
- `dev/src/interface.h` / `dev/src/interface.c`: a third button, "Frame", on the panel toggles the
  breakdown; `voe_dev_interface_draw` takes the `voe_dev_breakdown *` and draws it when shown;
  `VOE_DEV_INTERFACE_NODES` and `_ELEMENTS` grow by the button and the breakdown's stated cost, the
  comment saying so.
- `dev/src/startup.h`: the program holds one breakdown.
- `dev/src/main.c`: one take per frame beside the GPU time read, and the breakdown passed to the
  interface. Keep the file under 800 lines.
- `dev/src/src.md`: entries for the new files; fix `interface.*` and `main.c`.

## Done when
- `cmake --build --preset debug --target voe_dev` exits 0.
- `wc -l < dev/src/main.c` prints under 800.

## Human
Feature step 4: run dev, press Frame; the breakdown lists each pass with its time and the total.
