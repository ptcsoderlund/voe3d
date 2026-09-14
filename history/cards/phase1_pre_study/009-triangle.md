# 009 — a triangle

status: review  
claimed-by: claude-opus-5 (kanban-coder)  
blocked-by: -

Draw a triangle in the dev window. It is the first shader in the engine and the
first time `slangc` is ever run.

**Shaders are Slang, compiled by `slangc` at build time, embedded in the binary
with `#embed`** (ADR-0046). Nothing is read from disk at runtime. The `.spv` is
generated into the build tree and never committed, the same as the Wayland
protocol sources already are.

**The Clang floor moves 18 → 19** (ADR-0047) — `#embed` needs it. Three things
move together: the version guard in `cmake/voe.cmake`, **its message text**, and
the guard test in `check.cmake` that matches that text by substring, including
the faked version it uses, which must now be below 19. A guard changed without
its test still passes, for the wrong reason. `CLAUDE.md`'s *Givens* says 18 too.

**Keep it minimal.** Positions in the vertex shader, no vertex buffer, no
matrices, no textures, no render-to-texture, no `3d` folder. Draw into the same
swapchain image the cleared frame already gets. The deliverable is that the
shader toolchain works end to end — not that the renderer grew.

**`render` only.** Do not generalise the `slangc` invocation for other folders;
nothing else has a shader, and where that call should live is an open question
that needs a second folder to answer (D-052).

The row-major matrix test ADR-0035 asks for is **not** this card — nothing here
sends a matrix to the GPU. It arrives with the first one that does.

Report the `slangc` version you used.

## What was done

**The shader**

- `render/shaders/triangle.slang` — one file, two entry points, three vertices
  and three colours as constants. No vertex buffer; the draw is
  `vkCmdDraw(3, 1, 0, 0)` and the index picks the vertex.

**The build**

- `cmake/voe.cmake` — `voe_render_shaders()`, a second folder-specific function
  beside `voe_platform_backend()` and deliberately not a general mechanism.
  Globs `render/shaders/*.slang`, runs `slangc` per file into
  `${CMAKE_BINARY_DIR}/generated/render`, nothing committed.
- The `.spv` reaches the compiler as `--embed-dir`, **not** as an include
  directory: `#embed` has its own search path and does not read `-I`. That cost
  a build failure that reads as a zero-length array rather than a missing flag,
  which is why the function's header now says so in capitals.
- `OBJECT_DEPENDS` on the folder's sources ties the object to the `.spv`, which
  is what both orders the build and rebuilds after a shader edit. `#embed` is
  invisible to CMake's dependency scanning, so nothing else would.
