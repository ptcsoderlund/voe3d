# 0114. Every always-loaded document has a ceiling, and the write that heats one thing cools another

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (amends the *shape* of `STATUS.md`, the decision register and a
  card; changes no decision any of them records)
- **Superseded by:** —

## Context

The principal asked whether the agents' context could be reduced: quota runs out
faster and both roles work slower than they did a few weeks ago. Measured on
2026-09-10, before anything was changed:

| Document | Size | Loaded by |
|---|---|---|
| `STATUS.md` | 6,055 lines, 350 KB, ~88k tokens | tech lead, every session |
| `docs/prestudy/decision-register.md` | 3,648 lines, 369 KB, ~92k tokens | tech lead, every session |
| `docs/adr/` | 113 files, 1.1 MB | tech lead, when grepped whole |
| `voe3d/CLAUDE.md` + `guidelines.md` + `coding_convention.md` | 25 KB, ~6k tokens | coder, every card |
| one card in `todo/` | 20–34 KB, 5k–8k tokens | coder, per card |
| the two `SKILL.md` files | 5 KB each | either role, once |

A tech-lead session that opened `STATUS.md` and the register whole had spent about
180k tokens before the principal had said a word, and every later turn re-sent all of
it. Neither file was designed to be that size. `STATUS.md` was meant to be one page
for the principal and became a dated log with a *Latest* block on top; the register's
own header says *settled decisions are not listed here* while 66 of its 205 rows are
`Decided` and 3,000 of its lines are dated session entries. Cards, meant to restate a
decision, had grown the decision's argument back into them. The skills themselves were
never the cost.

**The repository already practises the fix in two places without naming it.** The
coder rule *a change to a folder's public surface updates its `<folder>.md` in the
same change* keeps a small summary current by the write that changes the large thing
it summarises. ADR-0110's *a report moves to `bugs/archive/` in the same act that
creates its card* is the same shape. The principal named it: classic hot and cold
data partitioning. What was missing was the rule stated once, applied to every
document a session loads unasked, with a ceiling and an eviction step.

