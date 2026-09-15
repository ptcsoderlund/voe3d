# 0053. The editor compiles shaders at runtime; the shipped game still embeds them

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0046 — narrows its rule to the shipped product rather than superseding it
- **Superseded by:** —

## Context

The principal wants a **visual shader editor**: a graph, stored as a text file
(JSON), from which materials are instantiated, carried through to the finished
product.

ADR-0046 compiles shaders with `slangc` at build time and embeds the SPIR-V with
`#embed`, and its consequences say flatly: *"No hot reload, and no runtime
shader authoring. Both are later decisions, and both start by superseding this."*

That wording is stronger than the decision it describes. ADR-0046's deciding
factor was that **the shipped product stays one file with nothing beside it** —
the property ADR-0040 had just bought. An editor is not a shipped product. Under
ADR-0052 the game is built by the editor, so every shader a game ships was
authored before its build ran and the embed is the right answer for the game.

What ADR-0046 genuinely blocks is the **preview**: the sphere in the graph
editor that shows the material as nodes are dragged. A preview that requires a
rebuild and a restart is not a visual shader editor. So one path is needed that
gets SPIR-V onto the GPU inside a running editor.

Constraints that survive: `slangc` is already on the installed-tools side of
ADR-0021, so the editor may assume it. ADR-0023's default answer to a
third-party dependency is no. ADR-0018 makes `render` the sole owner of GPU
resources, referenced by id, so whatever is added is added there.

## Options considered

### Option A — the editor runs `slangc` as a subprocess, loads the SPIR-V it wrote
The editor generates Slang from the graph, writes it to a temporary file, spawns
`slangc`, reads back the `.spv`, and hands the bytes to `render`.

Costs: a process spawn in `platform`. Shader errors arrive as compiler text on
stderr, not as something attachable to the node that produced them.

### Option B — link the Slang compiler into the editor only
Compile in-process through Slang's API. Structured diagnostics, so an error can
point at a graph node.

Costs: a large third-party dependency. It never enters a shipped game, only the
editor's link — but ADR-0023 makes adopting it the principal's decision.

### Option C — the preview requires a rebuild
No new mechanism at all.

Costs: it is then not a visual shader editor.

## Decision

**Option A.** The deciding factor is that it adds nothing that is not already a
required installed tool, and the seam it opens in `render` — *accept SPIR-V from
a caller instead of only from the embed* — is the same seam the "reload shaders
and assets without restarting" capability on the *later* list will need, so one
seam pays twice.

**ADR-0046 is narrowed, not superseded.** The rule becomes:

| Product | Where its SPIR-V comes from |
|---|---|
| Shipped game | Compiled during its build, embedded with `#embed`. Reads nothing from disk. |
| Editor | Its own shaders embedded the same way; **graph shaders compiled at runtime** and handed to `render` as a blob. |

**Two invariants, and they are the substance of this ADR:**

1. **One generator.** Graph JSON → Slang source is a single code path, used by
   the preview *and* by the cook. Two generators means the preview diverges from
   the shipped shader, and that defect is invisible until someone ships.
2. **One flag set.** The preview invokes `slangc` with the same three flags the
   build uses — `-target spirv`, `-matrix-layout-row-major` and
   `-fvk-use-entrypoint-name`. ADR-0035 records that the matrix layout default is
   wrong and **fails silently**; a preview compiled without it is a different
   shader that looks like a correct one.

**The graph JSON is the committed source of truth. The generated Slang is build
output**, treated like the generated `.spv` and the Wayland protocol sources —
never committed.

**Hand-written Slang stays first-class.** A shader may be written directly, not
only produced by a graph. ADR-0010's reason — that AI agents take part in
development — argues for both doors being open, and the graph is a convenience
over the language, not a replacement for it.

## Blast radius

**Cheap.** The editor calls one function to turn graph JSON into a pipeline;
whether that function spawns a process or links a compiler is behind it. Moving
to Option B later is a contained change in one place.

What is *not* cheap is violating invariant 1 — once preview and cook have
separate generators, every subsequent shader feature has to be written twice and
the divergence bugs are attributed to the GPU.

## Consequences

- **`render` gains a public entry point: build a pipeline from SPIR-V bytes the
  caller supplies.** Today the only source is the embed. This is a small
  addition to the surface D-035 will define, and it belongs in that decision
  rather than being smuggled in by an editor card.
- **`platform` gains process spawning**, since it is the only OS-aware folder
  (ADR-0022). First real caller is the editor; under ADR-0034 it is written then.
- **The editor requires `slangc` installed to preview a shader.** Free — the
  engine already requires it to build at all.
- **Shader iteration leaves the rebuild path.** This matters more than it
  sounds: ADR-0046 ties a folder's objects to its shader binaries, so editing a
  shader rebuilds more than it needs to. That is a direct tax on ADR-0055's
  budget, and the preview path avoids paying it during authoring.
- **A shader graph is an asset, so it is `assets`' business to parse** — and it
  is JSON, which ADR-0054 places there.
- **Material instances need a representation** that names a graph and carries
  parameter values. Not decided here.

## Rejected options and why

- **Option B** — the better tool, and the likely successor. Node-level
  diagnostics are the thing a graph editor lives or dies on, and stderr text is
  a poor substitute. It was rejected *for now* on ADR-0023: it is a large
  dependency, and the cheaper option has to be shown insufficient before the
  principal is asked to accept one. The trigger to revisit is explicit — when
  mapping compiler errors back to graph nodes becomes the editor's main
  complaint.
- **Option C** — rejected on the requirement. It does not deliver the thing.

## Questions this opens

- **Where the graph → Slang generator lives.** It must be reachable by both the
  editor and the cook step. `assets` is the obvious candidate since it already
  owns the parse, but code generation is not reading-into-CPU-data and may want
  its own home. Needs deciding when the first shader-graph card is written.
- **How a material instance is represented**, and how its parameters reach the
  shader — a constant buffer, a bindless index, or per-variant specialisation.
- **Shader variants.** "Emission as an option, not a rule" (D-055) implies a
  material declares its packing and the shader branches or is specialised.
  ADR-0052's cook step is the natural place to enumerate the variants actually
  used, because it is the only point where the full set of shipped materials is
  known. Not built until asked (ADR-0034), but the door is now identified.
