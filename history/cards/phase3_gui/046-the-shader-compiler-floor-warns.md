# 046 — the shader compiler's floor warns instead of failing

status: review
claimed-by: claude-opus-5-kanban-coder
blocked-by: card 045 being reviewed and in `complete/` — this edits the lines it added
folder: root — `check.cmake` only. **No engine code changes in this card.**
decided-by: **ADR-0117**

Written by the tech lead. **Not a spin-off** — it is the next number. Card 045 is
correct as it was specified; the specification changed after it was built, by the
principal, on 2026-09-11.

## Why

`check.cmake` now floors `slangc` at `2026.13.1` and calls `step_fail` below it. The
principal's Windows machine is below that floor, and he is the only person who tests
Windows. A check that refuses to complete on the sole machine able to verify the
Windows half of every card gates the testing, not the risk. **ADR-0117** turns that
failure into a warning and spends the difference on making the warning unmissable.

The floor value, and the reason it is the *newer* of the two machines rather than the
older, are not in question and are recorded in **ADR-0112**.

## What to do

All of it in `check.cmake`. The floor comparison is around line 135.

1. **Replace the floor's `step_fail` with a warning that does not stop the run.** The
   check exits zero and every later step runs. Set a variable holding the warning text
   instead of failing; step 1 continues to `step_ok`.

2. **Print it twice.** Once where it is found, in step 1, so it sits beside the tools
   line; and again as **the last thing `check.cmake` prints**, after the final
   `step_ok` at the end of the file, which today has no closing block at all — add one.
   A warning that appears only at the top is a warning that is read once, and the
   remaining steps produce enough output to scroll it away. Say in a comment that the
   repeat is the point and not an accident.

3. **The text names both numbers and the consequence.** The version found, the floor,
   and that this build's shaders are compiled by a version nothing in this engine has
   been verified under — so a fault seen on this machine is suspect until the compiler
   is current. Follow `step_fail`'s indentation convention for multi-line detail rather
   than inventing a second one.

4. **Make the warning provable by hand, following the pattern already there.**
   `VOE_CHECK_FAKE_CLANG_VERSION` exists so the clang guard can be seen to fire; add
   `VOE_CHECK_FAKE_SLANGC_VERSION` beside the floor comparison, read the same way, so
   the warning can be produced on a machine that is above the floor. Document it in the
   same place the existing fake is documented.

   **Do not add a `guard` step for it.** The existing guard steps prove guards in
   `cmake/voe.cmake` by configuring a sub-build; this floor lives in `check.cmake`
   itself, so an automated proof would mean re-running the whole check inside itself.
   The knob plus the Verify step below is the proof.

5. **Leave every other `slangc` failure exactly as it is.** Not found, could not be
   run, exited non-zero, version could not be parsed — all still `step_fail`. *Old* is
   a warning; *unidentifiable* is not.

## What is out of scope, stated so you do not do it

- **No change to the floor's value**, to the parse, to the `find_program(VOE_SLANGC …)`
  resolution, or to the printing of the path when it differs from `PATH`. Card 045 got
  those right and they are what makes the warning worth anything.
- **No override flag.** `-DVOE_ALLOW_OLD_SLANGC` and anything like it was considered and
  rejected in ADR-0117; a warning that is always printed is the mechanism chosen instead.
- **No change to `wayland-scanner`**, to `cmake/voe.cmake`, to any `.slang` file, or to
  anything under `render/`.
- **No edit to card 045.** It is in review and its history stands.

## Verify

`cmake -P check.cmake` exits **zero** on your machine and prints its ordinary output,
step 1's tools line still carrying a version for every tool it names.

Then prove the warning, which is this card's whole deliverable:

```
cmake -P check.cmake -DVOE_CHECK_FAKE_SLANGC_VERSION=2024.17
```

— or whatever invocation the knob needs. Confirm all three things, and paste the first
and last ten lines of that run into the card's notes:

1. The warning appears in step 1, naming `2024.17` and `2026.13.1`.
2. **It appears again as the final line of the output**, after the last step.
3. The check **still exits zero** and every later step ran.

Then confirm an unparseable version still fails, so point 5 is not theoretical.

## The principal's half

Nothing to collect. What he gets is a check that keeps passing on his Windows machine
and tells him the truth about his compiler on every run. **It must not be tightened
into a failure by a later card without a decision that supersedes ADR-0117** — the
failure was the previous behaviour and it was changed on purpose.

## Notes — coder, 2026-09-11, Linux (WSL2, lavapipe, scratch toolchain)

**The ordinary run is unchanged.** `cmake -P check.cmake` exits zero, 52 s, and its
output is **byte-identical to the run before this card** — this machine is 2026.17,
above the floor, so no warning fires and nothing else moved.

