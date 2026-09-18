# 0151. The scene reader and writer live in `authoring`, a folder a game does not build

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** —. Amends ADR-0022's module map by one folder
- **Superseded by:** —

## Context

**Closes D-260**, opened by ADR-0149: *where the scene reader and writer live.*

The reader takes the text `assets`' sectioned parser returns, reads it through the world's
component descriptions into `ecs` tables and patches references (ADR-0125 point 6). The writer
walks the world through the same descriptions and emits ADR-0149's canonical text. **No folder
today can see both sides**: `scene` depends on `ecs`, `math`, `base`; `assets` on `platform`,
`math`, `base`; `3d` on both, but `3d` is a renderer and a scene file is not a picture.

**The fact that settles most of it.** Both halves work only where descriptions are compiled in
(`VOE_BASE_DESCRIPTIONS`, ADR-0127). A game's own build has them off by default (ADR-0128 point
3), and under ADR-0144 a shipped game never reads a scene file — the cook turns it into C. So
this is authoring-time code, and its callers are the editor now and the cook next arc, which
D-251 already requires to run without the editor.

## Options considered

### Option A — a new folder, `authoring`, above `scene`, `ecs` and `assets`
The editor and the cook link it; a game's build does not, unless a developer turns descriptions
on to serialise components themselves, which ADR-0128 point 5 allows. Its own page, public
header and round-trip tests. Costs a folder in the map and a row in `cmake/voe.cmake`.

### Option B — `scene` gains `assets`
No new folder, and `scene` owns identity, which is what *authored* means. Every game links a
`scene` carrying a reader that compiles to nothing in its own build, and `scene` gains `platform`
transitively through `assets`.

### Option C — in `editor/` now, moved when the cook arrives
Cheapest today. The cook is already decided, so it is building in the known-wrong place, and
the code that most needs strict tests would start in a folder that has none.

## Decision

**Option A**, the tech lead's recommendation, the principal's call.

> *"authoring, go ahead"*

1. **`authoring` is a folder in this repository** with the ordinary four-line `CMakeLists.txt`,
   `include/authoring/`, `src/`, `tests/` and a folder page.
2. **Its row is `scene`, `ecs`, `assets`, `math`, `base`.** Nothing below it may name it.
3. **`editor`'s row gains `authoring`, and `3d`.** The cook, when it exists, is `authoring`'s
   second caller (D-251). **`3d` is a correction, not part of this choice**: ADR-0147's scene
   views draw the world through `3d`'s draw system and its mesh and material components, and
   `cmake/voe.cmake`'s editor row — which says *a card that wants one of them is a decision, not
   an edit here* — was never given it. Card 067 was written without naming the edge; this point
   is the decision that row asks for, and card 067 now names it.
4. **It holds the reader and writer of ADR-0149's scene format.** The project file (ADR-0146
   point 7, today `editor/`'s) and prefab templates (D-175) are the natural next tenants, and
   each moves in only when a second caller needs it — neither is moved by this decision.
5. **It is authoring-time code and says so.** Its folder page states that its halves need
   descriptions compiled in, that no shipped game is expected to link it, and why a game that
   turns descriptions on may.
6. **It reads and writes text, never files.** The reader takes a byte range and the writer
   returns text in an arena, as every reader in `assets` does; opening a file is `platform`'s,
   and which call does it is D-226.

## Blast radius

**Moderate for the name and the row.** Every include of `authoring/...` and the edge list depend
on them, and moving the folder later is a rename across the editor and the cook. **Cheap for
what is inside it.**
Reversibility: **moderate.**

## Consequences

- **Nine engine folders plus the programs.** `voe3d/CLAUDE.md`'s tree and table, the root
  `CMakeLists.txt` and `cmake/voe.cmake` gain a line each; the card that creates the folder does
  it.
- **`scene` stays a runtime folder**, with no parser in it and no transitive `platform`.
- **The writer can be built before the reader.** It only reads the world, so it has no bearing on
  rule 3; its card needs nothing undecided.
- **The reader cannot be written yet**, and the reason surfaced while writing this: it must put
  rows into tables it does not own, and `voe3d/CLAUDE.md` rules 3 and 4 say a component is
  written only by its own system. That is D-261.

## Rejected options and why

**B — `scene` gains `assets`.** It puts authoring-time code, which does nothing in a game's own
build, inside the folder every game links, and widens `scene`'s dependencies for it.

**C — `editor/` for now.** The move it defers is already known to be needed, and it would start
the engine's strictest round-trip code outside any tested folder.

## Questions this opens

- **D-261** — how a loaded scene's rows reach tables `authoring` does not own, given rules 3 and
  4. Hot, and it blocks the reader.
