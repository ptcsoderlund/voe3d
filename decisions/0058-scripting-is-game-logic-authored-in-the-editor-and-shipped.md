# 0058. Scripting is game logic authored in the editor and shipped in the product; C qualifies

- **Status:** Accepted
- **Date:** 2026-09-03
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Amends:** ADR-0057, point 5 and register row D-065 (see *Decision*)
- **Superseded by:** —

## Context

ADR-0057 was written the same day, and it used the word *scripting* the way the
tech lead had framed it in ADR-0052's closing question: an extension layer whose
placement — editor-side or shipped — was itself the open question. Its point 5
deferred "which side of the shipping line a bridge lands on".

The principal corrected the definition the same day:

> In theory, C should also be a viable scripting language. In our case,
> scripting means what you write in editor and being output is gamelogic in the
> finished product.

That is a definition, not a new decision, and it makes half of the deferred
question disappear. It is recorded as its own ADR because ADR-0057 is
append-only and because a definition that lives only in conversation is the kind
of thing that gets re-argued.

## Decision

**Scripting is game logic authored in the editor whose output is game logic in
the finished product.** It is defined by what it produces, not by which language
produces it.

Three things follow directly:

1. **Scripting is on the shipped side by definition.** The question ADR-0057
   deferred — *does* scripting reach the finished product — is answered: it
   does, always. What remains open is **how** a given language's output gets
   there.
2. **C is a scripting language under this definition.** Game logic written in C
   in the editor and cooked into the game binary is scripting, and it is exactly
   the path ADR-0052 already builds: a compile and a link, one binary, players
   install nothing. **The first scripting language is therefore C, and it needs
   no bridge, no runtime and no new decision.**
3. **Any other language must meet the same output.** A .NET bridge, or any
   other, is judged on how its game logic reaches the finished product without
   breaking ADR-0040. That is the question D-065 now carries, and it stays
   deferred until a card for a non-C scripting language exists.

ADR-0057's other points stand unchanged: the editor is C, everything below
scripting is C, a non-C bridge is optional and not privileged.

## Blast radius

**None today.** This ADR names the path that already exists and narrows an open
question. Nothing built or planned changes shape.

## Consequences

- **ADR-0052's Option C stays dead on its own terms.** A player binary with the
  game as data was rejected because it needs a runtime beside the binary. That
  rejection now applies to any *non-C* scripting mechanism individually, when
  its card arrives, rather than to "scripting" as a category.
- **The editor's first job around scripting is authoring C game logic and
  cooking it.** No bridge work precedes that.
- **The principal has released the board.** No further decisions are taken
  ahead of the cards in `voe3d/kanban/todo/`; the tech lead resumes when a card
  consumes an open question.

## Rejected options and why

None weighed. This records the principal's definition and its direct
consequences. The alternative — leaving ADR-0057's narrower use of the word in
place — would have left the record contradicting what the principal said.

## Questions this opens

- **D-065 is reworded, not closed:** *how* a non-C scripting language's game
  logic reaches the finished product under ADR-0040 — ahead-of-time
  compilation into the one binary, a runtime beside it, or something else.
  Trigger unchanged: the first card for a non-C scripting language.
