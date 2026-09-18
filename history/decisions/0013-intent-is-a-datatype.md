# 0013. Intent is a datatype; the read API and the write API are separate headers

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0011 established that writes cross a folder boundary only as intent, and
reads cross directly and read-only. It left intent as a shape, not a thing.

D-028 asked whether every system should additionally carry a hand-written list
of the components it may change, so that ADR-0011 — otherwise an unenforced
convention — could be checked.

The principal's answer removes the question rather than answering it:

> **Intent is a datatype.** The design of the intent input decides, together
> with the system, what to mutate. Data (read) is a separate type from intent
> (write request).

## Decision

**A component and an intent are two different types.**

| Type | Direction | Audience |
|---|---|---|
| `<module>_component` | Read | Anyone, read-only or by copy |
| `<module>_*_intent` | Write request | Anyone wanting a change |

**The two headers of a module split by audience, which is the same as splitting
by direction:**

| Header | Contains | Who includes it |
|---|---|---|
| `<module>_component.h` | The data, exposed `const` | Readers — anyone |
| `<module>_system.h` | The intent types, and how to submit them | Writers — anyone wanting a change |

Mutable access to a component exists **only inside its own module**. It is not
declared in any public header.

**No separate write manifest. D-028 is closed as unnecessary.** The set of
intent types a module publishes *is* the declaration of what can be changed, and
the system that consumes them is the declaration of who changes it. A hand-kept
list would have been a second copy of information the types already carry, with
the drift that always follows.

## Consequences

- **Ownership becomes compiler-enforced, not convention-enforced.** A foreign
  module that tries to write a component does not compile, because no mutable
  handle to it is in scope. This is the same trade as ADR-0005 — the compiler
  checking what would otherwise be a rule people must remember — and it is the
  third time that argument has decided a question on this project.
- **The guarantee is against accident, not determination.** C permits casting
  away `const`. Adequate: the rule exists to stop mistakes, and a deliberate
  cast is visible in review.
- **The intent type is the threading seam.** ADR-0011 makes intent a direct
  function call for now, and a direct call mutates on the *caller's* thread — so
  the one-writer rule prevents logical conflicts, not concurrent ones. Two
  threads submitting the same intent concurrently is still a data race.
  Because intent is a type, the fix when it is needed — queue the intents and
  have the owning system drain them at a defined point — happens **inside the
  owning module**, with no call site changed. This is what makes the answer to
  "do we need to plan for multithreading now" *no*: the plan is the type.
- **The read/write split is legible at a glance.** Which header a file includes
  says whether it observes or asks. A module including only `*_component.h`
  files is provably a reader.
- No third file kind is introduced. Intents live in the system header, whose job
  is precisely the module's write vocabulary. ADR-0012's `_component` /
  `_system` suffixes stand unchanged.
- An intent type is a public commitment. Changing its shape breaks callers, in
  the way changing a component's fields does not.

## Rejected options and why

- **A per-system write manifest** (`static const component_id writes[]`,
  validated at runtime in debug builds) — rejected. It duplicates what the
  intent types already state, must be maintained by hand, and can drift from the
  code it describes. It bought runtime enforcement; typed intents buy
  compile-time enforcement, which is strictly better and free.
- **One header per module** — rejected. Merging the read and write APIs puts the
  mutable surface in front of readers and loses the audience split that makes
  ADR-0011 visible in the include list.

## Questions this opens

None. **Closes D-028.** The module map (D-002) is now unblocked with nothing
outstanding.
