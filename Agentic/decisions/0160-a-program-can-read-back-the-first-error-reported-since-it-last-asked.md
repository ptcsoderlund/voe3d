# 0160 A program can read back the first error reported since it last asked

Status: accepted
Date: 2026-09-15

Spec 004 has the editor show, in its top bar, a notice naming the file, the line and what is wrong
when a project will not open. Every one of those details is already written — by
`assets`' sectioned reader, `authoring`'s scene reader and `platform`'s file calls — but only to
stderr through `base/report.h` (ADR-0140), and a returned `false` or error category cannot carry
"line 3: position holds 2 numbers, and a FLOAT3 holds 3". The editor's window has no terminal.

## Decision

**`base/report.h` keeps the first error-level report since the program last cleared it, per
thread, and hands its message back.** Two calls: one clears what is kept, one returns the kept
message — the `<message>` part as composed, without the level word, the module or the newline,
cut at the same capacity as the printed line — or NULL when no error has been reported since the
clear. Printing to stderr is unchanged. Warnings are not kept.

**The first and not the last**, because a failure travels upward and the site closest to the
cause reports first: a scene reader refusing a line is followed by nothing better from its
callers.

**Per thread (`thread_local`)**, so a report on one thread never lands in another's notice and no
lock is added to a path that must not fail.

A program clears before an operation whose failure it wants to show, and reads after the
operation returns its failure. Nothing below a program reads it.

This is a partial step 2 of ADR-0140: the line still goes to stderr, and a program can now also
see it. Where lines go in general — a console, a sink — is still D-248.

## Rejected

- A problem out-parameter on every reader (`assets` sectioned, `authoring` scene and project,
  `platform` files) — three public signatures and every call site change, and each new failing
  call would need the same parameter threaded through again to reach a notice.
- A destination the program installs — a function pointer outside `render`'s table, forbidden, and
  the threading question ADR-0140 declined to answer without a console to answer it for.
- Keeping the last report — the outermost caller's summary would replace the line that names the
  cause.

## Consequences

- The editor's notices carry exactly what stderr carries, and nothing below the editor changes to
  make that so.
- `base` gains its first per-thread state. It is one fixed buffer and a flag; `report.c` still
  never allocates.
- A program that forgets to clear reads a stale error from earlier; clearing is the reader's job
  and the header says so.
