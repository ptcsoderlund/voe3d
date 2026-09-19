# 05 — An object record carries a colour
folder: render
decisions: 0168, 0191, 0177

## Change
`include/render/device.h`: `voe_render_object` gains `voe_math_float4 colour` after `reserved[3]`, at offset 144.
The struct becomes 160 bytes. Its comment: linear, multiplied into the shading record's base colour factor,
alpha included; (1, 1, 1, 1) leaves the record as it is; a zeroed one draws black, so every call site sets it.
Update the file-top example.

`src/descriptors.c`: size assert 160, and a new offset assert for `colour` at 144. `shaders/draw.slang`: the
matching `float4 colour` in `voe_render_object`, multiplied into the base colour where the fragment stage reads the
shading record's factor. Update `shaders/shaders.md` if it describes the record.

Call sites, all set to `{1, 1, 1, 1}`: `tests/transient.c`, `tests/passes.c`, `tests/elements.c`,
`tests/targets.c`, `tests/offscreen.c` (both). Downstream (ADR-0113): `3d/src/draw_system.c`'s `object_of` sets
`(1, 1, 1, 1)`. Card 07 replaces that line with the shape's colour.

`tests/offscreen.c` (or whichever test already reads a drawn pixel back): one more check draws an object with
colour (1, 0, 0, 1) over a white shading record and reads back red with green and blue near 0.

## Done when
The folder's check passes (`checks.sh` for `render`) with the new colour check. The `3d` check passes too:
`top=3d` in the `## Checks` command. With `C=$(mktemp -d)`, `./build/debug/editor/voe_editor --capture $C/o.png` exits 0, and reading
the PNG (ADR-0177) shows the untitled grey cube as before.
