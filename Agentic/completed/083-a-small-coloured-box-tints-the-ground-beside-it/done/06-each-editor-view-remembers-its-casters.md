# 06 — Each editor view and the preview remember their casters
folder: editor
after: 05
decisions: 0168, 0394

## Change
0394 point 4: a removed caster marks where it stood only when the frame carries a
`voe_3d_bounce_casters` (header of `3d/include/3d/bounce_casters.h`), one per target. Give the editor's.

- `editor/src/view.h`: `voe_editor_view` gains a `voe_3d_bounce_casters` beside `target`;
  `voe_editor_views` gains one for the preview beside `preview_target`. The comments say whose bounce
  each feeds and that only 3d writes it. The views live in a local in `editor/src/main.c`; four views and
  the preview add about 100 KB to it, inside every platform's default main-thread stack. Say so in the
  `voe_editor_views` comment; do not touch `main.c`.
- `editor/src/view.c`: where a view's target and the preview's are made (`voe_render_target_create`),
  the memory is zeroed with it, so a remade target starts with nothing remembered. Nothing else changes.
- `editor/src/view_passes.c`: each `voe_3d_frame` built for a view's target sets `.casters` to that view's
  memory, the preview's frame (`frame.target = views->preview_target`) to the preview's. A frame that
  never reaches `voe_3d_draw_system_shadows` sets nothing. Header: one line that each target's frame
  carries its own memory (0394).
- `editor/src/src.md`: the `view.h` entry names the remembered casters.

A world loaded in place of another leaves old entities in the memory; they mark spheres once and are
forgotten, a recapture and no fault, so no clearing on load.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'casters' editor/src/view_passes.c` prints 2 or more.
