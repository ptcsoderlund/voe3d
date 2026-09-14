# 0045. Step 7 reads `compile_commands.json`, runs the default checkers, and suppresses with `[[clang::suppress]]`

- **Status:** Accepted
- **Date:** 2026-08-31
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0042 decided *that* `clang --analyze` is step 7 of `check.cmake`, with zero
warnings, no baseline, and suppression at the site rather than globally. It
deliberately left the mechanics open — **D-046** — to be settled "against real
`base` and `platform` code" rather than by taste. That code now exists:
`base`, `math`, `platform`, `render`, `dev` and their tests are all written and
committed.

**The rule currently lies.** `voe3d/CLAUDE.md` rule 8 says the check run
"includes step 6 (tests) and step 7 (`clang --analyze`)". `check.cmake` stops at
step 6b. Card 007 ran the analyser by hand and said so in its report. Every card
from here on would claim analysis it did not receive. That is what makes this
decision due now rather than later.

### Evidence, measured rather than argued

Run on the development machine, 2026-08-31, clang 22.1.8, every translation unit
in the tree, using each unit's real compile flags:

- **Default checkers report exactly one warning across the whole tree.**
- **It is a true positive**, in `render/tests/loader.c:48` —
  `core.CallAndMessage`, a call through a possibly-null function pointer. Line 47
  checks the pointer with `VOE_TEST_CHECK`; line 48 calls it. ADR-0031's harness
  header states its own contract: *"A check never stops the program."* So on a
  machine where the pointer is null, the test dereferences null instead of
  reporting a failure. The analyser found a real defect in the seam between two
  of this project's own decisions, which is precisely the class of bug ADR-0042
  argued warnings cannot see.
- **ADR-0042's predicted false positive did not occur.** That ADR expected
  `unix.Malloc` to complain about ADR-0032's arena, which never frees
  individually, and expected `base` to be "read more harshly than anything
  else". `base` is clean. The concern was reasonable and is now retired.
- **`alpha.core`, `alpha.security` and `optin.portability.UnixAPI` together add
  zero findings** on this tree.
- **`[[clang::suppress]]` works in C23** on this compiler, compiles clean under
  `-Wall -Wextra -Wpedantic -Werror`, and suppresses the report while leaving the
  code compiled. It must sit on the enclosing block or function, not on the
  offending line: a leak report anchors at end of scope, and an attribute on the
  statement that allocated does not catch it.
- **`#ifndef __clang_analyzer__` also silences a finding** — by deleting the
  code from the analyser's view entirely.

The binding constraints: no new required tool (ADR-0021, and ADR-0042 rejected
`clang-tidy` on exactly this ground); `check.cmake` is `cmake -P` script mode
with no project and no target knowledge (ADR-0028); both platforms, GNU driver
(ADR-0026); **Clang floor 18 with no ceiling and no CI** (ADR-0005, ADR-0004),
so contributors run different compiler versions and nothing central reconciles
them.

## Options considered

### Where step 7 learns each file's flags

Script mode knows nothing about `-std=c23`, `render`'s vendored Vulkan include
path, or `platform`'s generated Wayland headers, so it must be told.

**Option A — read `compile_commands.json`.** ADR-0044 exports it beside the root
`CMakeLists.txt`, and step 3 of the check script has already configured and
built by the time step 7 runs, so the file is fresh by construction. Step 7
re-issues each entry with `--analyze`, dropping the output and dependency flags.
Costs a modest amount of command-line rewriting in CMake script mode.

**Option B — an `analyze` target emitted by `voe_module()`.** A per-source
custom command carrying the build's own flags; step 7 becomes
`cmake --build … --target analyze`. Flags are correct by construction with no
parsing, and Ninja parallelises it. Costs machinery in `cmake/voe.cmake`, the one
file ADR-0027 exists to keep at four lines per folder.

**Option C — restate the flags inside `check.cmake`.** A second copy of
ADR-0027's flag set, drifting silently from the first, and structurally unable to
know about generated headers.

### Which checkers

**Defaults only**, or **defaults plus named `alpha` checkers**.

### How a suppression is spelled

**`[[clang::suppress]]`**, **`#ifndef __clang_analyzer__`**, or **defer the
spelling** until a false positive actually appears.

## Decision

**Option A, default checkers, `[[clang::suppress]]`.**

**Flags come from `compile_commands.json`** (ADR-0044). Deciding factor: the file
is exported for editors regardless, and step 3 refreshes it before step 7 reads
it, so the mechanism costs one consumer of a file that already exists rather than
new machinery in the project's most load-bearing CMake function.

*The tech lead first recommended Option B, on the argument that a stale database
would let step 7 analyse an old tree and still exit zero. The principal pointed
out that `compile_commands.json` is wanted anyway for `clangd` and CLion — which
both removes its cost from this decision and, with step 3 building first, answers
the staleness objection. The recommendation was changed before the decision, not
overruled after it.*

**Default checkers only.** Deciding factor: `alpha` checkers are explicitly
unstable and their findings change between LLVM releases. With a floor of Clang
18, no ceiling, no CI, and ADR-0042's zero-tolerance rule, an unstable checker
set means a contributor's `check.cmake` can fail on code that passed on the
principal's machine, for no reason either of them can see. The default set is the
only part of the analyser that behaves like a contract. It also costs nothing to
refuse them: they found nothing here today.

