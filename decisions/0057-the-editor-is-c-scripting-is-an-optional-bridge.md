# 0057. The editor is C; scripting is an optional bridge on top of the engine

- **Status:** Accepted
- **Date:** 2026-09-03
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Amends:** ADR-0009 (scripting leaves the non-goals list; see *Decision*)
- **Superseded by:** —

## Context

ADR-0052 closed with a question: the principal had stated a direction toward
.NET scripting in the editor, and it was not clear which side of the shipping
line that landed on. Register row D-059 carried it as *wants an ADR before any
editor card*.

What depends on it: every editor card, the shipping shape (ADR-0052), the
install-nothing rule (ADR-0040), and whether the principal's *deliberately not
doing: scripting* line in `STATUS.md` still stands.

Constraints already fixed:

- **ADR-0040** — end users install nothing; a game is one binary.
- **ADR-0008** — folders link statically; no runtime code loading.
- **ADR-0023** — dependencies default to no; each one is the principal's yes.
- **ADR-0052** — engine, editor, game; the editor is *a program built on the
  engine*; cooking is a compile and a link.
- **ADR-0055** — fast rebuild is the iteration strategy, five seconds the
  budget, explicitly scoped to the C engine and game.
- **The principal's standing rule:** heavy engine work is always C.

Those constraints kill the Unity shape — a JIT runtime shipped beside the game
— before options are weighed.

## Options considered

### Option A — editor-only scripting; the editor hosts .NET
The editor embeds the .NET runtime for panels, tools and custom modules. Engine
and game stay C; shipping decisions stand. Makes permanent: the editor is a .NET
host, the .NET SDK joins the installed-tools list, the engine's public headers
become a binding surface. Widening to shipped games stays possible; narrowing
back does not once tools depend on it. **The tech lead's recommendation.**

### Option B — scripting reaches shipped games, compiled ahead of time
Gameplay in C#, compiled by NativeAOT at cook, linked into the one binary.
Players still install nothing. Costs: Play must either AOT-compile every time,
far outside ADR-0055's budget, or JIT in the editor and AOT at ship — which
recreates the "it worked in the editor" bug class ADR-0052 was taken to prevent.
Rewrites ADR-0052 and amends ADR-0040.

### Option C — the editor is C; scripting is an optional bridge on top
The editor is a C program built on the engine, exactly as ADR-0052's table
already says. Everything up to scripting is C: engine, editor, cook, game code.
Scripting is a separate, optional layer that binds the engine's public C API.
.NET is the expected first bridge but not a privileged one; someone who wants
another language writes their own bridge against the same headers. The editor
works with no bridge present. Costs: editor tools written by people who do not
write C wait for a bridge to exist. Makes permanent: the public C API is the
thing every bridge binds, so its stability starts to matter the day the first
bridge exists.

## Decision

**Option C.** The principal's call, against the tech lead's recommendation, on
two grounds.

**Dogfooding.** The editor is the engine's first real program and its most
demanding consumer. If it is written in C against the engine's own API, it
exercises the API the way a game will. An editor whose substrate is .NET would
not.

**Extensibility.** A C API is the lowest common denominator every language can
bind. Making .NET the editor's substrate would make it the *only* first-class
extension language. Making it one optional bridge among possible bridges keeps
the door open for all of them and closes it for none.

Concretely:

1. **The editor is a C program built on the engine.** ADR-0052's table is
   unchanged; this ADR states what was implicit.
2. **Everything below scripting is C** — engine, editor, cook, game code. The
   principal's "heavy engine work is always C" is restated with a wider scope:
   it is not only the heavy work.
3. **Scripting is an optional bridge layer on top of the engine's public C
   API.** It is not part of the engine and not part of the editor. The editor
   builds and runs with no bridge present.
4. **.NET is the expected first bridge, not a privileged one.** Nothing in the
   engine or editor knows about .NET. A .NET bridge, when it exists, is one
   module and one dependency, taken under ADR-0023 on its own merits.
5. **Which side of the shipping line a bridge lands on is not decided here.**
   The principal's words: *way ahead of development*. It is deferred with a
   trigger — the first scripting card — and until then ADR-0052's rejection of a
   player binary stands and ADR-0040 is untouched.
6. **Amends ADR-0009.** Scripting leaves the non-goals list and becomes a
   *later, optional layer*. Networking stays a non-goal.

## Blast radius

**Cheap today, moderate later.** Nothing editor-shaped is built, so the editor
being C costs nothing now. Reversing it once an editor exists — rewriting a C
editor as a .NET host — is a rewrite of the editor, not of the engine.

The load-bearing part is quieter: **the public C API becomes the bridge
surface.** ADR-0013's two-header split already separates public from internal,
so the surface exists. What this ADR adds is that, from the first bridge on,
changing a public header breaks something outside the engine. That is an
obligation the engine did not carry before.

Reversibility: **moderate**.

## Consequences

- **Extending the editor means writing C and rebuilding.** ADR-0055 makes that
  five seconds. The editor's extension model and the game's iteration model are
  the same mechanism, which is consistent with dogfooding.
- **No new dependency today.** The .NET SDK is *not* on the installed-tools
  list in `voe3d/CLAUDE.md`. ADR-0023 is honoured without a fresh exception.
- **Editor tools from non-C programmers wait for a bridge.** This is the cost
  the tech lead argued against, and it is accepted with eyes open: the first
  editor tools will be written by the people writing the engine anyway.
- **A bridge's mechanism is that bridge's business.** How a .NET bridge hosts
  the runtime, how bindings are generated from the public headers, whether the
  bridge runs inside the editor process — all decided when the first bridge
  card exists, not now.
- **Public header stability arrives earlier than it otherwise would.** The
  consequence we like least. Until the first bridge, headers churn freely, as
  they should under ADR-0034. After it, each public change has an external
  consumer. The register carries this as a question with a trigger.
- **The shipping question is deferred, not closed.** ADR-0052's Option C — a
  player binary with the game as data — stays rejected unless and until the
  deferred question reopens it.
- **`STATUS.md`'s *deliberately not doing* line changes.** Networking remains;
  scripting moves to *later*.

## Rejected options and why

- **Option A** was the tech lead's recommendation and was rejected because it
  makes .NET the editor's substrate rather than an optional layer. That fails
  dogfooding — the engine's most important program would not consume the
  engine's C API the way a game does — and it privileges one extension language
  over all others. It would also have added a dependency (ADR-0023) before any
  card needed it.
- **Option B** was rejected as premature rather than wrong. It decides the
  shipping side of scripting before an editor, a bridge, or a scripting card
  exists. The principal declined to decide that far ahead, and the question is
  parked with a trigger instead.

## Questions this opens

- **D-065 — which side of the shipping line a scripting bridge lands on.**
  Editor-only, or reaching shipped game code. Inherits the collision analysis
  from D-059: shipped scripting collides with ADR-0040 and revives ADR-0052's
  rejected Option C. Trigger: the first scripting card.
- **D-066 — what the bridge surface is and what it promises.** Whether
  ADR-0013's public header *is* the binding contract, how bindings are
  generated, and what stability the engine owes a bridge across changes.
  Trigger: the first bridge card.
- **D-060 (existing) — where the editor sits relative to the module map** — is
  narrowed by this ADR: it is a C program built on the engine, so the remaining
  question is folder, executable beside `app`, or repository. Trigger unchanged.
