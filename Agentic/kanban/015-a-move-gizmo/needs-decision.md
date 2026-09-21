# Needs decision — the whole-suite check fails on debt older than this feature

Card 08's proof is `checks.sh --all`. It cannot pass on this feature's work alone.
`checks.sh --structure` today gives 44 findings. A few are 015's own, and a
3d card can clear them:

- `3d/include/3d/gizmo.h` and `3d/tests/gizmo.c` include `platform/window.h`,
  which 3d does not DEPENDS on (check.cmake). `voe_platform_size` already
  comes through `render/device.h`, as it does in `pick.h` and `outline.h`.
  The fix is to drop the include, not to add an edge.
- The `gizmo.h` header is 83 lines (the cap is 60). `3d/include/3d/3d.md` does
  not list `gizmo.h`. The `gizmo.c` entry in `3d/src/src.md` is 324 characters
  (the cap is 300).

The other ~40 findings are header comments over 60 lines and a few entries
over 300 characters. They are in 3d, app, assets, authoring, base, dev, ecs,
editor, platform, render, scene, text, theme and ui (for example
`ui/include/ui/layout.h` at 292 lines and `dev/src/main.c` at 555). The
workflow's caps were tightened after that code was written. Clearing them
touches about 38 files in 14 folders, many of them public headers, and has
nothing to do with a move gizmo.

## Question
How does 015 reach a green `checks.sh --all`?

## Options
1. **A separate cleanup feature, run first.** One card per folder moves the
   per-function reasoning from each over-cap header down above its function.
   015 plans only its own 3d card plus the walk and waits for that feature.
   The repo ends up clean. 015 is held up by roughly 14 cards of prose work.
2. **Raise the header cap for this product.** Pass `--header-cap` and
   `--entry-cap` through the product's configuration, or change the default
   in agentic_rules. The debt stays but is allowed. 015 needs only its own 3d
   card. This is the cheapest option, but the cap is there to keep what a
   coder reads small.
3. **015 is proved by its own folders.** Card 08's proof becomes
   `checks.sh --folder 3d` and `--folder editor` plus the whole ctest suite,
   and the debt is left for later. This breaks the rule that a feature's last
   card is proved by `--all`.

## Recommendation
Option 1. Its cards come from the findings list without judgement, and it
leaves the tree meeting the caps every later coder depends on. Once it is
chosen, the replan of 015 is two cards: one 3d card (drop the platform
include, get `gizmo.h` under the cap, list it in `include/3d/3d.md`, shorten
the `src.md` entry) and the walk as card 10.