### The warning, which is the deliverable

`cmake -DVOE_CHECK_FAKE_SLANGC_VERSION=2024.17 -P check.cmake` — **exit 0**, 23 steps
`ok`, two `WARN` blocks.

First ten lines:

```
ok    tools (clang 21, cmake 4.2.3, slangc 2024.17, wayland-scanner 1.24.0)
WARN  tools
      slangc 2024.17 is below 2026.13.1, which is the oldest version any shader in this engine has been verified under.
      Every .slang file in this build was compiled by this slangc and its SPIR-V is embedded in the binary, so a rendering fault seen on this machine is suspect until the compiler is current.
      The check has not failed and will not fail on this (ADR-0117).
ok    folders in the root build (12: 3d assets base dev ecs math platform render scene sprite text ui)
ok    standalone 3d
ok    standalone assets
ok    standalone base
ok    standalone dev
```

Last ten lines:

```
ok    guard map
ok    includes
ok    tests (39 passed — 3d 6, assets 6, base 2, dev 0, ecs 3, math 5, platform 2, render 6, scene 3, sprite 1, text 3, ui 2)
ok    harness reports a failure
ok    analyser (106 files)
ok    analyser reports a finding
WARN  tools
      slangc 2024.17 is below 2026.13.1, which is the oldest version any shader in this engine has been verified under.
      Every .slang file in this build was compiled by this slangc and its SPIR-V is embedded in the binary, so a rendering fault seen on this machine is suspect until the compiler is current.
      The check has not failed and will not fail on this (ADR-0117).
```

All three things the card asked for: it names `2024.17` and `2026.13.1` in step 1, it
is the **final** output of the run, and the check exits zero with every later step run.

**The knob goes before `-P`, not after.** The card's Verify block writes
`cmake -P check.cmake -DVOE_CHECK_FAKE_SLANGC_VERSION=2024.17`; in script mode cmake
treats anything after `-P` as a script argument, so that form leaves the variable
undefined and the probe silently does nothing. The card allowed for this
(*"or whatever invocation the knob needs"*). The working form is the one above, and
it is written into `check.cmake`'s header beside the knob so the next person does not
have to rediscover it.

**Proved with a real old compiler too, no knob involved:** `VOE_SLANGC` pointed at a
script reporting `2024.17-1-g839bc9aa` — the principal's exact string — warns with the
git tail intact, twice, exit 0.

### Point 5 is not theoretical

Every other `slangc` condition still `step_fail`s and still exits 1:

| Probe | Result |
|---|---|
| version unparseable (`I am not a shader compiler`) | `FAIL tools — could not parse a version out of: …`, exit 1 |
| `VOE_SLANGC` at a file that does not exist | `FAIL tools — slangc could not be run: no such file or directory`, exit 1 |
| `slangc -v` exits 3 | `FAIL tools — slangc -v exited 3: 2026.17`, exit 1 |

*Old* warns; *unidentifiable* fails.

### What the change is

- `step_warn()` beside `step_ok`/`step_skip`/`step_fail`, using `step_fail`'s own
  six-space indent for multi-line detail rather than a second convention.
- The floor sets `slangc_warning` instead of failing; empty means no warning.
- Step 1 prints it right after its `step_ok`, so it sits beside the tools line.
- A **closing block** at the foot of the file — there was none — prints it again and is
  the only thing after the final step. The comment says the repeat is the point.
- `VOE_CHECK_FAKE_SLANGC_VERSION` at the floor, documented in `check.cmake`'s header
  block with the invocation. No `guard` step for it, as instructed.

**Nothing else changed.** `git diff --ignore-cr-at-eol --stat` names `check.cmake` and
nothing else — no `wayland-scanner` change, no `cmake/voe.cmake`, no `.slang`, nothing
under `render/`, and card 045 untouched. The floor value, the parse, the
`find_program(VOE_SLANGC …)` resolution and the path printing are all as card 045 left
them.

### One reading taken, and one thing the probes confirmed

**Reading:** *"Document it in the same place the existing fake is documented"* — the
clang fake is documented in `cmake/voe.cmake`'s own header block and again at its site.
That file is out of scope here, so this knob is documented in the two analogous places
in `check.cmake`: its header block and the line above the hook. Same pattern, this
file.

**Confirmed, previously reported:** pointing `VOE_SLANGC` at a script that is not a
compiler at all made the *whole check pass* — because `check.cmake`'s own
sub-configures go through `${common}` and never receive the override, so they built
every shader with the real `slangc` on `PATH` while step 1 reported the fake. That is
the gap reported in card 045's notes, now seen rather than reasoned about. Still not
closed here: it is line 30, not step 1, and it is the tech lead's decision.
