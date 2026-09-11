# 043 — `samples` is not only durations

status: review
claimed-by: claude-opus-5-kanban-coder
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: card 040
reported it as blocked work in another folder and takes no number for it; this takes
the next free one. **042 is not free** — it was written for reading a display's scale
and withdrawn with nothing built (ADR-0104), so the number is consumed.

**There is no ADR behind this card and it does not need one.** It changes one sentence
of a comment so that it is true again. Nothing behaves differently and no signature
moves.

## This card is one sentence and must not grow

If you find yourself changing a field, a function, a caller or a test, **stop and
report** — you have found something this card is not about. The whole of it is below.

## What is wrong

`base/include/base/samples.h` says:

> `worst` **IS THE LARGEST VALUE AND THAT PRESUMES A DIRECTION.** Everything measured
> with one of these is a duration, and a longer duration is a worse one; naming it
> `max` would be more general and would say less. A quantity where small is bad does
> not belong in this type.

**The middle sentence was a fact about the callers, not a constraint, and card 040
made it false.** `dev`'s readout now measures the frame's **draw command count** with
a `voe_base_samples` — not a duration, and correctly so: more draw commands is worse,
which is the direction the type actually requires.

**The contract is the last sentence and it is untouched.** *A quantity where small is
bad does not belong in this type* is the real rule, it was always the real rule, and a
draw count satisfies it. The paragraph's own reasoning already points this way —
it says naming the field `max` *"would be more general and would say less"*, which is
an argument about **direction**, not about **unit**.

## Scope — `base`, one comment

- **Correct the stale sentence so the paragraph is true of its callers again.** Keep
  the direction argument and keep the final constraint exactly as they are; what has
  to go is the claim that everything measured is a duration.
- **Name the counter-example in the sentence.** A reader meeting *any quantity where
  larger is worse* will want to know what the second kind is, and *`dev`'s readout
  measures a frame's draw count with one* answers it in a clause. A header that says
  only the general rule invites the next reader to wonder whether anyone actually
  relied on it.
- **Nothing else in the file.** Not the period-not-a-sliding-window paragraph, not the
  empty-run paragraph, not the example.
- **No code, no test, no other folder.** If a compiler or the analyser reacts to this
  card at all, something is wrong with the change.

## Why it is worth a card at all

Because it is in `base`, which every folder depends on, and because **this project's
whole method is that a header is where a reader finds out what is true.** A public
header in the most-depended-on folder carrying a sentence that its own callers
contradict is the cheapest possible kind of wrong and the most expensive kind to leave.

**And because the coder who found it was right not to fix it.** Card 040 says
*nothing outside `dev` changes*; editing `base` from a `dev` card is the module
boundary being crossed quietly, which is the thing cards exist to prevent. He reported
it as blocked and it became this. **That is the mechanism working, and it is worth one
small card to keep it working.**

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean — **and
  nothing in the output differs from the run before your change**, which is the real
  check for a card like this.
- Read the paragraph back once, whole, and satisfy yourself it is true of every caller
  `samples.h` now has. **Say in your report how many callers there are and what each
  measures** — that is the sample this card's own claim is about, and ADR-0105 asks
  you to name it rather than assume it.

## Report when this lands

- The sentence as it now reads, quoted.
- The callers you found and what each one measures.
- Confirmation that nothing else changed.

## Notes — coder, 2026-09-11, Linux (WSL2, lavapipe)

**The paragraph as it now reads:**

> `worst` IS THE LARGEST VALUE AND THAT PRESUMES A DIRECTION. Most things
> measured with one of these are durations, and a longer duration is a worse
> one, but it is the direction that matters and not the unit: `dev`'s readout
> measures a frame's draw command count with one, and more draw commands is
> worse. Naming it `max` would be more general and would say less. A quantity
> where small is bad does not belong in this type.

The direction argument and the final constraint are carried over word for word.

**Callers: six declared `voe_base_samples`, five of them live.** All five live ones
are `struct timing` in `dev/src/main.c`:

| Caller | Measures | Unit |
|---|---|---|
| `timing.frame` | one top of the loop to the next | seconds |
| `timing.update` | poll, input and the three systems | seconds |
| `timing.draw` | the draw phase, `_begin` to `_end` inclusive | seconds |
| `timing.gpu` | the graphics card's own clock for that frame | seconds |
| `timing.draws` | draw commands in the completed frame | a count |

The sixth is the local in `base/tests/samples.c`, which adds bare numbers and
measures nothing. So one caller in five is the counter-example the sentence now
names, and the paragraph is true of all of them.

**Verified:** `cmake -P check.cmake` exits zero — 13 standalone configures, root
build, the four guards, includes, 39 tests, analyser over 106 files, both
negative controls. Run twice, once with the new sentence and once with the old
one restored, and the two outputs are **byte-identical**, which is what this card
asked for. Linux only; nothing here is platform-dependent, and no Windows check
is owed because no code changed.

**Nothing else changed.** `git diff --ignore-cr-at-eol --stat` over the whole tree
names `base/include/base/samples.h` and this card and nothing else.

### Suggestions, not done

- `voe_base_samples_add`'s own comment in the same header still says *"every
  quantity this type is for is a duration, and a duration that ran backwards means
  the clock was read in the wrong order somewhere upstream"* — the same stale claim
  as the one this card fixed, in the justification for its negative-value assert.
  The assert itself is right for a count too. Left alone: the card is one sentence
  and says so.
- `dev/src/main.c` around line 1707 says *"See base/include/base/samples.h, whose
  header still says everything measured with one is a duration"*. That sentence is
  now false. `dev` is another folder and the card forbids it; the tree builds
  either way, so ADR-0113 does not cover it.
