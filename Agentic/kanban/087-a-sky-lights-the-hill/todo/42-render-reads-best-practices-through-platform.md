# 42 — Render reads Best Practices' variable through platform
folder: render/src
after: 41
decisions: 0168, 0413

## Change
Bug 05: `render/src/instance.c` calls `getenv("VOE_RENDER_BEST_PRACTICES")` (about line 284), which the
Windows headers deprecate. Read it with `voe_platform_environment_read` from
`platform/include/platform/environment.h` (card 41) on the `arena` the function already has, and include
that header. The answer passed to `voe_render_best_practices_wanted` is unchanged in meaning (NULL when
unset). If `instance.c` includes `<stdlib.h>` only for `getenv`, drop it. Header comment: no change unless
it names `getenv`. No index entry changes.

## Done when
- `grep -nE '\bgetenv[[:space:]]*\(' render/src/instance.c` prints nothing.
