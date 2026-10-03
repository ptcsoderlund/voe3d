# Needs decision — bug 04: how is a Release break caught when the change is made?

## Question
Bug 04 expects two things: Ship builds again, and "a change that breaks the Release build is caught
when the change is made, not the next time someone ships". The first is a one-card fix. The second
changes what every card or feature is checked with, so it reaches past 052 and is not the planner's
to choose. Which check builds Release, and how often?

## What the planner found
- The fault is in `base`, not `scene`. Under `NDEBUG`, `VOE_BASE_DEBUG_ASSERT`
  (`base/include/base/assert.h`) wraps its expression in `sizeof`. Its header says that makes
  "anything used only inside a debug assert still count as used". Clang does not agree for a
  `static` function: a reference that is never evaluated gives `-Wunneeded-internal-declaration`, and
  `-Werror` makes that an error. `is_finite` in `scene/src/transform_component.c` is the only such
  function today.
- Fix, ready to plan: one card in `base` that keeps the expression compiled but never run (for
  example inside `if (0)`), with a test that stays in the tree. No caller changes.
- A whole-tree Release build with only this warning downgraded builds all 592 steps. Nothing else in
  the tree breaks Release today.
- Today no check builds Release. The folder check in `CLAUDE.md` `## Checks` builds `debug` only, and
  so does `check.cmake`. The `release` preset is there but nothing runs it.

## Options
1. **The folder check also builds the folder's library in Release.** Add
   `cmake --preset release && cmake --build --preset release --target voe_$top` to the first
   `## Checks` line. This catches the break on the card that makes it. It costs a second build tree
   in every coder checkout, and the first build in each checkout compiles the folder's dependencies
   at `-O3`.
2. **`check.cmake` gains a step that builds the whole tree in Release** (its own build folder under
   `build/check/`). `drive.sh` runs it once after a feature's last card, and the secretary runs it
   before a merge, so a break never reaches `main` or Ship. It is caught per feature, not per card.
   `check.cmake` is 932 lines, so it gets a split card first.
3. **Only the `base` fix, no new check.** This is the cheapest. The next break of this kind is again
   found by Ship.

## Recommendation
Option 2, together with the `base` fix. It meets "not the next time someone ships" at one Release
build per feature instead of one per card. The `base` fix removes the only Release-only fault found.
What Debug can still miss is a bare `assert()` or `#ifdef NDEBUG` code, which is rare. If a per-card
catch is worth the extra build per checkout, choose option 1.
