# 0340 — The whole tree builds in Release once per feature
date: 2026-10-03
by: tech-lead

## Decision
For 052 bug 04, and from then on for every feature: `check.cmake` gains a step that builds the whole
tree with the `release` preset, in a build folder of its own under `build/check/`. `check.cmake` is a
whole-suite command in `CLAUDE.md` `## Checks`, so this step runs in the suite (`checks.sh --all`) that
`/drive` runs once after a feature's last card, before it says `TEST`. No workflow script changes.
A Release break therefore never reaches the human's test, `main` or Ship. The per-folder check in
`CLAUDE.md` `## Checks` stays Debug-only, so a card is not built in Release. Bug 04 is planned as:
`check.cmake` is split first (it is 932 lines), then the Release step is added. Separately, `base` fixes
`VOE_BASE_DEBUG_ASSERT` so that under `NDEBUG` its expression stays compiled but is never run. That fix
removes the `-Wunneeded-internal-declaration` error on `is_finite` in `scene`, the only Release-only
fault in the tree today. The fix gets a test that stays in the tree.

## Reasoning
"Caught when the change is made, not the next time someone ships" is met at one Release build per
feature. The human tests a feature in one sitting, so a break found before that test is soon enough.
The only fault of this kind found today is the `base` assert, and fixing it at its root leaves little
that Debug can miss: a bare `assert()` or `#ifdef NDEBUG` code.
Alternatives: a Release build of the folder in every card's check (caught on the card that breaks it,
but a second, slower build tree in every coder checkout); only the `base` fix (the next such break is
again found by Ship).

## Replaces
Nothing.
