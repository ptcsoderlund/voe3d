# 0041. Recoverable failure is a returned value the compiler forces you to check

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

D-008, the last Phase 4 convention with a real call site. ADR-0006 says a
convention is decided when a card first needs it; the `render` card needs this
one, and would otherwise invent an answer that every later folder copies.

Most of the space is already closed by decisions taken before this one:

- **Allocation failure is fatal** (ADR-0032). Not on this plate. `CLAUDE.md`
  rule 11 forbids a null check after an allocation.
- **Asserts are for "the program is already wrong"** — `base/assert.h` says so
  in its own header, and says explicitly that recoverable failure is a separate
  question it does not presume to answer.
- **`**` is forbidden, definitively** (`coding_convention.md`, `CLAUDE.md` rule
  6, not to be worked around with a typedef). This eliminates the standard C
  shape `int f(thing **out)` outright — the shape most C libraries use to
  return a status and a pointer together.
- **`new` allocates and returns it to the caller** (ADR-0014). So a `_new`
  cannot return a status; the thing is the return value.
- **Linux kernel coding style** (`coding_convention.md`), which points at
  `0`-on-success integer returns.

**Two call sites now exist, and they disagree, which is what makes the decision
decidable.** Card 004 gave the first: `voe_platform_window_new` has exactly one
way to fail — it opened or it did not — and the coder hit that path for real,
by accident, on a machine whose seccomp filter blocked the socket. A returned
`NULL`, one `if`, a message. Nothing about it wanted an error code, because
there was one question to answer. `render` startup gives the second and is
shaped differently: **three** distinct recoverable failures — no Vulkan loader
(ADR-0040), no device meeting the 1.3 baseline (ADR-0018), no surface for the
window — which differ in what a human must be told and what they must do about
it.

**The reframe that decided it:** ask what a caller *does* differently for those
three. Nothing. There is no fallback, no retry, no degraded mode; all three end
with telling the user and stopping. So they differ in **diagnosis**, not in
control flow, and the requirement is much smaller than a general error system.

NASA/JPL's *Power of Ten* rule 7 — *each calling function must check the return
value of non-void functions, and each called function must check the validity of
all parameters provided by the caller* — draws the same line this engine already
draws between asserts and failures, and adds one word: **must**. See ADR-0043.

## Options considered

### Option A — `NULL` and `bool` only; the failure site prints why
What exists today. Costs nothing, adds nothing. The caller learns *that* it
failed and never *why*, so `render` must print "no Vulkan driver found" to
stderr from inside a library — an output policy decided in the wrong place, fine
for a console tool and wrong for anything with a log window or no console.

### Option B — `NULL` for control flow; an error code where there is more than one way to fail
```c
[[nodiscard]] voe_render_device *voe_render_device_new(voe_platform_native native,
                                                       voe_base_error *error);
```
One asterisk, so legal. `_new` still returns the thing. `error` may be `NULL`
when the caller does not care. One shared enum in `base`, one
`voe_base_error_string()`. Nothing allocates, so there is no error object to
own or free — which matters under ADR-0032, where an allocating error type
would need an arena to be born into.

### Option C — a result struct returned by value
```c
typedef struct { voe_render_device *device; voe_base_error error; } ...;
```
The most data-driven, which `guidelines.md` names as the first preference. But
`_new` stops returning the thing, every module grows its own result type, and
`platform` — which works — gets churned.

### Rejected on sight — the kernel's own answer, `ERR_PTR`/`IS_ERR`
Encoding an error code inside the pointer value. The most literally
kernel-faithful option available, and refused: a pointer that is not a pointer
is exactly the implicit flow `coding_convention.md` exists to prevent — *"Be
explicit about implicit flows."* It earns its keep in the kernel for reasons
(millions of lines, no room for a wider return) that do not apply here.

## Decision

**Option B, governed by one rule:**

> **A function with exactly one way to fail returns `NULL` or `false` and takes
> no error parameter. A function with several distinguishable failures takes
> one.**

|  | One way to fail | Several |
|---|---|---|
| Returns a thing (`_new`) | `NULL` | `NULL`, plus `voe_base_error *error` |
| Everything else | `bool` | returns `voe_base_error` (`VOE_BASE_OK` is `0`) |

The second row is kernel style exactly: zero on success, a code otherwise.

**The deciding factor: it adds ceremony only where there is something to say.**
`platform` is not retrofitted and not touched.

