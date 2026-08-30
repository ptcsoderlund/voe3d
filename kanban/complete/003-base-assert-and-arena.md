# 003 — base: assert and arena

claimed-by: claude-code (kanban-coder)
status: review

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

---

## Result

`status: review`. Markers left in the code: **none**. No `DEVIATION:`, no
`BLOCKED:`.

### What landed

- `base/include/base/assert.h`, `base/src/assert.c`
- `base/include/base/arena.h`, `base/src/arena.c`
- `base/tests/arena.c`
- `base/base.md` updated

Nothing outside `base/` was touched. No container, no string type, no logging,
no allocation tracking — and none was needed: the arena is a chain of blocks it
links itself, which is a `next` pointer and not a container.

### Which C23 features were used

Two, and only in `base`:

- **`[[noreturn]]`** on `voe_base_assert_fail` — the C23 attribute syntax. C11's
  `_Noreturn` is deprecated in C23, so this is the spelling that is not on its
  way out.
- **`alignas` as a keyword**, on the block's flexible array member. In C11/C17
  it is a macro from `<stdalign.h>`; in C23 it is a keyword and that header is
  not included.

**The floor stays at clang 18.** Both have been in clang since well before 18.
Caveat: this machine has clang 22 only, so that is knowledge of the feature's
history, not something compiled here.

### Decisions the card left open

Three, all recorded in the file headers rather than only here:

1. **`VOE_BASE_ASSERT` survives, `VOE_BASE_DEBUG_ASSERT` is compiled out**
   (under `NDEBUG`, which `CMAKE_BUILD_TYPE=Release` already defines — no new
   build machinery). The plain name is the one a writer reaches for without
   thinking, so it has to be the one that still holds in a release build.
   The reasoning is in `assert.h`'s header, as the card asked.
2. **The message is a required second argument.** The card lists four things in
   the output — expression, file, line, message — and the expression cannot
   supply the message.
3. **The mark type is `struct voe_base_arena_mark`, a tag and not a typedef.**
   `voe_base_arena_mark` is already the name of the function that makes one, and
   in C a tag and a function name do not collide. One concept keeps one name;
   the alternative was inventing a second word for the same thing.

### Verified

`cmake -P check.cmake`, all ten steps:

    ok    tools (clang 22, cmake 4.3.0, slangc)
    ok    standalone base
    ok    standalone math
    ok    root configure and build
    ok    guard compiler
    ok    guard version
    ok    guard map
    ok    includes
    ok    tests (1 passed)
    ok    harness reports a failure

**With one caveat that is not this card's doing: `slangc` is not installed on
this machine.** Unpatched, the run stops at step 1 with
`FAIL tools / slangc could not be run: no such file or directory`, before
reaching anything this card touches. The run above was made with a stub `slangc`
on `PATH` (in the scratch directory; nothing in the repository was changed) so
that steps 2–6b were genuinely exercised. Installing `slangc` is a programmer
step per `CLAUDE.md`, so this is reported, not worked around.

Also run, beyond the check:

- `ctest -R base` — `base/arena` passed, as the card requires.
- **Release build** (`CMAKE_BUILD_TYPE=Release`) configures, builds and passes,
  which is what exercises the compiled-out branch of `VOE_BASE_DEBUG_ASSERT`.
- **Both macros proven to behave as documented**, with a scratch program outside
  the repository: in Debug the debug assert fired and aborted; under `-DNDEBUG`
  it vanished and execution reached the always-on assert on the next line, which
  then fired. Message format:

        ASSERT  probe.c:6
                argc == 999
                the debug assert fired

- **ASan + UBSan**, `-fno-sanitize-recover=all`: the test binary runs clean, no
  leak reported.

### Each test was made to fail

A test that has never failed proves nothing, so the arena was broken six ways
and every break was caught:

| Sabotage | Caught by |
|---|---|
| `push` stops zeroing | `all_zero(p, sizes[i])` |
| the bump ignores alignment | `aligned(p)` |
| `rewind` forgets the block, resets only the offset | `after[i] == before[i]` |
| an oversized push is clamped to the block size | ASan heap-buffer-overflow |
| inserting a block drops the rest of the chain | ASan `Direct leak of 1056 bytes` |
| a full block wraps round and reuses the first | the corruption test, at every chunk |

The fifth of those found a real gap while it was being written: the listed tests
never reached the branch where a rewind keeps a block that a later, larger push
cannot use, so a leak there went unnoticed. `a_kept_block_too_small_is_not_lost`
was added to close it. It tests behaviour the card already specifies, on a path
the listed cases miss.

### Not verified

- **Windows.** Linux only. Nothing here is platform-specific — `malloc`,
  `memset`, `fprintf`, `abort` — but it has not been run there.
- **`git` is still unusable in this checkout.** `.git` is a gitlink to
  `../.git/modules/voe3d`, and that path does not exist, so every `git` command
  exits with "not a git repository". Unchanged from cards 001 and 002. This card
  was moved with `mv`, and nothing was committed.

## Notes — suggestions, not done here

- **`base/src/version.c` and `base/include/base/version.h` still exist**, and
  their own headers say "Delete when the first real file lands." The first real
  files have now landed. Deleting them is not in this card's scope, so they were
  left alone and `base.md` still lists them. Worth a line in the next `base`
  card. Note that `version.c` is where the tree's
  `static_assert(__STDC_VERSION__ >= 202311L)` lives, so deleting it should move
  that assertion rather than drop it.
- **`push` zeroes on every call.** It has to, because a rewound block is dirty
  and the card requires zeroed memory. For a large push that the caller
  overwrites immediately it is a real cost. If that ever shows up in a profile
  the answer is a second entry point that says "not zeroed" in its name — not a
  flag, and not a change to this one.
- **Nothing detects two threads on one arena.** The header says single-threaded;
  no code enforces it. A debug-only owner-thread check would need a thread id,
  which needs the OS, which `base` may not call. Worth knowing that this is a
  `platform` question if it ever becomes one.
