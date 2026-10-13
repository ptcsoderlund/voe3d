# 0422 — A positional vertex names its zeroed colour
date: 2026-10-13
by: planner

## Decision
For 088, after card 07 was blocked. `-Wmissing-field-initializers` stays on with `-Wextra -Werror`.
Every positional `voe_render_vertex` initializer, `{ {pos}, {normal}, {uv} }`, gains a trailing `{ 0 }`
for `colour_complement`: the same white vertex as before, said where it is built. Designated
initializers and vertices filled field by field need nothing (clang 22 accepts both as they are). One
card per folder that builds vertices so: `render/tests`, `text`, `3d`, `sprite`, `dev`.

## Reasoning
- Turning the warning off is a change to `cmake/voe.cmake` for every folder, and the warning catches a
  field forgotten in any other struct; that is not 088's to give up.
- A trailing `{ 0 }` is the smallest edit that compiles and keeps 0421's zeroed-is-white meaning; a
  rewrite to designated initializers is a larger diff for the same result.

## Replaces
Nothing. Amends 0421 point 1: builders outside the model store keep their meaning but not their text.
