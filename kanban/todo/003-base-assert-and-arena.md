# 003 — base: assert and arena

claimed-by:
status: todo

**Do not start this before card 002 is in `review/` or `complete/`.** It writes
tests, and the mechanism for writing them is that card.

## Goal

The two things in `base` that the next folder will need: a fatal-assert path,
and the arena allocator.

Done means `cmake -P check.cmake` exits zero and `ctest -R base` runs and passes
the new tests.

## Scope

**Write only what is listed here.** No dynamic array, no string type, no hash
map, no allocation tracking, no logging. Nothing in this engine stores a
variable number of things yet, so nothing needs a container yet.

If implementing what is below turns out to require one, stop and say so on this
card. That is a real finding and the right thing to report — it is not a licence
to write the container.

## Constraints

- `base` depends on nothing. The C standard library only.
- **No OS headers.** `base` is not `platform`, and the check greps for them.
- Every name is `voe_base_*`.
- No `**`. An allocating function returns the pointer; it does not fill one in
  through an out-parameter.

## Assert

First, because the arena depends on it.

- A check that survives into release builds, and one that is compiled out.
  Decide which name says which, and put the reasoning in the file header.
- On failure: the expression, the file, the line and a message, on stderr, then
  abort.
- **This is not error handling.** It is for conditions that mean the program is
  already wrong. How a recoverable failure gets reported — a file that will not
  open, a model that will not parse — is not decided yet. Do not invent an
  answer here, and do not build anything that presumes one.

## Arena

Working memory in this engine comes from an arena that is passed in as a
parameter. Memory is not freed one allocation at a time; the arena is rewound or
destroyed as a whole. There is no global arena and no default one.

```
voe_base_arena_new(block_size)      -> arena, caller owns it
voe_base_arena_destroy(arena)
voe_base_arena_push(arena, size)    -> 16-byte aligned, zeroed
voe_base_arena_mark(arena)          -> mark
voe_base_arena_rewind(arena, mark)
voe_base_arena_clear(arena)
```

- Backed by `malloc`, in blocks, chained when a block fills. Not one large
  reservation: reserving address space needs the OS, and `base` may not call it.
- `push` memory is never freed individually. There is no `voe_base_arena_free`.
- **Failing to allocate is fatal**, through the assert path above. `push` does
  not return `NULL`, and callers do not check it. This is deliberate: it keeps
  "what if this failed" off every function in the engine. A genuinely large,
  externally-sized allocation is a different problem and is not in this card.
- Two separate pushes are **not** guaranteed to be contiguous — a chained block
  may sit anywhere. Say so in the file header; it is the one sharp edge of this
  design and the one thing a caller can get wrong.
- Single-threaded. No locking, no atomics. Say so in the header.

## Tests

`base/tests/arena.c`, per card 002.

- A push returns memory that is writable, zeroed, and 16-byte aligned.
- A push larger than one block works.
- `rewind` to a mark, then push: the same addresses come back.
- `clear`, then push: back to the start.
- Crossing into a new block does not corrupt the previous one. Write a distinct
  pattern into every allocation, push enough to force several blocks, then
  verify all of them at the end — not just the last.

The assert path is not unit-tested here: it aborts the process by design.

## Report back in this card

Which C23 features you used. It decides whether the compiler floor moves off 18.
