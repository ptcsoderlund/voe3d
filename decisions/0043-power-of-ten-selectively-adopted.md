# 0043. *Power of Ten* is a named influence, adopted in part and rejected in part

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal asked what this project should take from NASA/JPL's *The Power of
Ten — Rules for Developing Safety-Critical Code* (Holzmann, 2006).

**Four of the ten are already in force here**, one nearly word for word, which
is the reason the question is worth an ADR rather than a note: an influence
already shaping the codebase should be named, so that future arguments about it
start from a written position rather than from taste.

| Rule | Where it already lives |
|---|---|
| **9a.** No more than one level of dereferencing | `coding_convention.md` and `CLAUDE.md` rule 6 — *"`my_var**` is not allowed"* |
| **4.** Functions short enough to print on one page | `coding_convention.md` — *"Keep functions small with asserts"* |
| **5.** Assertions used to check conditions that should never happen | Same line, and `base/assert.h` |
| **10a.** Maximum warnings, zero tolerated | `-Wall -Wextra -Wpedantic -Werror` on every target (ADR-0027) |
| **3.** No dynamic allocation after initialisation | ADR-0032 **in spirit** — arenas, bump allocation, nothing freed individually |

The danger with a rule set this famous is adopting it wholesale. It is
calibrated for code where a defect loses a spacecraft and there is no second
attempt; some of the cost it accepts would make a 3D engine worse. This ADR
therefore records the **rejections** as firmly as the adoptions, so that neither
side gets relitigated by whoever reads the paper next.

## Decision

**The rules that bind, beyond what already binds:**

- **Rule 7 — every fallible return is checked.** Adopted, and made mechanical
  rather than cultural: **ADR-0041** puts `[[nodiscard]]` on every function that
  can fail, so ignoring one is a build failure under the existing `-Werror`.
  Rule 7's second half — *each called function must check the validity of all
  parameters* — is already the assert convention, and ADR-0041 pins the split:
  **bad parameters are asserts (the program is wrong); failures are returned
  values (the world is uncooperative).** Holzmann draws the same line.
- **Rule 10b — a static analyser, every run, zero warnings.** Adopted by
  **ADR-0042** as `check.cmake` step 7, using `clang --analyze` because it needs
  no tool that is not already required.
- **Rules 1 and 2 — bounded loops, no unbounded recursion — in `assets` only.**
  ADR-0023 commits us to writing the glTF and JSON readers ourselves, and
  recursive descent is the natural way to write a JSON parser. On input read
  from a file it is also a stack overflow on deep nesting — a real crash on a
  corrupt or hostile asset, not a theoretical one, and one no amount of
  `-Werror` or analysis will find. **Binding on `assets`: an explicit stack, an
  explicit nesting limit, and the file refused past it.** The limit is a named
  constant with the reason in the file header.

**The rules that do not bind, and why, so this is not reopened:**

- **Rule 3 literally — no allocation after initialisation.** Rejected. An engine
  that loads models at runtime cannot obey it. Its actual payoff is already
  held: ADR-0032 makes lifetimes predictable and allocation failure fatal, so
  the fragmentation and use-after-free that rule 3 exists to prevent do not
  arise.
- **Rule 1 elsewhere — no recursion anywhere.** Rejected outside `assets`.
  Recursion over data we constructed and whose depth we know is fine.
- **Rule 2 elsewhere — every loop provably bounded.** Rejected. The frame loop
  is deliberately unbounded, an exception the paper itself grants a scheduler.
- **Rule 5's density target — two assertions per function.** Rejected as a
  number. A quota produces assertions written to meet the quota. The practice
  stays; the metric does not.
- **Rule 8 — restricted preprocessor.** Not adopted as a rule because it is
  already the practice by other routes: ADR-0026 and ADR-0037 removed platform
  `#ifdef` from shared code in favour of `*_wayland.c` / `*_win32.c` file
  naming, and ADR-0038 forbids OS `#ifdef` in the dev project.

**The rule this engine structurally cannot take:**

- **Rule 9b — no function pointers.** **Direct, unavoidable conflict.**
  ADR-0040, decided the same day, makes *every* Vulkan call go through a
  resolved function pointer: there are no symbols to link, and the entire
  no-SDK property depends on it. The conflict is recorded rather than
  papered over.

  **The mitigation is confinement, and it is binding on `render`:** one table,
  resolved once at startup, never reassigned afterwards, never passed as a
  parameter, never stored in a component. That is a different thing from
  function pointers used as dispatch — which ADR-0008 already removed by
  deleting the registry and vtable case, and which ADR-0007 keeps rare by making
  data-with-functions-over-it the default. Outside `render`'s loader table,
  rule 9b effectively holds.

## Blast radius

**Reversibility: cheap for the adoptions, moderate for one.** `[[nodiscard]]`
and step 7 are each removable in one edit. The `assets` parsing rule is cheap
while unwritten and expensive after: converting a recursive parser to an
explicit stack later is a rewrite of the parser, which is why it is recorded
before the card exists rather than after.

## Consequences

- **A named influence with a written boundary.** The next person to read the
  paper and propose banning recursion or allocation gets an answer instead of an
  argument.
- **`assets` cards inherit a constraint before they are written.** Any card for
  the glTF or JSON reader must state the nesting limit, and a card that does not
  is under-specified.
- **`render`'s loader table is a documented exception**, not an oversight. Its
  file header must say so, since ADR-0003 puts the reasoning in the file.
- **We are not a safety-critical project and do not claim to be.** Adopting four
  more rules from a safety-critical standard does not make this engine one, and
  nothing here should be read as a safety claim.
- **The three adoptions all cash out as machine checks**, not as things a
  reviewer must notice — `[[nodiscard]]` under `-Werror`, step 7 in
  `check.cmake`, a constant in the parser. That is the filter that selected
  them, and it is the filter to apply to anything else proposed from the paper.

## Rejected options and why

- **Adopt all ten** — rejected. Rule 3 and rule 9b are incompatible with
  decisions already taken (ADR-0032, ADR-0040), and rules 1 and 2 in full would
  cost more than they return in a renderer.
- **Adopt none; treat it as background reading** — rejected. Four rules already
  bind this codebase without being attributed anywhere, and rule 7's mechanical
  form is a genuine improvement available for one attribute.
- **Record it in `guidelines.md` rather than an ADR** — rejected. `guidelines.md`
  is the human's document and states practice; this is a decision with rejected
  alternatives, which is what an ADR is for. The binding parts reach coders
  through `CLAUDE.md`, per ADR-0003.

## Questions this opens

- **D-047 (new)** — the nesting limit `assets` refuses past, as a number.
  Decided by the first parser card against a real glTF file, not here.
- Nothing else. The adoptions land in ADR-0041 and ADR-0042; this ADR is the
  record of the selection and its boundary.
