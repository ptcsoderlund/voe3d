# 0105. A measurement names the sample it took

- **Status:** Accepted
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Opened by:** the coder of cards 032, 033, 039 and 040, who found all four
  instances below and named the class before being shown a frame for it

## Context

**Four defects in one week share one shape**, and the fourth was identified by the
coder who found the other three:

1. **`check.cmake` reported green over a folder that was never built.** Its own
   header states the convention — *"A folder is discovered, never listed"* — while
   the root `CMakeLists.txt` lists `add_subdirectory()` by hand, with nothing
   comparing the two. A folder missing from that list passed its standalone
   configure and was absent from the root build and from `ctest`, and every step
   printed ok. Found while working card 033, repaired by card 039.
2. **The element shader was correct only while there was one surface in the frame.**
   Slang gives `SV_InstanceID` HLSL's meaning — the instance's number *within* the
   draw — so passing a range through `vkCmdDraw`'s `firstInstance` alone drew the
   first `count` records of the buffer for every range but the first. Every frame
   this engine had ever drawn had one surface in it. Found and fenced in card 032.
3. **A verify bullet in card 032 named the wrong object.** It asked for two
   window-shape screenshots of the *overlay panel* to prove that nothing stretches —
   but an overlay panel is in metres and in perspective (ADR-0074) and looks
   identical in both shapes for reasons unrelated to the claim. A coder following it
   literally would have produced two correct screenshots and proved nothing. **This
   one is the tech lead's**, in a card, in the same shape as the three in code.
4. **`dev`'s readout reports its glyph usage from the first frame** — the one frame
   whose `draws` line reads *no frame yet* and is therefore shorter than every frame
   after it. It prints 80 against a 128 budget where the steady state is 83. D-195.

**The class, in the coder's words:**

> **The measurement is taken at a moment or over a set that does not include the case
> it claims to cover, and it reports a true number about the wrong thing.** None of
> them is a wrong answer. Each is a right answer to a question nobody asked.

*A right answer to a question nobody asked* is card 039's own phrase, and it names all
four rather than the one it was written for.

**What makes the class worth having rather than filing as four bugs: every instance
passes its own check.** The build is green, the frame looks perfect, the number
prints, the screenshot is correctly taken. **None would have been caught by being more
careful**, because care is applied to the thing being measured and the defect is in
what the measurement touches. Each is caught only by asking *which sample did this
take*.

**And three of the four were invisible while there was only one of something** — one
folder outside the list, one surface in the frame, one frame before the line grew.
That is why they surfaced together: this was the week the engine acquired a second of
several things.

**The constraint on where this could live.** The tech lead's standing rules say a
convention is only real when it is an enforceable skill an agent can load. **The
skills that would carry it are gitignored and per-machine** (root `CLAUDE.md`: the
repository is the only memory), so a skill does not travel between the principal's
machines and cannot be the record. That leaves the decision records and the engine's
always-on rules.

## Options considered

### Option A — a decision record

Numbered, permanent, with the four instances attached as evidence. Travels with the
repository, and the reasoning survives the sessions that produced it. Nothing
enforces it: it works as far as agents read the records, which they do, at their own
discretion.

### Option B — one line in the engine's always-on rules

`voe3d/CLAUDE.md`, which every coding agent loads before touching anything. The
shortest reach to the people it binds, and it travels. But that file is deliberately
short and holds hard invariants — build commands, module edges, *never commit in
detached HEAD* — things that are either kept or broken. This is guidance, and
guidance in a list of invariants dilutes the list.

### Option C — the decision register only

The class and its instances stay in the tech lead's working file, and the
card-writing half becomes a habit he holds. Cheapest, and honest about being a habit
rather than a rule. But the register is an agent working file the principal does not
read, and a habit recorded only there dies with any session that does not look.

### Option D — both A and B

The record carries the reasoning and the evidence; the rules file carries the one-line
form agents actually load. Two places to keep in step if it is ever revised.

## Decision

**Option A.** The deciding factor: **this is a decision with evidence, and evidence is
what makes it persuasive rather than a slogan** — four named instances, one of them
the tech lead's own, are the reason anybody will follow it, and the only artefact that
holds evidence is a decision record.

Two consequences follow, and they bind different people:

1. **Code that reports on itself says which sample it took**, and prefers the worst
   case or the steady state over the first one to hand. A number whose sample is
   unnamed is not yet a measurement.
2. **A card's verify step names the sample it takes and the case it claims to
   cover.** This one is the tech lead's own and starts immediately: instance 3 was a
   verify bullet, and verify bullets are his to write.

**The scope is deliberately narrow.** This is not a rule about testing in general, not
a coverage policy, and not a requirement that every number be worst-case. It says only
that a measurement which does not name its sample is incomplete, and that the first
sample to hand is the one most likely to be unrepresentative.

## Blast radius

**Reversibility: cheap.** Nothing is built on this and no code depends on it.
Withdrawing it costs a superseding record and leaves no artefact behind, because
everything it asks for is a sentence in a header or a verify bullet — additions that
remain correct even if the rule stops being required.

## Consequences

- **Cards get slightly longer**, in the one section where length has been earning its
  keep. A verify bullet that names its sample is a line or two more than one that does
  not.
- **The one we do not like, stated plainly: nothing checks this.** It is a rule people
  must remember, on a project whose habit is to swap those for rules the machine
  checks — six times, by ADR-0041's count. **No machine check exists for it**: whether
  a sample covers the case a claim is about is a question about intent, and the four
  instances here were all *syntactically* fine. So this record is weaker than the
  project's usual instrument, and it is adopted knowing that.
- **The mitigation is not enforcement but visibility**: the four instances are listed
  above, so the next person meeting a fifth recognises it rather than filing it as an
  unrelated bug.
- **It gives a name to something already happening.** All four were found without the
  rule. What the rule adds is that the fifth gets found *before* it ships rather than
  by the coder who trips over it.

## Rejected options and why

- **Option B, the always-on rules file.** Rejected because it holds invariants, not
  guidance. *Never commit in detached HEAD* is a rule you have either kept or broken;
  *name the sample your measurement took* is a thing you do well or badly. Mixing the
  two teaches readers that the list is advisory, which is expensive for the entries
  that are not.
- **Option C, the register only.** Rejected on the principal's own constraint: the
  repository is the only memory and the register is an agent working file. A class
  worth naming is worth putting where the record of decisions lives.
- **Option D, both.** Rejected as premature: it duplicates a rule that has not yet
  been shown to be ignored. If a fifth instance appears *after* this record exists,
  that is the evidence that reading the records was not enough, and the one-line form
  can be added then with a reason.

## Questions this opens

- **D-195** — `dev`'s readout reports its glyph usage from its first and lightest
  frame. Already open; this record is what decides how it should be repaired when a
  card next touches the readout: name the sample, or take the steady state.
- **No new rows.** The rule is a discipline, not a mechanism, and it introduces
  nothing that needs deciding later.
