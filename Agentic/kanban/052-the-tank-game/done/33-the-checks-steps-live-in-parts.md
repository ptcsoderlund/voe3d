# 33 — The check's steps live in parts
folder: check
after: none
decisions: 0168, 0340, 0341

## Change
`check.cmake` (932 lines, at the root) is split by step group before the Release step is added
(0340). The parts go in a new folder `check/` (0341); this card writes them, and card 35 makes
`check.cmake` include them. `check.cmake` itself does not change on this card, so the parts are
unused until then.

The folder `check/` does not exist yet. Read `check.cmake` and nothing else. Create:

| part | `check.cmake` lines | what it is |
|---|---|---|
| `check/report.cmake` | 43–84 | `step_ok`, `step_skip`, `step_warn`, `step_fail`, `run_capture` |
| `check/tools.cmake` | 89–216 | step 1, the tools and the slangc floor |
| `check/folders.cmake` | 218–320 | the folder list and step 1b |
| `check/build.cmake` | 322–346 | step 2 standalone, step 3 root build |
| `check/guards.cmake` | 348–404 | steps 4a–4c |
| `check/includes.cmake` | 405–466 | step 5 |
| `check/tests.cmake` | 467–674 | steps 6, 6b, 6c |
| `check/analyser.cmake` | 675–920 | steps 7, 7b |

- Each part is a header comment, one blank line, then its lines copied from `check.cmake`
  unchanged. The header is one comment block with no blank line in it. Its points: which steps
  it holds; that it is included by `check.cmake` in order and runs in that file's scope; the
  variables it reads from earlier parts or `check.cmake` (`root`, `checkdir`, `common`, `folders`,
  the `report.cmake` functions, as the case is) and those it leaves for later ones (for example
  `folders`, and `slangc_warning`, which the closing block reads).
- New `check/check.md`: what the folder is (the steps of `check.cmake`, one file per step group,
  no build of its own, not a code folder), and one entry per part.

Lines 36–41, 86–87 and 921–932 stay in `check.cmake` only.

## Done when
`for f in report tools folders build guards includes tests analyser; do sed '1,/^$/d' check/$f.cmake; done | diff -B - <(sed -n '43,84p;89,920p' check.cmake)`
exits 0, and `test -f check/check.md` exits 0.
