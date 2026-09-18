# 0032. Memory is explicit arenas, passed as parameters

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Tech Lead (delegated by Human)
- **Supersedes:** —
- **Superseded by:** —

## Context

D-009, and the first Phase 4 convention to come due. `base` is one half of the
first build wave (ADR-0022) and cannot be written without knowing where memory
comes from: its containers, strings and arena are the first code in the project
that allocates anything. `math` does not need this — it is pure value types
(ADR-0014) — so this ADR gates the `base` card, not the `math` one.

Constraints already fixed, and what each eliminates:

- **No `**` (ADR-0007, reaffirmed by the principal).** This kills the ubiquitous
  C signature `int alloc(size_t n, void **out)`. An allocating function must
  **return** the pointer. Convenient, because ADR-0014 already says exactly that.
- **`base` depends on nothing (ADR-0022).** It cannot call `platform`, so it
  cannot reach `VirtualAlloc` or `mmap`. And ADR-0028 step 5 forbids OS headers
  outside `platform/`. **Arenas in `base` are therefore backed by `malloc`, not
  by virtual-memory reservation** — which rules out the reserve-huge/commit-as-
  needed design used by most modern arena implementations.
- **ADR-0014 names allocation.** `new` hands memory to the caller, who calls
  `destroy`; `init`/`deinit` is a module's own. A function with none of those
  words does not allocate.
- **ADR-0025.** Rendering is written for speed; loading only has to work. A
  single memory strategy is not required to serve both well.
- **`guidelines.md`.** Data-driven first; interfaces and function-pointer
  dispatch are explicitly the second choice.
- **D-008 is undecided.** This ADR must not decide error handling by the back
  door — but it must say what happens when an allocation fails, because that is
  a memory question that would otherwise force D-008's hand.

## Options considered

### Option A — One global allocator
`voe_base_alloc(size)` / `voe_base_free(ptr)` over `malloc`, called from
anywhere. Nothing to pass, nothing to design.

Costs: no lifetime grouping, so every allocation is freed individually and every
one is a leak an agent can forget. Nothing tells a reader how long a pointer
lives. Per-frame scratch memory becomes malloc traffic in the hot path, which is
the one place ADR-0025 says performance is a requirement.

### Option B — Explicit arenas, passed as a parameter
A `voe_base_arena` is a bump allocator over `malloc`-backed blocks, chained when
it runs out. Memory is not freed individually; the arena is rewound or destroyed
as a whole. Any function that needs scratch or scoped memory takes the arena it
should use as a parameter. Long-lived, individually-owned or growable data still
uses `new`/`destroy` over a thin `malloc` wrapper.

Costs: an extra parameter on functions that allocate, and a lifetime that must
be reasoned about once per arena rather than never. Chained-block arenas cannot
grow an existing allocation in place, so growable arrays do not live in them.

Makes easy: per-frame scratch that costs a pointer bump and a reset; asset
loading that frees everything on one line whatever the parser did; and — the
reason it matters here — code in which forgetting to free is not a thing that
can happen. The lifetime of every pointer is visible in the signature that
produced it.

### Option C — An allocator handle with function pointers
A `voe_base_allocator` struct carrying `alloc`/`free` pointers, passed
everywhere, so arena, malloc, pool or tracking allocators are interchangeable.
Zig's `std.mem.Allocator` is the reference.

Costs: an indirect call on every allocation, the function-pointer dispatch idiom
`guidelines.md` ranks second, and — decisively — it is an interface with one
implementation on the day it is written.

## Decision

**Option B.** Deciding factor: it is the only option that makes the lifetime of
a pointer visible at the call site, and this codebase is written by agents in
small isolated cards where a leak is invisible until much later.

Fixed by this ADR:

1. **`voe_base_arena`** — a bump allocator over `malloc`-backed blocks, chained
   when full. In `base`, using `<stdlib.h>` only, so `base` stays
   dependency-free and OS-free.
   - `voe_base_arena_new(size_t block_size)` / `voe_base_arena_destroy(arena)`
   - `voe_base_arena_push(arena, size)` — returns 16-byte-aligned, zeroed
     memory. Never freed individually.
   - `voe_base_arena_mark(arena)` / `voe_base_arena_rewind(arena, mark)` for a
     nested temporary scope; `voe_base_arena_clear(arena)` to rewind everything
     and keep the blocks.
2. **The arena is a parameter, never a global.** No default arena, no implicit
   scratch, no thread-local. A function that allocates from an arena names it in
   its signature, which is what makes the lifetime readable without opening the
   body.
