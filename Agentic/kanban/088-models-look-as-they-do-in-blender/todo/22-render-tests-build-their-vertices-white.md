# 22 — Render's tests build their vertices white again
folder: render/tests
after: none
decisions: 0168, 0421, 0422

## Change
Card 07 gave `voe_render_vertex` a fourth field, `colour_complement` (`uint16_t[4]`, zeroed is white),
and committed `vertex_colours.c` and the shaders; its own test passes, but every test below fails to
compile: clang's `-Wmissing-field-initializers` rejects a positional `{ {pos}, {normal}, {uv} }`.
0422: each such initializer gains a trailing `{ 0 }`, nothing else. A designated initializer or a
vertex filled field by field is left alone. No header or `tests.md` entry changes; no test changes
what it checks.

Files, all under `render/tests/`: `best_practices.c`, `blocked_bounce.c`, `blocked_light.c`,
`blocker_kinds_bounce.c`, `blocker_kinds.c`, `bounce_capture.c`, `bounce_probes_scene.c`,
`bounce_read.c`, `bounce_settle.c`, `bounce_shadow.c`, `bounce_suns.c`, `depth_copy.c`,
`directional_lights.c`, `elements.c`, `exposure.c`, `haze.c`, `mips.c`, `offscreen.c`, `passes.c`,
`pass_times.c`, `pixels_write.c`, `point_lights.c`, `point_shadows.c`, `pools.c`, `prepare.c`,
`shading_write.c`, `shadow.c`, `shadow_lights.c`, `sky.c`, `sky_light.c`, `surface_maps.c`,
`targets.c`, `terrain_layers.c`, `terrain_vertex.c`, `textures.c`, `transient.c`, `unshaded.c`,
`water.c`, `worker_guard.c`.

Find the initializers by building (`ninja -C build/debug -k 0`, read each `missing field
'colour_complement'` error); clang stops at 20 errors per file, so build again until none is left.

## Done when
`ninja -C build/debug -k 0 2>&1 | grep -c "render/tests/.*colour_complement"` prints 0, and the
tests `render/vertex_colours` and `render/surface_maps` pass.
