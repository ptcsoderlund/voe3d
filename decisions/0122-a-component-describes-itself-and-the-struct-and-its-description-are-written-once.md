# 0122. A component describes itself, and the struct and its description are written once

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-024**, open since the module map was drawn: *what type description does a
component carry so it can be written and read back as text?* The row had been marked
blocked behind D-002 ever since, and ADR-0022 closed D-002 and said so in words —
*"Closes D-002 … Unblocks D-010, D-012 and D-024"* — so the block had been dead for
about ninety-eight decisions when this was found on 2026-09-11.

It is the keystone under the editor the principal described the same day. Four separate
things read one description: **the inspector draws it, the serializer writes it, the
loader reads it back, and a bridge in another language binds against it.**

The constraints already fixed. ADR-0007 and ADR-0011: anything editable must be component
data, because *"state hidden in a system's private variables is invisible to the editor"*,
and text serialisation of components was made a requirement on the ECS itself by ADR-0009
**while the editor was still years away** — this ADR is that requirement being collected.
ADR-0010: v1 has no scene file and the text scene format arrives with the editor.
ADR-0057: the editor is a C program on the engine and another language is an optional
bridge on the public C API, not a privileged substrate. ADR-0008: no runtime code loading.
The engine's promise: a programmer installs Clang, CMake and Slang, and the build fetches
everything else — a new build-time toolchain dependency is expensive against it.
ADR-0121: the shipped game is built without the editor.

## Options considered

### Option A — a hand-written description beside each component
Whoever writes the component writes a table next to it: field name, type, `offsetof`.
Plain C, nothing to learn, obvious on the page.

### Option B — the struct and its description written once, together
The fields are written once as a list; the struct and the description are both produced
from it. Offsets come from `offsetof` and sizes from `sizeof`, so both are the compiler's
answer rather than anyone's.

### Option C — a tool that parses the headers and generates the descriptions
Unreal's and O3DE's answer: a build step that reads C and emits descriptors.

### Option D — annotation comments the editor reads out of the source
Raised by the principal: `//[Expose]` above a field, and the editor reads the code files
and works the description out from them. Keeps the header a completely plain struct,
which is the one real cost of B, and reads like C# attributes.

## Decision

**Option B.** The principal's call, and the deciding factor is the failure mode rather
than the elegance: **A's way of going wrong is silent and it lands in the user's saved
scene file.** Add a field, forget the table, and that field is invisible in the editor and
absent from every scene saved from then on — data quietly missing from the one artifact
the user cannot rebuild. B makes that impossible by construction, because there is only
one list and both the struct and the description come out of it.

Two rules come with it.

1. **The description sits beside the struct, never inside it.** The principal's
   `#if VOE_EDITOR` instinct — that the person writing the component writes how it is
   edited, right there — is adopted. What is **not** adopted is putting the switch around
   the component's own fields: a struct that is a different shape in the editor than in
   the shipped game means the thing that was tuned is not the thing that ships, and
   produces bugs that exist only in the finished product. The struct is byte-for-byte
   identical in both builds; the editor build additionally compiles the description.
2. **The description is the language boundary.** Anything that can call the public C API
   can register a description and appear in the editor. This is what makes ADR-0057's
   optional-bridge model real rather than nominal.

## Blast radius

**Cheap for the mechanism, load-bearing for the fact.** Moving between A, B and C later is
mechanical and touches no saved file. What is expensive is the existence of the
description at all — the inspector, the serializer, the scene format and every bridge are
written against it, and removing it would mean writing all four by hand per component.
That cost was already accepted by ADR-0009.

## Consequences

- **The header for a component reads as a field list rather than a plain struct.** This is
  the real price and it is paid on every component, for ever. It is about twenty lines of
  one-time machinery and then invisible, but a reader meets it on their first day.
- **Adding a field to a component makes it appear in the editor and in the save file with
  no editor code written.** That is the whole payoff and it is what the principal asked
  for by *"heavy serializers which can keep things organized and visually updated"*.
- **Unblocks D-097**, the authored format's grammar, which ADR-0073 could not settle first
  precisely because the sigils in the sketch exist only while a struct cannot describe
  itself. With this, the sigils go.
- **The engine gains a concept it did not have**: a vocabulary of field types. Which types
  a description can express is now a question and it is on the critical path (D-232).
- **A silent-drift class of bug is closed before it exists.** It is worth recording that
  this was the deciding argument, because the cost of B is visible on every header and its
  benefit is invisible by design.

## Rejected options and why

**A — hand-written descriptions.** Rejected on the failure mode above. A compile-time
check comparing described bytes against `sizeof` catches much of the drift but not all of
it, because struct padding makes the sum inexact — so the check cannot be made to fail
reliably on the case that matters.

**C — a header-parsing generator.** Rejected on the principal's own objection to O3DE:
*"making stuff too complex (as C++ usually does)."* It needs a new build step, a large
third-party C front-end behaving identically on Windows and Linux, and a macro layer that
becomes its own thing to learn. It is also aimed at a problem we do not have — that
machinery exists to describe C++ class hierarchies with inheritance, templates and hidden
state. A component here is a flat struct of scalars, which is a smaller problem and not
the same one attempted with less courage.

**D — annotation comments read out of the source.** Rejected on two grounds, the first
of which the principal raised himself.

- **It breaks every other language, and it breaks them at the worst place.** A description
  living in C source comments means a C# module wanting its own components must either
  emit C for the editor to parse, or the editor needs a second parser for C#, and a third
  for the next language. Under B the description arrives through the C API and the
  boundary holds for everything. This alone is decisive given ADR-0057.
- **It quietly requires a C compiler front-end, which is option C without option C's
  correctness.** A field's *offset* is the thing the editor actually needs, and knowing it
  means resolving typedefs, nested structs, arrays, enums and — the killer — the packing
  and alignment rules of two compilers on two platforms. Under B the compiler answers with
  `offsetof` and is right by construction; under D we reimplement an ABI, and being wrong
  by one byte produces a cooked scene of garbage with nothing to point at.

**What is kept from D.** The goal is right: a plain-looking header is worth something, and
writing the list by hand may become a chore. If it does, an **offline** tool that reads
annotations and writes the B-style list, whose output is committed and readable, is a
legitimate later convenience — it keeps the compiler as the source of truth and adds no
runtime parser. Parked as D-233 rather than dismissed.

## Questions this opens

- **D-232** — the vocabulary of field types a description can express, and which folder
  owns it. On the critical path: the first component description cannot be written without
  it. Scalars and vectors are obvious; strings, arrays, enums and **a reference to another
  entity** are the ones that decide how far the scene format can go.
- **D-233** — whether an offline tool later writes the field lists from annotations, and
  what would make that worth its machinery.
