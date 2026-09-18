# 04 — Every widget draws from the nearest theme
folder: ui
decisions: 0170, 0171, 0168

## Change

Done on 2026-09-17 under the earlier workflow as task 4 of spec 006; converted to a card by ADR-0168.

The context carries a theme — `voe_ui_theme_set(ui, const voe_ui_theme *)`, the caller's
memory, outliving the context as the font does — and a subtree carries its own through
`voe_ui_theme_push` / `voe_ui_theme_pop`; every node records the theme in force when it was made
and the widget pass reads the node's own, so the nearest wins (ADR-0170). Replace
`voe_ui_panel_begin`'s `voe_math_float4 colour` with a `voe_ui_surface` role — NONE (emits
nothing, as an alpha of nought does today), GROUND, SURFACE, RAISED. Every remaining colour
constant in `src/widgets.c` becomes a role: the button's three states with the pressed one on the
accent, the number box on the accent while it is dragged, the field, the caret, the scrollbar's
track and thumb, and a label's text. A panel and a button draw a hairline border in the border
role — the border rectangle with the fill inset, two element records, the hairline width a
constant of this folder. Add `voe_ui_text_role` (NORMAL, ACCENT) and
`voe_ui_label_role(ui, text, role)`; `voe_ui_label` stays and means NORMAL. Remove
`voe_ui_text_scale_set`: the theme's `text_size` is the one place a size is said. Move the
capacities that a second record per panel and button costs. Update the call sites this breaks:
`dev/src/interface.c` (its HUD plate becomes a RAISED panel and its text-size knob sets the size
on a theme of dev's own) and `editor/src/{dock,topbar,browser,inspector,interface}.c`. Tests:
nearest wins over the theme above, a pop restoring it, an unbalanced push refused the way this
folder already refuses a frame, a panel and a button emitting border and fill in the right order,
and a label in ACCENT coming out in the accent.

Note carried from the old plan: the coder stopped mid-check when the sponsor interrupted on 2026-09-17; both programs built and the `ui` tests had passed, but it never reported, and commit `e5297ca` then applied review fixes. Treat this card as done but unconfirmed: the suite run before "you can test" is its confirmation.

## Done when

`cmake --build --preset debug && ctest --test-dir build/debug -R '^ui/'` — all tests pass, and `voe_dev` and `voe_editor` still link.