**`[[clang::suppress]]`, on the enclosing block or function, with a comment
giving the reason.** Deciding factor: it suppresses the *report* and leaves the
code compiled, whereas `#ifndef __clang_analyzer__` makes the analyser reason
about a program we do not ship — the one outcome a static analyser must never be
configured into.

**The spelling is decided now even though nothing needs suppressing yet.** This
is in tension with ADR-0034's ban on speculative decisions, and the tension is
acknowledged rather than waved away: ADR-0034 permits an early decision where it
changes what code looks like, and this one does not. It is taken anyway because
the failure mode of deferring is specific and silent — a coder meeting a false
positive mid-card, with a policy that demands a site suppression and no spelling
for one, reaches for the widely-published `__clang_analyzer__` recipe, which is
the wrong answer and looks like the right one.

**`render/tests/loader.c:48` is a fix, not a suppression.** The check must return
early rather than continue into the call. This is the first application of
ADR-0042's rule that correct code is never changed to appease the analyser — and
its converse, that incorrect code is not suppressed to preserve a green run.

## Blast radius

**Reversibility: cheap**, in three separable pieces.

- The flag source can change to Option B later without touching a line of engine
  code; it is entirely inside `check.cmake` and `cmake/voe.cmake`.
- The checker set is one argument. Widening it later is additive; the cost is
  paid once, when the new findings are cleared.
- The suppression spelling is the only piece with reach, because each use is a
  mark in a source file. Changing it later is a mechanical grep across however
  many sites exist. There are zero today, which is the cheapest moment this
  choice will ever be made.

The expensive direction, avoided: a tolerated warning count or a baseline file.
ADR-0042 already rejected it and nothing here reopens it.

## Consequences

- **`check.cmake` and `CLAUDE.md` rule 8 stop disagreeing.** Today the rule
  claims a step that does not exist; after the implementing card it does.
- **Step 7 depends on ADR-0044.** If the export is ever turned off, step 7 breaks
  rather than silently analysing nothing — it must fail loudly when the database
  is absent, not skip. A skip here would be a green run that checked no code,
  and `check.cmake` already distinguishes a skip from a pass by design.
- **The analyser runs over tests too**, because they are in the compilation
  database like everything else. This is deliberate and consistent with
  ADR-0027's refusal to compile tests with weaker flags — and it is where the
  only finding in the tree actually is.
- **The check script gets slower**, roughly a second per translation unit, on a
  script the human already waits on. At today's size this is seconds; at forty
  folders it is the first thing anyone will want to parallelise, which is the
  point at which Option B becomes attractive again on its own merits.
- **A contributor on a much newer Clang may see findings the principal does not**,
  even with default checkers — the default set itself grows between releases.
  Choosing defaults minimises this; it does not eliminate it. The honest answer
  when it happens is to fix the finding, because a default-checker finding on a
  newer analyser is usually a real one.
- **Nothing enforces that a suppression carries its reason.** The comment is a
  convention a reviewer checks, not something the script can see. Making it
  mechanical would need a grep step and is not worth one today.

## Rejected options and why

- **An `analyze` target in `voe_module()` (B)** — rejected on cost, not on
  quality; it is the more robust construction and stays the right answer if the
  tree ever outgrows a serial script. It lost because ADR-0044 already puts the
  database in the tree for editors, which erased Option A's only real
  disadvantage.
- **Restating flags in `check.cmake` (C)** — rejected outright. Two copies of one
  flag set is the two-sources-of-truth failure ADR-0003 exists to prevent, and it
  cannot see generated headers at all, so it would not even work.
- **`alpha` checkers** — rejected on version stability, not on value. They found
  nothing on this tree today, so adopting them would have bought a
  version-dependent failure mode in exchange for no demonstrated finding. If a
  specific alpha checker ever earns its place it is a one-argument amendment.
- **`#ifndef __clang_analyzer__`** — rejected because it removes code from
  analysis rather than suppressing a report. It is the most commonly published
  answer, which is precisely why it is named and refused here rather than left
  unmentioned.
- **Deferring the suppression spelling** — rejected against ADR-0034's grain, for
  the reason stated in the decision: the cost of deferring is a coder silently
  adopting the rejected spelling under card pressure.

## Questions this opens

- **D-049 (new)** — `render/tests/loader.c:48` calls a function pointer after a
  non-fatal check that it is not null. ADR-0031's harness never stops on a failed
  check, so a check that guards a dereference needs something the harness does not
  have today: either an early return at the site, or a stopping check macro
  beside `VOE_TEST_CHECK`. **This is a question about the test harness, not about
  one file** — every test that checks a pointer before using it has the same
  shape.
- **D-050 (new)** — whether step 7 analyses the `debug` build, the `release`
  build, or both. The analyser follows `#if` branches, and ADR-0032's asserts
  compile out in release (ADR-0043), so the two builds are genuinely different
  programs and a release-only path is currently analysed by nothing.
