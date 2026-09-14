# 0127. A struct describes its fields, and the description lives in `base`

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** in part, by ADR-0154 — point 1: an element count becomes a shape of up to seven dimensions

## Context

**Closes D-239.** ADR-0122 settled that a struct and its description are written once
together; ADR-0123 settled the vocabulary. Neither said where the description *type* lives,
and the first card cannot be written without it.

The tech lead's first recommendation was `ecs`, with `ecs` gaining a dependency on `math`
so a field could declare itself a vector. **The principal's challenge is what corrected
it**: *"Isn't that supposed to be in the editor module? Devs not using editor won't use
description mechanisms either?"*

Working through it produced the fact the first recommendation had missed: **a description
never names a type.** It records a field's name, a *kind* — an enum value — an offset and a
size. `offsetof` and `sizeof` are evaluated in the folder that owns the struct, where its
types are already in scope. So the folder holding the description type needs no dependency
on `math` and none on `ecs`, and the thing being described is not a component at all: it is
any struct.

Fixed by earlier decisions: ADR-0022's DAG and `base`'s charter — *"the primitives any
other folder may depend on"*. ADR-0122: the editor build and the shipped build must not
disagree about a struct's shape. ADR-0123's vocabulary.

## Options considered

### Option A — `ecs` owns it, and gains a dependency on `math`
A field declares itself `VEC3` and `ecs` includes `math` to say what that is.

### Option B — `base` owns it, and the module map does not change
The kind is an enum; the declaring folder supplies the C type and the offset. `base` learns
nothing about `math` or `ecs`.

### Option C — the editor owns it
The principal's proposal: descriptions live with the tool that consumes them.

## Decision

**Option B.** `base/include/base/describe.h`. **No change to the module map.**

1. **A description is a name, a kind, an offset, a size and an element count.** The kind is
   an enumeration covering ADR-0123's vocabulary. `VOE_BASE_FIELD_FLOAT3` and
   `VOE_BASE_FIELD_ENTITY` are enum values, not types — `base` includes neither `math` nor
   `ecs` and gains no dependency.
2. **The declaring folder supplies the C type.** A field is declared as *(type, name,
   kind)*, so `base` never maps a kind onto a type. The redundancy is load-bearing: `base`
   knows each kind's expected size and asserts it against `sizeof` of the declared type at
   compile time, so a field declared `FLOAT3` that is actually a `float2` fails the build.
3. **The struct is always generated; the table is conditional.** One list produces both.
   Where descriptions are not wanted the table is not compiled and **the struct is
   byte-for-byte identical**, which is ADR-0122's rule satisfied rather than bent.
4. **It describes structs, not components.** Nothing in it mentions an entity or a
   component table. `ecs` is one caller among possible others.

## Blast radius

**Cheap.** It is a header and a table with no behaviour. Moving it to another folder later
is a rename. Reversibility: **cheap.**

## Consequences

- **A component header reads as a field list rather than a plain struct**, and that cost is
  paid by every reader whether or not they ever open the editor. It is the whole price of
  ADR-0122 and it is now visible on the page.
- **A developer who never uses the editor pays nothing else.** The table compiles out and
  the struct is unchanged.
- **`base` grows a concept that is not memory, containers, strings or assert.** It fits the
  charter — a primitive any folder may depend on — but the folder's own summary sentence
  needs widening, and that is a real edit rather than an oversight.
- **Point 2's compile-time size check is the only thing standing between a mistyped kind
  and a silently wrong description.** It is worth more than it costs.
- **Nothing here says who reads a description at runtime.** D-238 owns that and it is what
  decides whether descriptions ship at all.

## Rejected options and why

**A — `ecs` gains `math`.** Rejected because the dependency was never needed: it existed
only to let `base`-level code name `math`'s types, which point 2 removes. Recorded because
it was the tech lead's recommendation and was one word from being written up.

**C — the editor owns it.** Rejected, and the principal's instinct behind it is honoured by
points 3 and 4 instead. The declaration cannot move to the editor because ADR-0122's
mechanism produces the struct and the description from one list — they cannot be in two
folders — and a hand-written copy in the editor is exactly the drift ADR-0122 rejected:
add a field to the struct, and the editor's copy silently omits it from every saved scene.
What *can* be excluded is the table, and it is.

## Questions this opens

None. D-238 was already open and is unaffected.
