# 0014. Naming carries the namespace; allocation is visible in the name

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Amended by:** ADR-0029 — the prefix is `voe_`, not `v_`. All other clauses stand.

## Decision

Directed by the principal and written into `voe3d/coding_convention.md`, which
is the binding location per ADR-0003 — this ADR is the rationale, not the rule.

**Prefix.** Every function is prefixed `v_`. Not `voe_`.

**The namespace is spelled out.** A folder is a namespace (ADR-0012) and nested
folders spell out in order: `v_<namespace>_<subnamespace>_<name>`, as in
`v_math_vec3_add`. Types follow the same rule — `v_math_vec3` is a type,
`v_math_vec3_add` a function on it. C has no namespaces; this is the substitute,
and it is only useful if it is applied without exception.

**Compact pure data needs no system.** `v_math_vec3` is data and the functions
on it, nothing more. This narrows ADR-0012, which said a component without a
system is suspect: that holds for **entity state that something owns and mutates
over time**, and does not apply to **value types**. A `vec3` has no owner because
it has no identity.

**Allocation is visible in the name.** A function that allocates internally has
`new` in its name and a matching `destroy` beside it — `v_render_pipeline_new` /
`v_render_pipeline_destroy`.

## Consequences

- **The useful direction of the allocation rule is the inverse:** a function
  without `new` in its name does not allocate. That makes ownership readable at
  the call site without opening the implementation, which is the whole point in
  a language with no destructors and no RAII.
- The `new`/`destroy` pairing is a naming rule, not an allocator strategy.
  **D-009 stays open** — global allocator, arenas or passed-in handles is a
  separate question, and this convention holds under any of them.
- Names get long. Accepted deliberately: `v_render_pipeline_new` is longer than
  `pipeline_new` and unambiguous in a way it is not. The alternative is what C
  projects normally do, which is collide and then abbreviate inconsistently.
- **Greppable, and therefore checkable.** `v_.*_new` finds every allocation site
  in the engine, and every one missing a `destroy` beside it is a defect a check
  script can find. This matters because ADR-0004 leaves local scripts as the
  only enforcement there is.
- Substantially completes **D-012**; what makes a header public remains open,
  though ADR-0013's two-header split largely answers it.

## Questions this opens

None.
