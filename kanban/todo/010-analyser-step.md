# 010 — the analyser step the rulebook already promises

status: todo
claimed-by: -
blocked-by: -

`CLAUDE.md` rule 8 says `check.cmake` runs `clang --analyze` and fails on any
warning. **It doesn't.** The script ends at step 6b. Decided by ADR-0042 and never
built. The triangle card found it, ran the analyser by hand, and it was clean —
which is not the same as the script enforcing it.

## Goal

`check.cmake` runs the analyser over every folder's `src/` and `tests/`, zero
warnings, no baseline. A finding fails the script.

## What is already decided

- **Zero warnings, no baseline** (ADR-0042). Not `clang-tidy` — that is a separate
  install and would be a fifth required tool.
- **A false positive is suppressed at the site with a reason**, never globally, and
  never by changing correct code to quiet it.
- The step is numbered per the script's current ordering; the triangle card's run
  reported 13 steps, so pick the next number and say what it is.

## What this card decides

The register carries these as open, and they are yours to settle against real
code rather than in the abstract:

- **The exact invocation and checker set.** Start with the default checkers and no
  extra `-Xanalyzer` enables. Add one only if you can show a real finding in this
  tree that the default set misses, and say so if you do.
- **How a per-site suppression is spelled.** Whatever you choose, it must be
  greppable, and the reason must be next to it.

## Verify

- A deliberately leaky file fails the step; removing it passes. **Prove the step
  fires** — a check step that cannot fail is the failure mode card 002 existed to
  prevent.
- Clean over the whole tree as it stands today.

## Known cost, already accepted

This compiles the tree a second time and roughly doubles a check. The principal
accepted that explicitly: there is no CI, so this script is the whole safety net.
Do not add a fast path or a changed-folders-only mode — that was considered and
rejected.
