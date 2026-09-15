# 0012. A folder is a namespace; a module is a component plus its system

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0011 made the component-and-system pairing the unit of ownership but did not
say how that lands on disk. The engine's `CLAUDE.md` currently maps "module" onto
a single `foo.h` + `foo.c` pair, which is narrower than the ownership unit.

## Decision

**A folder is a namespace.** Folder and namespace stay in sync, as
`guidelines.md` already requires.

**A module is a component plus its system.** It may span several files, all in
the same namespace. Files of one module share the module's name.

**Suffixes carry the role:**

| Role | File |
|---|---|
| Component — the data | `<module>_component.h` / `.c` |
| System — the behaviour | `<module>_system.h` / `.c` |

So a `transform` module is `transform_component.h`, `transform_component.c`,
`transform_system.h`, `transform_system.c`, all in one namespace folder.

**Functional coherency decides what shares a name.** Files belong to the same
module because they serve the same function, not because they are the same kind
of thing.

## Consequences

- **Not all code is a component-system module.** Maths, platform, the ECS core
  itself and renderer internals are none of the three. The suffix convention
  governs **ECS modules only**; foundation and leaf folders are plain code named
  for their function, without suffixes. Applying `_component` to a `vec3` would
  be cargo cult.
- **A component with no system is suspect.** It is data with no owner, which
  ADR-0011 forbids by construction — someone is writing it from outside. A
  system with no component of its own is fine: it reads and it sends intent.
- **The naming is grep-able ownership.** `*_system.c` is the complete list of
  code permitted to mutate anything. That is a property a check script can use,
  and it matters because ADR-0011 is otherwise an unenforced convention.
- **Mild tension with `guidelines.md`, consciously accepted.** That document
  says everything is named for its function, not for what it is — and
  `_component` / `_system` name what a file *is*. The tension is confined to the
  suffix: the stem and the folder are both functional, and the suffix exists to
  distinguish the two halves of one module. Grouping — the rule's actual target
  — is unaffected.
- The engine's `CLAUDE.md` still maps "module" to a header+source pair and now
  contradicts this. Updating it stays in **D-021**, deferred by the principal
  along with the repository work.

## Questions this opens

None. Substantially resolves **D-012** for module-level naming; what makes a
header public remains open there.
