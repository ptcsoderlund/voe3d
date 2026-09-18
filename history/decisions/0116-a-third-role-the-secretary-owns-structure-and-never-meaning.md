# 0116. A third role, the secretary: she owns structure and never meaning

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0114 gave every always-loaded document a ceiling and an eviction rule. ADR-0115
replaced the ceiling as the working test with a semantic one: a hot document holds only
what its reader can act on now. Between them they created a standing body of work that
is not technical — moving 113 register rows verbatim into a cold file grouped by the
condition each one names, keeping indexes true, updating every reference, archiving what
is finished.

That work was landing on the tech lead, and it is the wrong desk. The tech lead's scarce
resource is judgment about the engine; an hour spent moving table rows is an hour not
spent on the eleven open questions that are actually waiting on the principal. It is also
work the tech lead does badly, because the temptation in the middle of filing is to
improve what is being filed — and ADR-0056 is this repository's record of what happens
when somebody paraphrases what another person believed they saw.

The principal introduced a third agent role, with its skill already written:
`~/.claude/skills/secretary/SKILL.md`. Its own summary is the decision in one line —
*owns structure, never meaning.*

Constraints already fixed: skills are user-level and shared across projects, and
everything project-specific lives in the two `CLAUDE.md` files (ADR-0113); the repository
is the only memory across machines; the decision records, the status log, the archives and
`complete/` are append-only; a coder works inside `voe3d/` and reads only its card
(ADR-0102 and the engine rules); the submodule is a separate repository and the principal
handles every remote.

## Options considered

### Option A — No third role; the tech lead files as part of recording a decision
What was happening. Zero new structure, and it keeps eviction welded to the write that
causes it, which is ADR-0114's whole mechanism. It fails on volume and on temperament: a
one-off split of two files is a tech-lead job, a standing filing practice is not, and the
role most likely to rewrite a sentence while moving it is the role that wrote it.

### Option B — The secretary as a subagent the tech lead dispatches
The tech lead calls her mid-session when a document needs filing. Convenient, and her
file-reading stays out of the tech lead's context. But her *report* lands in it, and her
work then arrives inside a session that is also holding a decision — so the tech lead
becomes her reviewer, which makes him accountable for filing again by the back door. It
also breaks her own rule against filing over another role's shoulder, since the tech lead
is by definition editing documents in that same session.

### Option C — A peer role in her own session, reporting to the principal
She is started by the principal like the other two roles, on a clean tree, and her work is
one reviewable change read as *from → to*. Her findings go to whichever role owns the
content. The tech lead hands her errands as text, and never sees her file listings at all.
Costs a third role in the map, and a hand-off that has to be written down rather than
spoken.

## Decision

**Option C.**

The deciding factor: the purpose of the whole hot-and-cold effort is to keep a role's
working context down to what it can act on, and a subagent whose report lands in the tech
lead's session spends the saving it was hired to make.

**1. What she owns.** Where a document is, what it is called, how it is found, and what
has been archived. Categorising, sorting, moving, splitting, merging, renaming, indexing,
archiving. Her own text is index lines, pointers, headings, filing notes and the folder
maps she proposes.

**2. What she never does.** Change meaning — no paraphrase, no tightening, no summary
written over an original; a sentence she did not write leaves a file exactly as it entered
it. Set a status: *open*, *decided*, *deferred*, *not a bug* are facts other roles
establish. Decide, answer or fix anything. Edit an append-only zone. Delete — she archives,
and deleting needs the principal's word per file. Touch code. Commit unasked, or push ever.

**3. Where she works.** The planning root is hers. **`voe3d/` is not, except by a named
errand, and never while a coder is working in it** — her own rule against filing over
another role's shoulder, and the engine's rules put its folder summaries and cards under
the coder's and the tech lead's hands. Two clauses of the root's hard rules bind her
without amendment: never commit inside the submodule while it is detached, and do not edit
engine code from the root.

**4. The hand-off, in both directions.** The tech lead hands her an errand as text, naming
the files and the filing rule, and — where the rule turns on a status — **the status**.
For ADR-0115's register partition this means the tech lead names which rows are hot,
because judging whether an arrival condition has fired is content and therefore not hers.
She hands back the report her skill specifies: moves as *from → to*, what she created,
references updated, findings, and what she left undone. **Findings are not hers to act on**
— a stale fact, a contradiction between two files, a status that looks wrong, a hot file
doing two jobs all go to the role that owns the content.

**5. A hot file that is long because all of it is live is not her problem.** Under
ADR-0115 length is a symptom. She reports it and leaves it.

## Blast radius

Small. The role is a way of working, not a structure the engine depends on; withdrawing it
returns the filing to the tech lead and leaves every document where she put it. Her
operations are verbatim moves in a git repository the principal reviews before anything
leaves the machine, so the worst single mistake is a bad move that `git` reverses.

The one thing that becomes expensive: if she is ever permitted to paraphrase, the damage is
unbounded and invisible, because the original is gone and nobody knows which sentences were
touched. That is why rule 2 is absolute rather than a preference.

Reversibility: **cheap** for the role, **load-bearing** for the no-paraphrase rule.

## Consequences

- The tech lead stops filing and stops reading file listings. The register partition, the
  `render` folder summary, the `CLAUDE.md` trimming and every future eviction are errands.
- A third role is a third place the project's rules can drift, and the drift shows up as
  three `CLAUDE.md` role rows instead of two.
- **A real risk: she and a coder collide inside `voe3d/`.** A coder moved a card to
  `review/` during the tech-lead session that wrote this ADR. Rule 3 is the guard, and it
  depends on somebody knowing whether the board is quiet — which is the principal, since he
  starts every session.
- Her findings need a reader. A finding handed to nobody is worse than not noticing it, so
  her report goes to the principal and he routes it, in the same way a bug report does.
- She is likely to be right about things outside her authority, often. The discipline that
  she reports and does not act is what keeps her cheap to trust.

## Rejected options and why

- **A, the tech lead files.** Rejected on volume and on conflict of interest. It remains
  correct for the small case ADR-0114 describes — the eviction that is part of a single
  write, such as a new *Latest* sending the old one to the log. That stays with the writer.
- **B, the subagent.** Rejected because her report would land in the context the role
  exists to protect, and because it would quietly make the tech lead her reviewer. Worth
  revisiting only if a filing errand ever needs to happen *inside* a decision rather than
  after it, and no such case is known.

## Questions this opens

- Who tells the secretary the board is quiet, given rule 3, and whether that is the
  principal every time or something on the board says it.
- Whether the folder summaries inside `voe3d/` are ever hers by standing arrangement
  rather than by errand, since they are documents a coder loads unasked and therefore hot
  by ADR-0115's test.
