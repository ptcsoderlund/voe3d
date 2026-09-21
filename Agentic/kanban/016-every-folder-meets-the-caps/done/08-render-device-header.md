# 08 — device.h's header comes under the cap
folder: render
decisions: 0168, 0210

## Change
Comments and `.md` entries only, by 0210's method. Bring each header below to 55 lines or fewer as `checks.sh` counts them:

- `render/include/render/device.h` — header 178 lines today.

Read each header, then each declaration or definition a paragraph moves above. The file is 1394 lines; find each declaration a paragraph explains by `grep -n` for its name and read only around it. `render/vulkan` is never touched. Then the entries for these files in the `.md` pages under `render/` that list them (`grep -rn --include='*.md' <file name> render/`): change one only if it points at the header for text that moved.

## Done when
`bash ~/.claude/skills/checks/scripts/checks.sh --folder render` prints exactly 5 FINDING lines, the header caps of `render/src/frame.c`, `render/src/geometry.c`, `render/src/target.c`, `render/tests/elements.c`, `render/tests/offscreen.c`, untouched by this card, and none of its own; its product check (build and the folder's tests) passes; and 0210's token check prints nothing.
