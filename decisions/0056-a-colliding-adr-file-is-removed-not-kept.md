# 0056. A colliding ADR file is removed, not kept

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes the drift recorded as "A session's record was lost".** That record was
wrong, and this ADR replaces it.

On 2026-09-01 a tech-lead session found that card 009 and `voe3d/cmake/voe.cmake`
both cited ADR-0046 and ADR-0047, and that neither file existed. It concluded a
session's output had been lost, reconstructed both ADRs from that evidence, and
declared numbers 0044 and 0045 permanently unused because nothing cited them and
their subjects looked unrecoverable.

**Nothing had been lost.** All four ADRs had been written on 2026-08-31 and
committed to the remote `main` in `fd31124` ("Yes"). The 2026-09-01 session was
working in a tree that had not merged that commit. The merge `c40b4a2` brought
the originals in, and the tree now holds two files at each of four numbers:

| Number | Original (`fd31124`, 2026-08-31) | Surplus (`46683ff`, 2026-09-01) |
|---|---|---|
| 0044 | `compile_commands.json` beside each `CMakeLists.txt` — closes D-048 | `0044-unused.md` |
| 0045 | Step 7 reads `compile_commands.json`, default checkers, `[[clang::suppress]]` — closes D-046, D-049, D-050 | `0045-unused.md` |
| 0046 | Shaders compiled by `slangc` at build time and embedded — closes D-007a, D-052 | reconstruction |
| 0047 | The Clang floor is 19 — closes D-033 | reconstruction |

**No decision is in dispute.** Both files at 0046 decide Option B — `slangc` at
build time, SPIR-V embedded with `#embed`. Both files at 0047 decide the floor is
19. The originals are fuller and cite the register rows the reconstructions had to
infer. The two stubs assert that the subjects of 0044 and 0045 are unrecoverable
while those subjects sit beside them in the same directory.

What is broken is only this: **four numbers resolve to two files each**, in a log
whose value rests on a number resolving to one document. That is the failure the
append-only rule exists to prevent, and it is currently in the tree.

The hard constraint is ADR discipline itself, as stated in the tech-lead skill:
*never delete an ADR*, and *superseded, never edited in place*.

## Options considered

### Option A — remove the surplus four, record the recovery here
The two stubs and the two reconstructions leave the tree. Each number resolves to
one file again: the one its author wrote. Git retains the removed files. This ADR
is the record of what happened and of the condition under which removal is
permitted.

Costs a deliberate exception to the never-delete rule, which must therefore be
written narrowly or it becomes a licence to tidy the record generally.

### Option B — keep all eight, mark the surplus withdrawn
Strictly append-only. Each surplus file gains a banner pointing at the original.

Costs the log its defining property, permanently. `grep ADR-0046` returns two
hits forever; every future reader adjudicates between two files before reading
either. The two stubs go on asserting something false with a banner above it. It
preserves the corruption rather than repairing it.

### Option C — renumber the reconstructions to 0056 and 0057, withdrawn
No collisions, no deletions.

Costs two further numbers, permanently spent on documents that duplicate ADR-0046
and ADR-0047 and carry no content those do not.

## Decision

**Option A.** The deciding factor: **the never-delete rule protects decisions from
disappearing, and under Option A no decision disappears** — every decision at
0044–0047 survives in the original file its own author wrote, and only duplicates
of decisions we hold in better form leave the tree.

**The exception is narrow and is the whole of it.** A file may be removed from
`docs/adr/` when, and only when, all three hold:

1. it shares a number with another file that is present in the tree;
2. that other file is the original — written first, by the session that took the
   decision — and decides the same thing;
3. the removal is recorded in an ADR that names the removed file.

Anything else — a superseded ADR, a decision we regret, an ADR nothing cites —
stays. Those are the cases the rule is for.

**Files removed under this ADR:** `0044-unused.md`, `0045-unused.md`,
`0046-shaders-compiled-at-build-and-embedded.md`,
`0047-the-clang-floor-is-19.md`.

**Files that stand:** `0044-compile-commands-beside-each-cmakelists.md`,
`0045-how-step-7-runs-the-analyser.md`,
`0046-shaders-compiled-offline-and-embedded.md`, `0047-clang-floor-is-19.md`.

## Blast radius

Small, and this is cheap to reverse: git holds every removed file and each is
recoverable by name from `46683ff`.

What is expensive to change is not the files but **the rule**. An exception
written loosely — "remove ADRs that are redundant" — would licence exactly the
quiet history-rewriting ADR discipline exists to stop, and no later ADR can
un-teach a precedent already set. That is why the three conditions above are
stated as conditions rather than as judgement.

Reversibility: **cheap** for the files, **load-bearing** for the rule.

## Consequences

- Each of the four numbers resolves to one document again. Every citation from
  the engine repository — card 009, `voe.cmake`, `CLAUDE.md` — now resolves to
  the ADR its author meant.
- The never-delete rule has a written exception where it previously had none.
  Anyone reading `docs/adr/` for the first time now needs this ADR to know the
  boundary. That is a real cost and the reason the boundary is enumerated.
- **The two reconstructions were good work and are still discarded.** They were
  written honestly, said at the top that they were reconstructions, and reached
  the same decisions as the originals — which is the strongest evidence available
  that the reconstruction method worked. That fact is worth keeping even though
  the files are not.
- **D-048, D-049 and D-050 are no longer "assumed consumed".** Their subjects are
  known: D-048 by ADR-0044, D-049 and D-050 by ADR-0045. They stay retired, as all
  consumed ids do.
- **D-051 was never cited by anything**, including the recovered originals. It may
  never have been consumed at all. It stays retired regardless, because it cannot
  be shown to be free and the cost of a wrong reuse is exactly what this ADR is
  about.
- `STATUS.md`'s "A missing record, rebuilt" section is now false and is replaced.

## Rejected options and why

**Option B** was rejected because it honours the rule's letter while surrendering
what the rule is for. A log where a number resolves to two documents has stopped
being a log; keeping the collision visible does not make it navigable, it makes it
permanent. The two stubs make this worse than a neutral duplicate — they state, in
the accepted-record directory, that information is unrecoverable when it is
present three files away.

**Option C** was rejected as the tidy answer that costs the most in the long run.
It spends two more numbers to preserve text that duplicates other text, and leaves
a reader at 0056 asking why a withdrawn duplicate of 0046 was worth a number of its
own. Numbers are the scarce thing in an append-only log, not disk.

## Questions this opens

- **D-064** — whether the root repository's git workflow needs a stated rule about
  reading the remote before a session writes records. The proximate cause here was
  not a lost commit but a session reasoning from an unmerged tree, and it produced
  a confident, documented, wrong conclusion in the one place that exists to be
  trusted. The principal owns all remotes (`CLAUDE.md`), so this is a question
  about what an agent must check before writing, not about who fetches.

**The standing lesson stands, restated correctly.** The old drift note drew the
moral that a decision existing only in conversation does not exist. True, but not
what happened. The actual lesson is narrower and sharper: **an agent that cannot
see the whole record must not conclude that the part it cannot see is missing.**
The repair was performed competently on damage that did not exist, and it caused
the only real damage in this episode.