- Three `slangc` flags, none of them a preference: `-target spirv`,
  `-matrix-layout-row-major` (CLAUDE.md's rule, wrong default, silent failure),
  `-fvk-use-entrypoint-name` (the C side names entry points).

**The Clang floor, 18 → 19** — four places, not three:

- `cmake/voe.cmake` — the guard, and its message text, now `Clang 19 or newer
  (ADR-0005, ADR-0047)`. The header comment listing the substrings `check.cmake`
  matches moved with it.
- `check.cmake` step 4b — matcher moved to `Clang 19 or newer`, and **the faked
  version moved 17 → 18**. 17 fires the guard at either floor, so it would have
  kept passing for the wrong reason; 18 against a floor of 19 fails the moment
  someone lowers the guard back. There is now a comment saying that, because the
  number is the whole value of the step.
- `check.cmake` step 1 — the tools check had its own `LESS 18`. The card named
  three places; this was a fourth.
- `CLAUDE.md` — both *Givens* rows, plus the Shaders row, which now says where a
  compiled shader ends up.

**The pipeline** — in `device.c`, because that file is everything with a startup
lifetime and `device_internal.h` says the split is by lifetime, not by subject.
A pipeline no resize touches has a startup lifetime, so no new file.

- Viewport and scissor are dynamic state, so a resize rebuilds the swapchain and
  nothing else.
- `VkPipelineRenderingCreateInfo` carries the colour format — dynamic rendering,
  no render pass, same as the cleared frame used.
- `shaderDrawParameters` is enabled on the device. Slang lowers `SV_VertexID` to
  `gl_VertexIndex - gl_BaseVertex`, which makes the module declare the
  `DrawParameters` capability, which is invalid without the feature. There is no
  slangc flag to avoid it and the alternative is a vertex buffer this card
  forbids. All three devices on this machine report it, llvmpipe included.
- Ten loader entries added: shader module, pipeline layout, graphics pipeline,
  bind, set viewport, set scissor, draw, and the three destroys.

**The Y flip** — `frame.c` now sets the viewport, so this card is where
CLAUDE.md's "exactly one Y flip and it is in the viewport" first becomes real:
`y = height`, `height = -height`. Depth range is the plain 0..1; the reversed
depth is the projection matrix's job and there is no depth buffer here. The
front-face constant is `VK_FRONT_FACE_CLOCKWISE`, which is what a Y-up
counter-clockwise winding becomes in framebuffer space after the flip.

## Verified

Linux only. Windows is untouched by anything platform-specific here but is
unverified, as ever.

- `cmake -P check.cmake` — **all 13 steps ok**, including `guard version` with
  the new faked 18, `tests (2 passed)` and `harness reports a failure`.
- A faked clang **19** configures successfully, which proves the floor is
  exactly 19 and not something higher.
- `render` and `dev` each **configure and build standalone**, and `slangc` runs
  in the standalone tree too. `ctest` in the standalone `render` tree: 2 passed.
- The 1496 bytes of `triangle.spv` were found **byte-for-byte inside**
  `libvoe_render.a` and inside the `voe_dev` binary — the embed is real, not a
  path that happens to compile.
- Touching the shader reruns `slangc` and recompiles `device.c`. The dependency
  is not decorative.
- `clang --analyze` over every `render/src` and `dev/src` file: no findings.
  (Note: `check.cmake` has no analyser step — see the report below.)
- **`voe_dev` was run and screenshotted.** The triangle draws: red apex up,
  green bottom left, blue bottom right, smoothly interpolated, on the blue-green
  clear. Apex up confirms the Y flip goes the right way; green on the left
  confirms −X is left. Validation layers were active
  (`/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json`) and
  **stderr stayed empty** across several runs — no validation error from the
  pipeline, the shader module, the viewport or the draw.

**Not verified, and it is yours:** resizing and minimising. `wmctrl` only sees
XWayland windows, so the native Wayland window cannot be driven from here. The
things to look at are in `dev/src/main.c`'s header.

Tool versions used: **`slangc` 2026.13.1-1-g84792eb15**, clang 22.1.8,
CMake 4.3.0, Ninja 1.13.2.

## Markers

- `DEVIATION: CLAUDE.md, "the front-face constant is set to match and proven by
  a test"` — `render/src/device.c`. Set to match; not proven. With
  `VK_CULL_MODE_NONE` the constant has no observable effect, so there is nothing
  to assert on, and this card says keep it minimal. Marked at the site.
- No `BLOCKED:`.

## Notes

Three things found on the way, none of them done, all of them yours to judge:

1. **`CLAUDE.md` rule 8 describes a `check.cmake` step 7 (`clang --analyze`)
   that does not exist.** The script ends at step 6b. The rule reads as though an
   analyser warning fails the run; nothing runs the analyser. I ran it by hand
   over the touched folders and it is clean, but that is not the same as the
   script enforcing it. Reported, not fixed — it is a card, and it is not this
   one.
2. **The winding test CLAUDE.md asks for wants a card.** It needs culling turned
   on to be observable at all. That card is also where a triangle that vanishes
   tells you the flip is wrong.
3. **`dev/` was touched, and it is a second folder.** No logic changed: the
   window title still said "a cleared frame", and the header comment — which is
   the list of things a person is asked to try — still said the window is a flat
   colour and that a triangle comes later. The card's deliverable is "a triangle
   in the dev window", so leaving that text would have handed you wrong
   instructions for verifying this very card. `dev/dev.md` followed. Say if you
   would rather that had been a separate card.
