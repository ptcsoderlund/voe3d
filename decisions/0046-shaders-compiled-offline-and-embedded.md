# 0046. Shaders are compiled by `slangc` at build time and embedded in the binary

- **Status:** Accepted
- **Date:** 2026-08-31
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-007a.** ADR-0015 chose Slang as the shader language and ADR-0019
(superseded by ADR-0021) said the toolchain is managed separately. Neither said
*when* a shader is compiled or *how* the result reaches the GPU, and the question
has been open since.

It is now blocking. `render` clears a frame and can draw nothing, because
drawing needs a pipeline and a pipeline needs SPIR-V. **There is no shader in
this engine at all** — no `.slang` file, no `.spv`, and `check.cmake` verifies
`slangc` exists (step 1) and then never invokes it. Every roadmap item —
`3d`, `text`, `sprite`, `ui` — waits behind this one thing.

### One half is not a choice

**`slangc` is a tool the programmer installs, not something the engine ships**
(ADR-0021, and it is in `CLAUDE.md`'s *Givens*). An end user who plays a game
built on VOE3D has no `slangc` and never will. So compiling a shader at runtime
would require linking a Slang compiler into the engine — a large third-party
dependency that ADR-0023 rejects by default and ADR-0040's no-SDK reasoning
rejects in spirit. **Compilation happens at build time.** That was never really
open.

### The half that is a choice

What the build *produces*, and how `render` gets hold of it: a file on disk
beside the executable, or bytes inside the executable.

The relevant standing constraints: authoring data is text and shipping data is
binary (ADR-0010); nothing renders to the screen directly, so shaders will
eventually exist for offscreen passes too (ADR-0024); recoverable failure is a
returned value and every failure is a category someone has to handle (ADR-0041);
and shader hot-reload is on the *later* list in `STATUS.md`, not in v1.

## Options considered

### Option A — `.spv` files on disk, loaded at runtime
`slangc` writes `.spv` into the build directory; the executable finds them at
run time and reads them.

Costs: an asset directory and a convention for where it is relative to the
executable, which differs between a build tree, an installed tree and a shipped
game. A new failure category — shader missing, shader unreadable, shader from a
different build — each of which needs an ADR-0041 error code and a person to
handle it. It makes shader hot-reload nearly free later, which is a real
benefit for a *later* feature.

### Option B — embed the compiled SPIR-V in the binary
`slangc` writes `.spv` at build time; C23's `#embed` turns it into a byte array
at compile time. The shader is part of the program.

Costs: **`#embed` is a C23 feature Clang did not implement until 19**, so this
raises the compiler floor from ADR-0005's 18. Rebuilding is required to change a
shader, which forecloses nothing today and makes hot-reload a larger job later.

### Option C — both, selected at build time
Embedded for shipping, on-disk for development, so hot-reload is available while
developing.

Costs: two paths through shader loading, from the first day, on a renderer that
cannot yet draw a triangle.

## Decision

**Option B.** `slangc` runs at build time and its SPIR-V output is embedded into
the binary with `#embed`.

The deciding factor: **it deletes an entire failure category rather than
handling one.** A shader that is part of the executable cannot be missing, cannot
be stale relative to the code that binds it, and cannot be found at a path that
was right in the build tree and wrong in the shipped game. This is the same
instinct as ADR-0040 — the reason no end user needs a Vulkan SDK is that there
is nothing for them to fail to have.

**The floor rises to Clang 19.** That is an amendment to ADR-0005 and is
recorded separately as **ADR-0047**, because it binds every contributor and must
be findable without reading a shader decision.

**Option C is rejected as premature, not as wrong.** When shader hot-reload is
built, it will be an ADR that adds a development-only path — with the embedded
build as the shipping default and one path, not two, being the thing that
ships. Nothing here forecloses it: an embedded byte array and a file read
produce the same `const void *, size_t` at the point `render` creates a module.

## Blast radius

**Reversibility: moderate, and asymmetric.**

Cheap to reverse: the mechanism. Swapping `#embed` for a file read changes how
one buffer is obtained and nothing about pipelines, Slang, or the shaders
themselves. The seam is deliberately a pointer and a length.

Not cheap to reverse: **the Clang floor.** Once shaders are embedded, every
contributor needs 19, and dropping back to 18 means undoing the mechanism
everywhere it is used. This is why ADR-0047 exists as its own record.

The expensive direction, avoided: a runtime Slang compiler. That would be a
large dependency, an end-user install, and a permanent contradiction of
ADR-0021's line.

## Consequences

- **Changing a shader requires a rebuild.** Accepted. Incremental builds make
  this seconds at v1 scale, and hot-reload is explicitly *later*.
- **`slangc` finally gets invoked**, which means ADR-0035's
  `-matrix-layout-row-major` gets exercised for the first time. That ADR
  requires the layout be *proven by a test*, not trusted to the flag — the first
  card that compiles a shader owes that test.
- **The build gains a generated-file step**, and it is the second one:
  `platform` already generates Wayland protocol sources (ADR-0037). The same
  shape applies — nothing generated is committed.
- **The `.spv` must exist before the `.c` that embeds it compiles.** This is a
  build-ordering dependency, not a link-time one, and it is the one thing most
  likely to be got subtly wrong first time.
- **Which folder invokes `slangc` is not settled here.** ADR-0035 says the
  matrix layout is *"owned by the folder that invokes `slangc`"*; today that will
  be `render`. Whether it stays there when `3d`, `text` and `sprite` all have
  shaders is a question for the card that first has two folders wanting one.
- **Shader compilation errors become build errors**, which is the right place
  for them and consistent with `-Werror` everywhere else.
- **No new required tool.** `slangc` was already in *Givens*; this is the first
  thing that actually uses it, which means the tools list stops containing an
  entry nothing needs.

## Rejected options and why

- **On-disk `.spv` (A)** — rejected on failure surface, not on capability. It is
  the better answer the day hot-reload is built and the worse one until then,
  because it converts a compile-time certainty into three runtime error paths
  that each need an ADR-0041 code and a person to handle them.
- **Both paths (C)** — rejected as premature generality, the anti-pattern this
  pre-study names explicitly. Two shader-loading paths before one triangle
  exists is a seam maintained for a consumer that does not exist (ADR-0034).
- **Runtime Slang compilation** — rejected on ADR-0021's line. `slangc` is
  installed by programmers; requiring it at runtime would put a compiler on an
  end user's machine, which the onboarding invariant and ADR-0040's reasoning
  both refuse.

## Questions this opens

- **D-052 (new)** — where the `slangc` invocation lives once more than one folder
  has shaders. `voe_module()` is four lines per folder by ADR-0027 and a shader
  list is a fifth thing; whether that is an argument, a convention
  (`<folder>/shaders/*.slang`, globbed like `src/` and `tests/`), or a separate
  function is a real question. **The convention is the obvious guess and should
  not be adopted without a second folder to test it against.** Blocked on the
  first card where two folders both compile shaders.
- **D-032 is now live.** The `slangc` version floor was left half-open; the first
  card to invoke it should report the version it used, so the floor is set from
  evidence rather than guessed.
