# 0146. A game is a project folder, marked by `project.voe3d`

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0121 point 3's `dev_editor` row and point 5's wording only — `dev_editor`
  is a project, not a program built here. Everything else in ADR-0121 stands
- **Superseded by:** —

## Context

**Opened by the principal on 2026-09-13**, choosing the editor's next arc — save and open,
with a scene view alongside:

> *"We should have a project file. So we can do "new project" which creates a project file
> in a folder. Then the editor can attach to that project and dev can start creating games."*

**It names a noun the decisions already lean on and never define.** ADR-0052's table has a
game built from *the engine plus game code plus cooked data*; ADR-0145 point 4 says a game
is built by the cook *from its own tree*; ADR-0144 has the cook emitting C into it. None of
them says what that tree is, where its root is, or how the editor finds it. ADR-0096
anticipated *a project file* as a known-schema consumer of the authored text format and
went no further.

**The question: what is a project, and how does the editor come to be working in one?**

Constraints already fixed. ADR-0008: no runtime code loading. ADR-0010 and ADR-0073:
authored data is text in git, in the engine's sectioned format. ADR-0121: the editor is a
leaf, no engine folder gains anything for its sake. ADR-0145: nothing shipped is built
from this repository. And two facts about today: **nothing in the engine opens a file**
(D-226), and **nothing in the engine reads typed text** (D-245), so an editor has neither a
name field nor a folder picker to offer.

## Options considered

### Option A — a folder, marked by one file at its root under a fixed name
A project is a directory. A file with a fixed name at its root says so and holds what is
project-wide; scenes are their own files beside it. Godot's `project.godot`, Cargo's
`Cargo.toml`, Unreal's `.uproject`. Any tool finds the root by looking for one file, and it
is the natural place a game's build hooks in later. Permanent: the file's name, and that
the folder is the unit. Cheap: which keys the file holds, because keys are additive.

### Option B — one file holding everything, scenes included
Every scene edit rewrites the same file, so every two people editing any two scenes
conflict in git, and the file only grows.

### Option C — no marker, a folder layout by convention
A project is whatever folder has the expected sub-folders. Nothing marks a root, so a tool
cannot tell a project from any directory, and the editor opens the wrong folder quietly.

## Decision

**Option A**, the tech lead's recommendation and the principal's call, including the name.

> *"project.voe3d is a good name. Lets do it"*

1. **A project is a folder with a file named exactly `project.voe3d` at its root.** The name
   is fixed, not `<name>.voe3d`: a folder either is a project or is not, and nothing has to
   search for which file is the one.
2. **The file is written in the engine's authored text format** (ADR-0073), and its
   spelling follows whatever the format's grammar decision (D-097) settles. It is the
   format's first consumer after the theme, and deliberately small.
3. **It holds project-wide facts and nothing else.** Today that is **the project's name**,
   and **the start scene** as a path relative to the folder once a scene has been saved.
   **Scenes are their own files beside it, never inside it** — that is what rules out
   Option B. What a scene file is named and where in the folder it sits is decided with the
   scene's save, not here.
4. **The editor works in exactly one project, given as a folder on its command line.**
   - `voe_editor new <folder>` creates the folder if it is absent, **refuses a folder that
     is not empty**, writes `project.voe3d` with the name set to the folder's own name, and
     exits.
   - `voe_editor <folder>` opens it. **A folder with no `project.voe3d` is refused** through
     the reporting call (ADR-0140) with a non-zero exit. The editor does not walk up parent
     directories looking for one, as git does: which project is open is always stated.
   - With no argument the editor opens as it does today, on the scene built in code, until
     the card that loads a scene retires that scene.
5. **New and Open from inside the editor wait for typing.** A name field needs typed text
   and a folder picker needs either typed text or a native dialog, which is a new
   dependency. The command line is the first interface, not a stopgap to be apologised for;
   the button is additive (D-256).
6. **`dev_editor` is a project, not a program.** ADR-0121 made it a third program built in
   this repository. Under a project model it is **the example project the editor opens**:
   `voe3d/dev_editor/`, holding a `project.voe3d`, and no `CMakeLists.txt` until the cook
   gives a project a build. `check.cmake` lists only top-level folders that hold a
   `CMakeLists.txt`, so such a folder is outside the build and outside the check. What
   ADR-0121 wanted `dev_editor` to prove is unchanged; it now proves it the way a developer
   would, by being opened.
7. **Where the project-file code lives is `editor/`.** No engine folder gains a project
   concept for the editor's sake (ADR-0121 point 6). A game reads no project file at run
   time — the cook turns what it needs into C (ADR-0144) — so nothing but the editor, and
   later the cook, ever reads one.

**Nothing here puts a card on the board yet.** Writing `project.voe3d` needs the grammar
(D-097) and a file write (D-226); both are the arc's next decisions, and the project card
is written behind them.

## Blast radius

**Load-bearing for the name and for the folder being the unit** — every project anyone
creates carries `project.voe3d`, and renaming it later is a migration of other people's
folders. **Cheap for everything inside the file**: keys are added, not changed. Cheap for
the command-line shape, which a button joins rather than replaces.
Reversibility: **load-bearing for the name and the unit, cheap for the contents.**

## Consequences

- **A developer has something to make a game *in*.** After this arc — project, scene view,
  save, open — a developer can create a project, see its world, edit it, save and reopen.
  Turning it into a game that runs is the cook arc, which is where the folder gets a build.
- **The editor's run command gains an argument.** `editor/editor.md` names how to run it and
  changes with the project card.
- **The hardest question the project makes visible is not answered here.** A game has its
  own C and its own components, and the editor loads no code at run time. So *attach to a
  project* means either the editor edits only components it was built with, or the editor
  is rebuilt with each project's code, which is Unreal's shape. Nothing in this arc
  defines a game component, so it is parked on the first that does (D-254).
- **`dev_editor` stops being a program to keep building**, and becomes a folder that must
  keep opening — a lighter guarantee to maintain, and closer to the real use.
- **Refusing a non-empty folder is conservative on purpose.** `new` over a folder of
  existing work is how somebody loses it; the refusal can be relaxed later, a lost folder
  cannot be.

## Rejected options and why

**B — one file holding everything.** Merge conflicts on every pair of scene edits, in the
one tool (git) the authored format was chosen to serve.

**C — a folder by convention.** Nothing marks a root, so the failure is the editor quietly
working in the wrong folder. A marker file costs one file.

**`<name>.voe3d` rather than a fixed name.** Unreal's `.uproject` shape. It lets a folder
hold two projects, which nobody needs, and makes every tool search for which file is the
project. Rejected with the principal's choice of name.

## Questions this opens

- **D-254** — how the editor comes to know a game's own components, given no runtime code
  loading. Parked on the first component a game defines.
- **D-255** — what build files a project holds and how a project names the engine it builds
  against. Parked on the first cook card.
- **D-256** — New and Open from inside the editor: a name field, a folder picker or a native
  dialog, and whether there is a recent-projects list. Parked on typing (D-245).
- **D-231** — what `dev_editor` must contain to prove the cook, opened by ADR-0121 and
  never entered in the register. Narrowed here to *what the example project must contain*;
  filed under the first cook card.
