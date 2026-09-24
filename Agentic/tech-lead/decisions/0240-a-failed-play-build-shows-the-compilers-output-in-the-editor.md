# 0240 — A failed Play build shows the compiler's output in the editor
date: 2026-09-24
by: tech-lead

## Decision
From milestone 3 on, when Play's build fails, the editor shows the compiler's output so the
developer can read the errors without a terminal, and starts nothing (0187). The one line on
stderr of 0237 point 6 stays as well. How and where the editor shows it is the planner's.

## Reasoning
With the project's own C in the build, a failed build is the developer's own mistake and they must
see it. Rejected: stderr only (0237), which hides errors from anyone who started the editor
without a terminal.

## Replaces
The "no notice" of 0237 point 6.
