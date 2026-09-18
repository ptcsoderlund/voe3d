# 06 — dev: add_the_text returns the font it makes
folder: dev
decisions: 0168

## Change

`add_the_text` in `src/main.c` line 1423 breaks rule 6 (no `**`, ADR-0043) with its `voe_text_font **font`
out-parameter. The font is made inside the function and handed back, so hand it back as the return value:

```c
[[nodiscard]] static voe_text_font *add_the_text(voe_ecs_world *world,
						 voe_render_device *gpu,
						 voe_base_arena *arena,
						 voe_render_geometry quad,
						 voe_ecs_entity *hud,
						 voe_ecs_entity *panel,
						 voe_ecs_entity *readout,
						 voe_math_float2 *hud_size,
						 voe_base_error *error);
```

`NULL` for failure, with the distinguishable reasons still going into `*error` (rule 13). The other
out-parameters are single pointers and stay as they are. Update the one call at line 2275 to assign the result
to the existing `font` local and to test that for `NULL` where it tested the `bool`; every other use of `font`
in `main` is unchanged. Keep the three paragraphs of comment above the function, and extend the one that begins
*THE FONT IS MADE HERE AND NOT AT STARTUP* to say that this is why the font is what the call returns.

`static`, one file, nothing exported: no other folder is touched.

## Done when

`grep -nE '\*[[:space:]]*\*' dev/src/main.c` prints nothing outside comments, and
`cmake --preset debug && cmake --build --preset debug --target voe_dev` exits 0. If `slangc` is not found, it is
installed at `~/voe3d-scratch/tools/slang/bin/` — put that on PATH for the session; the tool is the programmer's
(ADR-0021), so do not teach `check.cmake` or the build to look for it.
