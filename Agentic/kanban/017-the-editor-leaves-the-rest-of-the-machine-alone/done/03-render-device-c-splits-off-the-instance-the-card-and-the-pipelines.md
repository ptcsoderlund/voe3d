# 03 — device.c splits off the instance, the card and the pipelines
folder: render
decisions: 0168

## Change
`render/src/device.c` is 1347 lines and card 04 changes the card choice in it. This card moves code
and changes nothing the program does. Read device.c's header first; its paragraphs on the shader as
bytes, the two pipelines, the third pipeline, depth and validation go with the part that takes them.

`render/src/startup.h` — new, internal. Declares what `open_device` calls outside device.c, each
function with a `voe_render_` name: create the instance, choose the card, say which card, create the
two mesh pipelines. Its header says these are steps of startup that live beside device.c, that
`open_device` is still the one place their order is, and why that order is (the pipeline after the
descriptor layout, the element pipeline after these).

`render/src/instance.c` — new. `debug_message`, `has_layer`, `has_instance_extension`,
`wants_validation`, `attach_messenger`, `create_instance`, `VALIDATION_LAYER` and
`VALIDATION_IN_THIS_BUILD`, with their comments.

`render/src/card.c` — new. `graphics_family`, `choose_physical_device`, `say_which_card`, with their
comments. `REQUIRED_VERSION` is needed by both instance.c and card.c: it moves into startup.h.

`render/src/pipeline.c` — new. `draw_spv` and its `#embed`, `create_pipeline`, `create_pipelines`,
with their comments. The `#embed` needs no CMake edit: every source in the folder already carries
the `.spv` as an object dependency (`cmake/voe.cmake`).

`render/src/device.c` keeps the logical device, formats, timing, present modes, frame objects,
close-down and the two `_new`s, calling the moved functions through startup.h. Its header drops
what moved and says where each moved part now is. `device_internal.h` is not changed.

`render/src/src.md` gains the four entries; `device.c`'s entry is narrowed.

## Done when
`checks.sh --folder render` exits 0, `wc -l render/src/device.c` is under 800, and `voe_dev` still
prints the same `render` line at startup and draws.
