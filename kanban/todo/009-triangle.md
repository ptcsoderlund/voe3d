# 009 — a triangle

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