3. **`push` does not violate ADR-0014's naming rule.** That rule is about
   *ownership obligation* — "does not allocate" means "does not hand you memory
   you must free". Arena memory carries no such obligation; the arena owns it
   and the arena's own lifetime is already named by `new`/`destroy`. The arena
   parameter is the disclosure. This is a clarification of ADR-0014, not an
   amendment.
4. **Long-lived, individually-owned or growable data does not use an arena.**
   ECS component arrays grow by reallocation and outlive every scope; a chained
   arena cannot resize a block in place. These use `voe_base_alloc`,
   `voe_base_realloc` and `voe_base_free` — a thin, deliberately boring wrapper
   over the C library, existing so there is one chokepoint to instrument later.
   Callers reach them through the owning module's `new`/`destroy`, per ADR-0014.
5. **Failure to allocate is fatal.** `push` and `voe_base_alloc` do not return
   `NULL`; on failure they abort through `base`'s assert/fatal path with the
   size and the call site. Rationale: on a desktop 3D engine, failing to obtain
   a few kilobytes means the process is already dead, and the alternative puts
   an error return on nearly every function in the engine and decides D-008 by
   accident. The exception is a large, externally-sized allocation — a 2 GB
   asset — which is a checked, explicit API and belongs to D-008.
6. **Arenas are not thread-safe.** One arena per thread or per job. No locking,
   no atomics. Revisited if and when there is a job system.
7. **No allocator interface.** Rejected as premature generality. The trigger to
   revisit is a second real allocator — most likely a debug tracking allocator —
   and it can be introduced behind `voe_base_alloc` without touching a call site.

## Blast radius

**Reversibility: moderate, and asymmetric.**

Going from B to A or C later is a signature change on every function that takes
an arena — mechanical, wide, and the kind of edit that is tedious rather than
dangerous. Going the other way, retrofitting arenas onto code written against a
global allocator, means re-deciding the lifetime of every allocation that
already exists, which is not mechanical at all. Taking B now is the cheap
direction of a decision that is expensive in only one direction.

Point 5 is the load-bearing part and the one worth overturning early if it is
going to be overturned: making allocation failure recoverable after the fact
means revisiting every allocating signature in the engine.

## Consequences

- **Two mechanisms, not one.** Arenas for scoped and temporary; `new`/`destroy`
  for long-lived and growable. This is more than the minimum, and it is
  justified because both already have real call sites: per-frame render command
  building and asset parsing on one side, ECS component arrays and GPU resource
  objects on the other. It is not two ways to do the same thing — a growable
  array cannot live in a chained arena at all.
- **`malloc`-backed, not virtual-memory-backed.** A direct consequence of `base`
  having no dependencies. It costs a chained block instead of one contiguous
  reservation, which means arena memory is not guaranteed contiguous and
  pointer arithmetic across two pushes is invalid. Acceptable. If a contiguous
  reservation is ever needed, it comes from `platform` and is a new decision.
- **Every allocating function grows a parameter.** Accepted deliberately: that
  parameter is the documentation.
- **Who owns which arena is not decided here.** A permanent arena, a per-frame
  arena reset each frame, and a per-load arena are the obvious three, and they
  are created by `app` and handed down. That is a wave-two question — D-038.
- **`base`'s assert/fatal path is now on the critical path** and must exist
  before the arena does. It is small, it belongs to the same card, and it is
  the first thing D-008 will build on.
- **Debug builds have no allocation tracking yet.** Deliberate. The chokepoint
  exists; filling it is a later card driven by a real problem.

## Rejected options and why

- **Option A** — the one thing this codebase cannot afford is a lifetime that is
  invisible in the code, because the author of any given function is an agent
  that will not see the free.
- **Option C** — an interface with one implementation, using the dispatch idiom
  `guidelines.md` ranks second, paying an indirect call at every allocation for
  flexibility nothing has asked for. It is the exact anti-pattern this project
  has been avoiding, and nothing is lost by deferring it: point 4's wrapper is
  the seam.
- **Virtual-memory reserve/commit arenas** — the better arena design, and
  unavailable: it needs OS calls, `base` may not make them, and moving the arena
  into `platform` would put `base` beneath `platform` and invert the module map.
- **Returning `NULL` on failure** — see point 5. Rejected for now, and named in
  *Blast radius* as the part to challenge early rather than late.

## Questions this opens

- **Closes D-009.** Exit criterion 5 advances; error handling (D-008),
  polymorphism (D-010) and public headers (D-012) remain.
- **D-008 is narrowed, not decided.** Allocation failure is off its plate.
  What remains is genuine, recoverable failure: a file that will not open, a
  glTF that will not parse, a device that will not create.
- **D-038, new:** which arenas exist, who creates them and when they are reset.
  Belongs to the `app` card, not to `base`.
- The `base` card must implement the assert/fatal path before the arena.
