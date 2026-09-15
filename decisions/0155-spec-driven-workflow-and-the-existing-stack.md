# 0155 The spec-driven workflow, and the stack as it stands

Status: accepted
Date: 2026-09-14

The engine was built under a kanban workflow: a tech lead in a separate planning repository
decided, cards arrived in `kanban/todo/`, coders moved them to `review/`, the human moved them to
`complete/`. Work now goes through the spec-driven framework, whose guidelines skill and
`lang-c.md` replace this repository's own `guidelines.md` and `coding_convention.md`. The stack
itself does not change, and it has to be recorded once in the new format so a planner can point
at it.

## Decision

**Work goes spec → plan → tasks, in `specs/NNN-slug/`.** The sponsor approves `spec.md`; a planner
writes `plan.md` and `tasks.md`; a coder implements one task in one folder; `cmake -P check.cmake`
exiting zero is what done means (ADR-0004, ADR-0028, ADR-0042). New project-wide decisions are
records in `decisions/`, numbered from 0156.

**`kanban/` is retired.** No card, bug inbox, claim field or review folder is used again. The rules
of ADR-0108, ADR-0109, ADR-0110, ADR-0113's card wording, ADR-0114, ADR-0115, ADR-0116 and
ADR-0143, which governed cards, bug reports, the ceilings on hot documents and the secretary role,
no longer bind work here. ADR-0113's substance survives, restated for tasks in `CLAUDE.md`. Those
records keep their accepted status because `0001`–`0154` are not edited; this record is what
retires them.

**`history/` keeps the finished cards and archived bug reports, unchanged**, because code and
records cite them as "card NNN" and "bug NNN". Nothing is added to it and nothing in it is edited.
`decisions/0001`–`0154` stay as written, in their older format.

**The framework's guidelines apply, with the project's stricter rules in `CLAUDE.md`.**
`guidelines.md` and `coding_convention.md` are removed; every rule they held is either stated by
the framework or carried into `CLAUDE.md`. Two engine rules deliberately stand over the framework:
a task owns the call sites its surface change breaks downstream (ADR-0113), and Linux alone
verifies a task (ADR-0130).

**The stack, as settled by the records that decided it:**

- Language and repository shape — C23, one repository of self-contained CMake folders, statically
  linked: ADR-0001, ADR-0008.
- Compiler — the GNU-driver Clang only, floor 19: ADR-0005, ADR-0026, ADR-0047.
- Build — CMake 3.28+ and Ninja, a four-line file per folder over `voe_module()`: ADR-0027; the
  compile database beside each folder: ADR-0044; LF everywhere: ADR-0048.
- Verification — no CI, `check.cmake` in script mode, tests as plain C under CTest, the analyser
  as step 7: ADR-0004, ADR-0028, ADR-0031, ADR-0042, ADR-0045; a skip names what it skipped:
  ADR-0106; below the shader compiler's floor it warns: ADR-0117.
- Tools and dependencies — tools installed, dependencies fetched, write it ourselves:
  ADR-0021, ADR-0023.
- Graphics — Vulkan 1.3, resources by id, no SDK with headers vendored and the loader opened at
  run time: ADR-0018, ADR-0040; `render` as the GPU layer under renderers: ADR-0030, ADR-0060;
  an offscreen frame, two frames in flight, passes and targets: ADR-0024, ADR-0050, ADR-0051,
  ADR-0148.
- Shaders — Slang, compiled at build time and embedded, the compiler's version part of the
  binary: ADR-0015, ADR-0046, ADR-0112.
- Platforms — Windows and Linux desktop, Wayland on Linux, Linux first: ADR-0037, ADR-0130.
- Architecture — the scene is an ECS, a component and its system are one module, intent is a
  datatype, no system calls a system: ADR-0007, ADR-0011, ADR-0012, ADR-0013, ADR-0017; the folder
  map: ADR-0022, extended by ADR-0121 and ADR-0151.
- Code conventions — naming and allocation, the `voe_` prefix, arenas, on-demand implementation,
  returned failure: ADR-0014, ADR-0029, ADR-0032, ADR-0034, ADR-0041.
- World conventions — coordinates, row-major matrices, reversed depth, Slang spelling in `math`:
  ADR-0033, ADR-0035.
- Programs — `dev`, `editor`, and the game as the three builds: ADR-0038, ADR-0052, ADR-0121,
  ADR-0135.

## Rejected

- Keep `kanban/` beside `specs/` — two places to look for what is being built, and the card rules
  assume a tech lead and a planning root this repository no longer has.
- Move the old cards into `specs/` — they are not specs, and a rename would break every "card NNN"
  citation's meaning without breaking a build to warn anyone.
- Rewrite `decisions/0001`–`0154` into the new record format — records are never edited, and the
  content is unchanged by the format.
- Keep `guidelines.md` and `coding_convention.md` as project copies — two authorities for the same
  rule drift apart; the framework's copy wins and the project's differences live in `CLAUDE.md`.

## Consequences

- Code comments that say "card NNN" or "belongs in a card" stay as they are; a card named in code
  is a file in `history/cards/`, and "a card" in a comment now reads as "a task".
- A folder `.md` written to ADR-0120 and ADR-0133 lists files in `include/`, `src/` and `tests/`
  by path; the framework's table-of-contents audit reports those as out of shape until they are
  brought to its form.
- `CLAUDE.md` exceeds the framework's usual length because it carries the engine rules whole.
