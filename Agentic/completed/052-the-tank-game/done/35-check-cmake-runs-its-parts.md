# 35 — check.cmake runs its parts
folder: .
after: 32, 33, 34
decisions: 0168, 0340, 0341

## Change
`check.cmake` keeps only what is its own and includes the parts cards 33 and 34 wrote in `check/`.
The per-folder check passes for `folder: .` without building (0341); the suite run after this
card is what runs the result. Read `check.cmake`, `check/check.md` and `ONBOARDING.md` lines 35–45.

- `check.cmake`, top to bottom: its header comment (lines 1–34), then lines 36–41 (the settings),
  then `include("${root}/check/report.cmake")`, then lines 86–87 (the fresh `build/check/`), then
  one `include("${root}/check/<part>.cmake")` each for `tools`, `folders`, `build`, `guards`,
  `includes`, `tests`, `analyser`, in that order, then the closing block (lines 921–932). Every
  other line goes; the parts already hold them. The header gains the point that the steps live in
  `check/`, one file per step group, run in this file's scope in the order included, and that the
  whole tree is also built in Release (0340). The closing block's "first of twenty-four lines" is
  stale (a run prints a line per folder); it says "first of a run's lines" or the like, no count.
- `ONBOARDING.md`: the sentence before `cmake -P check.cmake` also names the Release build of the
  whole tree among what it verifies.

## Done when
`grep -cE '^include\("\$\{root\}/check/[a-z]+\.cmake"\)$' check.cmake` prints 8,
`grep -oE 'check/[a-z]+\.cmake' check.cmake | xargs ls` exits 0,
`! grep -qE '^(function|foreach)\(' check.cmake` exits 0, and `[ $(wc -l < check.cmake) -lt 110 ]`
exits 0.

The human's, after `/drive` reports the suite passed:
1. Open `examples/tank_game` in the editor and press Ship. No error shows, `Build/ship/` holds the
   game, and the shipped program runs and shows its menu.

## Blocked
The change is made and `## Done when` passes (8 includes, every part exists, no function or foreach, 75 lines), but `checks.sh --folder .` reports 22 findings, all on `README.md` (one heading only; does not list the subfolders), and they are there on the clean tree before this card too. Clearing them means rewriting the project README into a table of contents, which this card does not ask for and which is the human's call. Unblock by deciding whether the root's `README.md` is held to the folder-index rule (then a card of its own rewrites it) or `checks.sh` exempts the root, then move this card to `done/` as it stands.
