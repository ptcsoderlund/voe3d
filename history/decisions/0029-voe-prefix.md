# 0029. The prefix is `voe_`, everywhere: functions, types, targets, projects

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0014 (prefix clause only), ADR-0027 (target and function names)
- **Superseded by:** —

## Context

ADR-0014 fixed `v_` as the prefix on every function and type, explicitly
rejecting `voe_`, and `voe3d/coding_convention.md` says the same. ADR-0027
followed it for CMake targets (`v_<folder>`) and used `voe3d_` for the build
function and alias. ADR-0030 introduces folders named after Bevy's crates, and
the principal's direction with it is one prefix, used the same way for code and
for projects: `voe_render`, `voe_3d`, `voe_text`, `voe_sprite`, `voe_ui`.

No code exists. The change costs nothing today and would cost a sweep of every
identifier later.

## Decision

**`voe_` is the prefix, and the only one.** Applied identically at every level:

| Thing | Name |
|---|---|
| Function | `voe_math_vec3_add`, `voe_render_pipeline_new` |
| Type | `voe_math_vec3` |
| CMake target | `voe_<folder>` — `voe_render`, `voe_3d` |
| CMake alias | `voe::<folder>` |
| CMake project | `voe_<folder>` |
| Build function | `voe_module()`, `voe_executable()` in `cmake/voe.cmake` |
| Check-script hook | `VOE_CHECK_FAKE_CLANG_VERSION` |

Everything else in ADR-0014 stands unchanged: the namespace is spelled out in
order, `new`/`destroy` pairs, a function without `new` does not allocate, value
types need no system.

The folder `3d` yields identifiers `voe_3d_...`, which are legal C — the
prefix carries the leading letter.

**Deciding factor:** one prefix that reads the same in a call site, a link line
and a folder listing. The saving `v_` bought — two characters per identifier —
was never the point of ADR-0014; the spelled-out namespace was.

## Blast radius

**Reversibility: free now, a full-tree rename later.** Which is why it is taken
now, before the first card.

## Consequences

- **`voe3d/coding_convention.md` line 8 is now wrong** — "All functions are
  prefixed `v_`. Not `voe_`." It is the binding file (ADR-0003) and the
  principal's; it needs the one-word change before the first card is written,
  or the coder will follow the file, correctly, and produce `v_` code.
- ADR-0014 and ADR-0027 are not rewritten. Their status lines point here.
- The engine repository keeps its name, `voe3d`. The prefix is `voe_`; the
  product is VOE3D.

## Rejected options and why

- **Keep `v_`** — rejected by the principal, who set it. It was chosen for
  brevity; the folder names of ADR-0030 make the longer prefix the consistent
  one.
- **`voe3d_`** — rejected; the digit mid-identifier reads badly and
  `voe3d_3d_` would be absurd.

## Questions this opens

None. **D-021** (rules into the engine's `CLAUDE.md`) carries the prefix.