Constraints already fixed: `STATUS.md` is the principal's only view and it is not
renamed; ADRs are append-only and are the record (ADR-0056's narrow exception aside);
the repository is the only memory across machines (`CLAUDE.md`, and ADR-0113 for the
skills); nobody reads the planning root from `voe3d/` (rule 3).

## Options considered

### Option A — Partition every always-loaded document into a hot page with a ceiling and a cold, append-only tail, and make eviction part of the write
Each hot document gets a line ceiling. The act that adds to it is also the act that
removes what it replaces: a new *Latest* moves the old one to a log, a decision
recorded as an ADR deletes its register row, a card carries an ADR's conclusion and
name and never its argument. A script at the root prints every hot file against its
ceiling so the rule is seen, not remembered. Costs a one-time split of two large
files and a habit change in the tech-lead skill. Makes permanent: the log exists as a
separate place, and the register is a queue rather than a diary.

### Option B — Keep the documents as they are and instruct each skill to read only the top of them
`head -120 STATUS.md`, grep the register for `Open`. Zero restructuring. Costs
nothing today and fails slowly: the files keep growing, every grep over them costs
more, an agent that needs one older fact must scan the whole thing, and *read only the
top* is exactly the kind of instruction a session forgets when the question is
interesting. It also leaves the register contradicting its own header.

### Option C — Rely on the model's long context and session compaction
Do nothing; the 1M-token window holds it all. This is what has been happening, and
the symptom the principal reported is its cost: nothing compacts until the window is
full, so a session grows for hours, and every turn re-sends the whole history. Cost is
paid in quota and latency on every turn, for content that is almost never consulted.

## Decision

**Option A.** The deciding factor: the two places the repository already does this
are the two places that have *not* grown out of hand, and the two documents that have
are the two with no eviction rule.

**1. The hot set, and its ceilings.** A hot document is one a role loads every
session without being asked. Each has a ceiling in lines, checked by the script below.

| Hot document | Ceiling | Loaded by |
|---|---|---|
| `STATUS.md` | 120 lines | tech lead, the principal |
| `docs/prestudy/decision-register.md` | 200 lines | tech lead |
| `CLAUDE.md` (root) | 200 lines | tech lead |
| `voe3d/CLAUDE.md` | 400 lines | coder |
| a card in `voe3d/kanban/todo/` or `review/` | 150 lines | coder |
| a `<folder>/<folder>.md` in the engine | 120 lines | coder |

Ceilings are in lines because lines are what a script can count and what a page
means; a line is expected to be prose of ordinary length, and a cell that runs to a
paragraph is over the ceiling in spirit and gets trimmed when noticed.

**2. Cold is everything else, and cold is append-only and read by lookup.** The
status log, the ADRs, completed cards, the register archive, `bugs/archive/`.
Nothing is deleted from cold; nothing in cold is loaded by default; a role that needs
a fact from cold searches for it — through a subagent when the search is broad, so
the search itself never enters the session's context.

**3. The write that heats something is the write that cools what it replaces, in the
same act.** Named per document:

- **`STATUS.md`.** It holds the promise, the current *Latest* block, and a short
  *where things stand*. Writing a new *Latest* moves the previous one, verbatim and
  dated, to `docs/status-log/YYYY-MM.md`. The dated `#` and `##` sections that used
  to sit below the block live in the log from now on.
- **The register.** It holds open rows, one line each, grouped by the phase or
  concern that consumes them. Writing an ADR that closes a row **deletes the row**;
  the ADR is the record and the row was the queue. A row is never marked `Decided`.
  Session narrative, standing-rule digests and drift notes are not register content;
  they belong in the ADR they come from or in the status log. The full register as it
  stood on 2026-09-10 is kept verbatim at
  `docs/prestudy/register-archive/2026-09-10-before-the-split.md`.
- **A card.** It states what to build, what done looks like, what must not change,
  and how to verify; for *why* it names the ADR in one line. Argument found in a card
  is a sign the ADR is missing something, and the fix goes to the ADR. A card moved to
  `complete/` is cold and its size stops mattering.
- **A `<folder>.md` and the folder's headers.** These are the coder's hot set for a
  folder: the `.md` says what the folder is for and how it couples, the `.h` files
  *are* the surface. A `.c` is opened to edit it, not to learn the folder. A
  `<folder>.md` over its ceiling is reported as a folder that wants splitting, the
  same way an unfinishable card is reported as a card problem.

**4. Enforcement is a script, not a memory.** `tools/hot.sh` at the root prints each
hot file, its line count, its ceiling, and `OK` or `OVER`, and exits non-zero on any
`OVER`. Running it is the first step of the tech-lead skill and the last step before a
card is put in `todo/`. Shell is sufficient because agents run only on Linux here; the
principal does not need it.

**5. The skills stay user-level and are not the repository's.** The principal keeps
`~/.claude/skills/tech-lead` and `~/.claude/skills/kanban-coder` as his own, shared
across several projects, so a skill describes a *role* and nothing specific to VOE3D.
Moving them into the repository was tried the same afternoon and reversed on his
decision. The consequence is the one ADR-0113 already drew: anything that binds a
coder or a tech lead *on this project* is written in `CLAUDE.md` or `voe3d/CLAUDE.md`,
and where a skill and the repository differ the repository wins. D-219 stays open.

## Blast radius

**Small, and mostly reversible by concatenation.** Every move is verbatim, so the
old `STATUS.md` is the new one plus the log, and the old register is in its archive
untouched. What becomes expensive is only the habit: a tech lead who marks a row
`Decided` instead of deleting it, or writes a fourth `##` section under the *Latest*
block, has re-heated the cold data and the script will say so.

The one genuinely load-bearing piece is the register becoming a queue. Anyone who
used it as a diary — *what did we conclude on the 7th* — now asks the status log or
the ADR. That is where those facts belonged.

Reversibility: **cheap.**

## Consequences

- A tech-lead session should start near 10k tokens of documents instead of near
  180k, and stop growing with the project's history.
- A card is about a third of its recent size, and the coder reads the ADR's
  conclusion rather than its argument, which is what rule 3 always intended.
- **The principal loses the scroll.** `STATUS.md` used to let him read backwards
  through every explanation ever given. That reading now happens in
  `docs/status-log/`, one file per month, which is a worse experience for browsing
  and a better one for finding.
- **The register's *Standing rules from accepted ADRs* digest goes cold.** It was a
  useful cheat sheet and it was also 250 lines of ADR content copied into a hot file.
  If a digest is wanted, it is a cold reference file, not the register.
- **Ceilings will be hit**, and the first few times it will feel like bureaucracy. A
  card that cannot fit in 150 lines is usually two cards; a `<folder>.md` that cannot
  fit in 120 is usually two folders. The ceiling is doing its job when it says so.
- Session hygiene is not a repository matter and is noted for the principal in
  `STATUS.md`: one session per card and per decision, cleared between them. A 1M
  window with no compaction pressure is what let sessions grow silently.

## Rejected options and why

- **Option B** because *read only the top* is a rule about reading, and the cost was
  in what the files contained. It would have left both files growing and both
  contradicting their own headers, and would have been forgotten the first time a
  session had a reason to look further.
- **Option C** because it is the status quo, and the principal's report is its
  measured cost.
- **Byte ceilings instead of line ceilings.** More precise, less legible. A line count
  is what a person and a script both see at a glance; long cells are trimmed by hand
  when noticed.
- **Monthly log files cut from today's `STATUS.md` by date.** The old block has no
  dates in its headings and would have needed rewriting to split, and rewriting the
  record is what ADR-0056 exists to warn against. Everything before today is one
  file, verbatim; the monthly files start now.

## Questions this opens

- **D-220** — Whether the register's *Standing rules from accepted ADRs* digest is
  wanted at all as a cold reference file, or whether the ADRs and the two
  `CLAUDE.md` files are enough. Open; trigger: the first session that misses it.