**`[[nodiscard]]` on every function that can fail.** This is the load-bearing
half. C23 has the attribute, every target already builds `-Wall -Wextra
-Wpedantic -Werror` (ADR-0027), so a caller that ignores a failure is a **build
failure**, not a review comment. *Power of Ten*'s rule 7 stops being a rule
anyone must remember. This is the sixth time on this project a rule people must
remember has been swapped for one the machine checks, and the cheapest of them.

**The error code is diagnosis; the return value is control flow.** They are not
alternatives. The `NULL` is what `[[nodiscard]]` forces you to look at; the code
only says which of several things went wrong.

**One enum, in `base`.** Every folder already depends on `base`, so no map edge
is created. `app` and `dev` handle failures from several folders and get one
type, one `switch`, one string function — rather than `voe_render_error`,
`voe_assets_error` and `voe_platform_error`, four types all meaning "it did not
work", which is the premature generality the standing rules name.

**Two disciplines that stop the shared enum rotting into a dumping ground:**

1. **Codes are added on demand** (ADR-0034). The enum starts with exactly what
   the `render` card needs and nothing else. No taxonomy is designed up front.
2. **Codes are categories, not incidents.** The specific incident goes in the
   message at the failure site and in the file header. Otherwise `base` gets
   edited every time any folder gains a failure, which couples every folder
   through one header.

**Bad parameters are not failures.** A `NULL` where the API requires a thing is
the caller's bug and goes through `VOE_BASE_ASSERT`, exactly as `assert.h`
already says. This decision governs the world being uncooperative, never the
program being wrong.

## Blast radius

**Reversibility: cheap now, moderate later.** Today one folder uses it. The
expensive direction is Option C: converting to result structs later means
touching every `_new` in the engine and every call site. Converting *from*
Option A to Option B — which is what this is — is additive, which is why doing
it now, with one consumer, costs nothing.

`[[nodiscard]]` is free to remove and expensive to add late: adding it to a
mature codebase surfaces every existing unchecked call at once, as errors, under
`-Werror`.

## Consequences

- **Two shapes exist in the engine**, and a writer must judge "one failure or
  several". Accepted deliberately: the judgement is easy, and the alternative is
  ceremony on every fallible function including the ones with nothing to report.
- **`platform` is untouched.** `voe_platform_window_new` returning bare `NULL`
  remains correct under this ADR, not a legacy exception — it has one way to
  fail. It does gain `[[nodiscard]]`.
- **`voe_base_error` and `voe_base_error_string()` are new public surface in
  `base`**, and are written by the `render` card that first needs them, not
  before (ADR-0034).
- **A returned error is never ignored silently**, because ignoring the return
  value does not compile. Passing `NULL` for the `error` out-parameter is still
  allowed and is not silence: it means *I have checked that it failed and do not
  need to know which way*.
- **We give up a stack of error context** — no chain of "while doing X, while
  doing Y". If that is ever wanted it is a new ADR, and it will want an arena to
  build the chain in. Not speculated on now.
- **Nothing here is a system.** `voe_base_error` is a value type with support
  functions, so under ADR-0014 it needs no component and no owning system.

## Rejected options and why

- **Option A** — rejected because it forces a library to own output policy. The
  three `render` failures need different sentences said to a human, and `render`
  is the wrong place to decide where a sentence goes.
- **Option C** — rejected on cost, not on principle. It is the cleanest shape in
  the abstract and it churns working code, breaks the `_new`-returns-the-thing
  rule, and multiplies types. Revisit only if out-parameters prove genuinely
  unpleasant in practice.
- **`ERR_PTR`/`IS_ERR`** — rejected as an implicit flow, above.
- **Per-folder error enums** — rejected as four types meaning one thing.
- **Deciding this before a call site existed** — which is what ADR-0006 and
  ADR-0034 forbid, and the reason this ADR is dated today rather than in the
  first week. Card 004's single failure and `render`'s three are the contrast
  the decision rests on, and neither existed a week ago.

## Questions this opens

- **D-008 closed** by this ADR.
- **D-045 (new)** — whether a failure carries a message string alongside the
  code, and who owns its memory. Deliberately not decided: `voe_base_error_string()`
  covers the category, and the incident is printed at the site today. Revisit when
  something needs to *display* a failure rather than print it — most likely the
  editor.
