# Needs decision — where `## Seeing what was drawn` lives

Card `blocked/10` is done in `editor`: `checks.sh --folder editor` and `cmake -P check.cmake` pass. Only
`checks.sh --all` fails, on `CLAUDE.md` line 16: `## Seeing what was drawn` is not one of the headings the
workflow allows (Checks, Exempt, Never touch). That is not something a card in this feature can fix. `CLAUDE.md`
applies to the whole repo, and the heading check belongs to the workflow's tooling (`agentic_rules`). No card is
written until it is settled. Once it is, card 10 needs no code change: move it back to `review/` after
`checks.sh --all` exits 0.

## Question
The capture instructions (ADR-0157: `voe_editor --capture`, `voe_app_new_headless`, `voe_app_capture_png`) are
useful to every coder. Where should they live so that `CLAUDE.md` passes the workflow's check?

## Options
1. **Move the text into decision 0168**, the engine's standing rules, which is named on every card. Remove the
   heading from `CLAUDE.md`. No change to the tooling. The instructions still reach every coder through the
   card's `decisions:` line.
2. **Allow the heading in the workflow**. Edit the heading check in `~/Projekt/agentic_rules`, then run
   `install.sh`. This could be a named extra heading or a project-defined list. The check stays loose for every
   project that uses the workflow.
3. **Put it under `## Checks`** as a non-command note. This is the smallest edit, but it mixes a how-to into a
   list that `checks.sh` runs as commands, so it may break the runner.

## Recommendation
Option 1. It keeps `CLAUDE.md` in the shape the workflow expects, puts nothing in the tooling that is specific to
one project, and 0168 already gathers the rules that every card carries. It is a tech-lead edit to `CLAUDE.md`
and to 0168 (or a new decision that 0168 points at).
