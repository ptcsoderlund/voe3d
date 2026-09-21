# 24 — render.md's opening comes under the cap, and the whole tree passes
folder: render
decisions: 0168, 0210, 0211
read: feature.md

## Change
Comments and `.md` text only, by 0210's and 0211's methods. The test headers card 10 was for are
already under the cap and committed; what is left in `render` is `render/render.md`'s opening, 1205
characters against 400.

- `render/render.md` — the opening keeps what the folder is for: the GPU, the only folder that names
  Vulkan, and what it holds (device, pools, pipelines, frame) with nothing above them; 400 characters
  or fewer. Its entries do not change.
- What leaves it, each claim to one header, not repeated where that header already makes it:
  - not scenes, entities or files — `render/include/render/device.h` says it already;
  - no abstraction over Vulkan, one graphics API and never a second — `render/src/loader.c`'s header;
  - the only folder with shaders and slangc — `render/shaders/shaders.md`'s opening says it already;
  - the second paragraph (two mesh pipelines one draw per thing; the element pipeline, one instanced
    draw, no vertex buffer; a rectangle and a letter the same record, a glyph an element reading a
    distance-field sheet, no text system) — `render/src/element.c`'s header, which says most of it
    already; add what it lacks, at 57 lines or fewer as `checks.sh` counts them;
  - the third paragraph (drawn into engine-owned colour and depth images, one pair per frame slot,
    copied onto the window, nothing draws into a swapchain image, linear light, sRGB only as formats,
    no gamma constant in the folder) — `render/src/target.c`'s header, 55 lines or fewer; the
    linear-light and sRGB claims `device.h` already makes are not repeated.
- `render/include/render/render.md` — its opening sends the reader to `render/render.md` for "the
  fuller account"; point it at `device.h`'s header instead.

Read only the headers of the three code files. `render/vulkan` is never touched.

This is the feature's last card. A finding outside `render` is not fixed here; block the card and
name it.

## Done when
The coder: `bash ~/.claude/skills/checks/scripts/checks.sh --folder render` exits 0; 0210's token
check prints nothing; and `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints
`FINDINGS: 0` (steps 1–3 of `## How to test`: it runs `cmake -P check.cmake` and the whole ctest
suite).

The human: opens `voe_editor` on a saved level and sees it start, draw both views, and select, move
with the gizmo, undo and redo as before (step 4); opens `ui/include/ui/layout.h` and finds a short top
comment saying what the file is for, the reasoning above the functions it explains (step 5).
