# 0145. This repository is a development tree, and field descriptions are on throughout it

- **Status:** Accepted
- **Date:** 2026-09-12
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Decides bug report 004**, and it is the second half of the principal's sentence that
ADR-0144 took the first half of: *"the editor project should not be baked in the finished
game."*

The report's finding, from the coder who filed it: the editor expands nothing unless it was
built with the `editor` preset, because `VOE_BASE_DESCRIPTIONS` is a build-wide `-D` that
decides what `scene/src/transform_system.c` and `identity_system.c` hand back at
registration. No code is at fault in any folder — a plain `Debug` build with that one flag
added inspects correctly, which the reporter proved and then reverted. The principal's
requirement, in his words:

> *"Editor should work the same way in debug mode as in release. Or else we cant debug."*

**What changed the answer was going to look for the gate.** `voe3d/CMakeLists.txt:17` is a
bare `add_subdirectory(editor)`, so the obvious remedy is to put the editor behind a build
option. That remedy collides with a guard that exists on purpose: **`check.cmake` step 1b
reads the root file with a regex and refuses control flow in it**, because a folder that
looked listed and was not once passed every step of the script while being absent from the
build — *"the worst shape a failure can have: not a wrong answer, but a right answer to a
question nobody asked."* Its own comment says the root's hand-written list is deliberate
and is not the thing to fix.

**An interim fix went in while this was being written**, on the principal's explicit
instruction — *"turn on descriptions in debug preset so i can review 059"* — because he was
reviewing card 059 and could not see the thing he was reviewing. That is the report's
remedy 1, the `debug` preset gaining the flag, and the coder recorded it in the report
rather than quietly. It is superseded by the decision below and the card undoes it. **It
also produced the most useful sentence in the report**: *"it configures its own build
directories and never reads a preset, so its runs still build with descriptions off."*

**And looking at that made the premise wrong rather than the remedy.** The separation the
principal is asking for is real and it is not between two presets here. This repository
builds the engine libraries, `dev` and `editor`; ADR-0121 adds `dev_editor`. **None of
those is a shipped game.** Under ADR-0052 a game is built from its own tree, by the cook,
against the engine's libraries, and under ADR-0144 what the cook hands that tree is
generated C. That tree never lists the editor. The boundary already exists and needs
nothing built to enforce it.

Constraints already fixed. ADR-0128 point 3: descriptions are off by default and the switch
belongs to whoever is building — untouched by this and quoted rather than changed.
`base/include/base/describe.h`: the switch is read once per translation unit, everything it
emits is static, and files built with it and without it link together. `voe3d/CLAUDE.md`
rule 1: a folder's `CMakeLists.txt` is four lines and nothing else.

## Options considered

### Option A — this root is a development tree and descriptions are on throughout it
Delete the `editor` preset. `debug` and `release` both define `VOE_BASE_DESCRIPTIONS=1`.
The editor inspects identically in either. The switch is unchanged; a game's tree simply
never defines it and carries nothing.

### Option B — the editor leaves the tree behind a build option
`VOE_EDITOR` gates `add_subdirectory(editor)` and implies the descriptions; presets split
into game trees and editor trees.

### Option C — descriptions on for `debug`, off for `release`
The smallest possible edit, and the report already names its failure: `release` still shows
nothing, so the editor is still two programs.

## Decision

**Option A**, the principal's call and the tech lead's recommendation.

1. **This repository is a development tree.** Everything built from this root is a tool
   (`editor`), a guarantee (`dev`, which proves the engine is usable without the editor —
   ADR-0121 point 4), a library, or a test. **No shipped game is ever built from here.**
2. **`VOE_BASE_DESCRIPTIONS=1` is set for every preset in this root.** The `editor` preset
   is deleted. `debug` and `release` remain and differ only by optimisation, as their names
   claim.
3. **The switch itself does not change**, and neither does ADR-0128 point 3. A game's build
   tree does not define it and gets no table and no string. **The cost this switch exists to
   avoid was never charged in this repository**, which is the whole of the decision.
4. **The editor not being in a finished game is enforced by the game's tree, not by a
   preset here.** It follows from ADR-0052's three builds and ADR-0144's generated C: the
   cook's output names the engine's libraries and never the editor. Nothing is added to
   guard it.
5. **The descriptions-off path keeps its coverage in the tests and not by accident.** With
   point 2 there is no build in this repository that compiles it, so
   `scene/tests/transform.c` and `identity.c`, which today follow the build through a
   `BUILD_DESCRIBES` constant, must prove both answers explicitly — the way
   `base/tests/describe.c` already turns the switch on for itself whatever the build said.

## Blast radius

**Cheap.** Two lines of `CMakePresets.json` and a test that names both answers. Reversing it
is putting the flag back on one preset.

The load-bearing half is point 1, and it is a claim about what this repository is rather
than a setting. If a shipped game is ever built from this root — a release preset pressed
into service as a ship configuration because it was there — then point 3's reasoning is
void and every binary it produces carries the field names. **The defence is that a game is
built by the cook from its own tree**, which is ADR-0052 and ADR-0144, and not a check.
Reversibility: **cheap for the mechanism, load-bearing for the claim.**

## Consequences

- **The editor is one program with one behaviour**, which is the requirement and the whole
  of the report. A reviewer reaches for `debug`, which is what a reviewer does, and the
  inspector works.
- **`release` here stops being byte-comparable to a shipping configuration.** It is bigger
  by a table per translation unit and the field names as strings. Speed is unaffected —
  the tables are static data nothing reads unless it walks them — but a size measurement
  taken from `release` in this repository is now measuring the wrong binary, and anyone
  who takes one should know that.
- **Nothing in this repository compiles the descriptions-off path any more.** That is a
  real loss of coverage and point 5 is what pays for it. If point 5 is skipped, the first
  person to find out that the `#else return NULL` branch stopped compiling is a game
  developer, and this ADR is why.
- **Three presets become two.** `editor/editor.md`'s build section names the `editor`
  preset and changes with the card; `voe3d/CLAUDE.md` names only `--preset debug` and stays
  correct as it is.
- **The switch moves from a preset to `voe_target_settings`**, which is where
  `cmake/voe.cmake` keeps *the one flag set … applied identically to a folder's library and
  to its test executables*. That is what makes it a property of the repository rather than
  of how somebody configured it — and it is also what fixes a second hole the report did not
  reach: `check.cmake` configures its own root tree with `-DCMAKE_BUILD_TYPE=Debug` and no
  flags, so a switch living in a preset would leave **the one gate this project has building
  a configuration nobody runs.** Same shape as the reported bug, pointed the other way.
- **Bug report 004 becomes work and is archived** in the same act, under ADR-0110.

## Rejected options and why

**B — the editor behind a build option.** Rejected on what it costs to buy. It needs either
control flow in the root `CMakeLists.txt`, which `check.cmake` step 1b refuses with a
written reason and a real incident behind it, or an opt-out inside `editor/CMakeLists.txt`,
which breaks the four-line folder rule. Two rules bent to enforce something that is already
true: no shipped game is built from this root. It is the right answer to a question this
repository does not have.

**C — on for `debug`, off for `release`.** Rejected by the report itself. It leaves the
editor as two programs and only moves which preset is the trap.

**Note on the fourth remedy the report killed.** A compile definition on the `voe_editor`
target cannot work, because what the world is told is decided inside `voe_scene`, compiled
once for the whole build. The reporter put that option to the principal before checking it
and then wrote down why it was wrong. It is recorded here so nobody proposes it a third
time.

## Questions this opens

None. The follow-on work is card 063.
