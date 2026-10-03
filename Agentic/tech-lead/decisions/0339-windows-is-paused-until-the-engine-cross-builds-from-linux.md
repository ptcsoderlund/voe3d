# 0339 — Windows is paused until the engine cross-builds from Linux
date: 2026-10-03
by: tech-lead

## Decision
Linux desktop is the only platform the engine builds for, is tested on and ships to, from now on. The Windows
code already in the tree stays where it is, but nobody builds, tests or fixes it. No card writes new Windows
code, and a new `platform` feature gets a Linux backend only. Windows code may fall behind the rest of the tree.
No work order, bug report or `## How to test` asks for anything on Windows, and a feature that names Windows
drops that part. Ship builds and installs the Linux program only. Windows comes back as its own feature, when
the engine cross-builds for Windows from Linux with Clang. That feature brings the paused code up to date and
proves it.

## Reasoning
Cross-building lets Linux build and check the Windows program, so Windows comes back cheaply and in one piece.
Testing it by hand on a second machine until then costs a sitting per feature. Writing Windows code that
nothing builds is effort spent on code that is likely wrong. Alternatives: keep writing Windows code untested
(it costs time and still rots); delete the Windows code (it throws away work that cross-building will want).

## Replaces
Amends 0168 (Givens: "Platforms Windows and Linux desktop only"; Scope of a card: "Windows code is still
written and `platform` keeps both backends as equals") and ADR-0130 while it holds. 0243, 0245, 0248 and the
Windows halves of 0227, 0233, 0265 and 0292 are paused, not undone.
