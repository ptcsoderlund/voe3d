# 0143. A card's folder is its status, and the `status:` field goes

- **Status:** Accepted
- **Date:** 2026-09-12
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

A card carries a `status:` line in its header — `todo`, then `review`. The folder it
sits in says the same thing. Nobody had ever said which of the two is the record.

On 2026-09-12 the tech lead reported to the principal that three finished cards "never
sat in `review/`" and asked him to confirm they had been reviewed. The report was
wrong. The principal's answer states the working practice: **he moves a card when it is
done and does not write inside it.** A card's header is written by whoever wrote the
card and by the coder who worked it; the principal's act is the move.

The evidence backs him and is unambiguous. Every card in `complete/phase4_editor/` and
`complete/phase4_editor/plumbing/` — 048 through 054, 057, 058a — reads `status:
review`. The coder set that when it handed the card in and nothing has touched it since.
There is not one card anywhere on the board whose `status:` field reads `complete`. The
field is not merely unreliable in `complete/`; **it has never once been correct there.**

So the question is not which to trust. It is what to do with a field that is stale by
design and that has now cost a session's reasoning and a wrong statement to the
principal.

## Options considered

### Option A — the folder is the status, and the field goes
`todo/`, `review/`, `complete/` are the three states and the only record of them. The
`status:` line comes out of `CARD-TEMPLATE.md` and out of the coder's write-up
instructions. `claimed-by:` and `blocked-by:` stay — neither is derivable from the
folder, and `claimed-by:` is the one field that genuinely tells a second coder to keep
its hands off.

### Option B — the folder is the status, the field stays as the coder's note
Keep `status:` as a coder-owned scratch field, documented as meaningless outside
`todo/` and `review/`, and read by nobody looking for state.

### Option C — the field is the status and the principal maintains it
Make the header authoritative and ask the principal to edit `status: complete` when he
moves a card.

## Decision

**A card's folder is its status. There is no other record of it, and the `status:` field
is removed from the template and from what a coder writes.** The deciding factor: the
field is duplicated state with one writer who never updates it and several readers who
believe it, which is the shape of every bug of this kind. Option A rather than B because
a field documented as *stale by design* is a trap with a sign on it — the sign is not in
the reader's context at the moment they read the field.

## Blast radius

Nearly none. Three states in three directories, already how the board actually works; the
change is deleting a line from a template and from the coder's instructions. Reversing it
means re-adding a header line and would cost an afternoon of nothing.

Reversibility: **cheap.**

## Consequences

- `CARD-TEMPLATE.md` loses its `status:` line. `claimed-by:` and `blocked-by:` stay.
- The coder's hand-in step stops setting `status: review` and is the move to `review/`
  alone, which is what it always materially was.
- **Cards already in `complete/` keep their stale `status: review` and are not
  retro-edited.** `complete/` is cold and append-only (root `CLAUDE.md` hard rule 5); a
  sweep to correct a field we have just abolished would be work in the wrong direction.
  A card in `todo/` or `review/` may lose the line whenever it is next opened for another
  reason.
- **Neither edit happens today.** Both files are inside `voe3d/`, where a coder is
  working; the template edit is a filing act and belongs to the secretary, timed by
  D-222 — who tells her the board is quiet.
- An agent reading the board reads directory listings. It already did; now that is the
  documented way.

## Rejected options and why

**Option B** keeps a field whose only property is that it lies. The cost of removing it
is one line; the cost of keeping it is that every future reader must know a convention
that is not next to the thing it governs. That is the trade we refuse everywhere else.

**Option C** asks the principal to do bookkeeping a directory listing does for free, and
it would have to survive him working from several machines and moving cards in a file
manager. He told us plainly that he moves cards and does not write in them. Building a
rule that requires the opposite is how the record drifts from the practice.

## Questions this opens

No new register row. This is a concrete instance of **D-219** — the coder sets `status:`
because its machine-local, gitignored skill tells it to, and that instruction is not in
the repository. The audit D-219 asks for now has one named finding in front of it rather
than a *may*.
