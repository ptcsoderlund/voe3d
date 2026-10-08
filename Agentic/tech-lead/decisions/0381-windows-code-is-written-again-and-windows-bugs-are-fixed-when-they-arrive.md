# 0381 — Windows code is written again, and Windows bugs are fixed when they arrive
date: 2026-10-08
by: tech-lead

## Decision
Windows and Linux desktop are again the engine's two platforms. Every card writes for both: a `platform` feature
gets a Windows backend beside the Linux one, as equals, and no code may leave Windows unable to build. Linux is
still the only platform that is built, checked and tested in a controlled way: `check.cmake` green on Linux
finishes a card, and no work order's `## How to test` names Windows. The sponsor uses Windows now and then,
without notice. What they find there arrives as a bug report at any time and is fixed when it arrives, before
new work. A Windows bug is filed against the feature in `kanban/`, whichever feature caused it, so it never
waits for the board to empty. When `kanban/` is empty, it goes against the feature that caused it, moved back
from `completed/`. Ship builds and installs on both platforms again. The Windows code that fell behind under
0339 is brought up to date as its bugs are found. The first is bug 02 of 082: the editor does not link.

## Reasoning
The sponsor has no Windows machine they control, so Windows cannot be tested to a plan. They still work on one
often enough that a stale Windows build costs them a sitting. Writing both halves keeps Windows close, and bug
reports catch what writing alone misses. Alternatives: keep the pause until a cross-build exists (Windows stays
broken for an unknown time); controlled Windows testing (impossible without a machine to test on); patching
only today's break (the next feature breaks it again).

## Replaces
0339. Restores 0168's Givens ("Platforms Windows and Linux desktop only") and its Scope of a card ("Windows code
is still written and `platform` keeps both backends as equals; what the other platform turns up is reported, not
reopened"), and ADR-0130. Adds that such reports are fixed when they arrive. 0243, 0245, 0248 and the Windows
halves of 0227, 0233, 0265 and 0292 hold again. 0378's home trash is the Recycle Bin on Windows.
