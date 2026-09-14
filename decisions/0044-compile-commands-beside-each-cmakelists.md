# 0044. `compile_commands.json` is exported and placed beside each project's `CMakeLists.txt`

- **Status:** Accepted
- **Date:** 2026-08-31
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

Two unrelated consumers want the same file.

**Editors.** `clangd` — which is what a Neovim contributor uses, and what
several other editors use underneath — has no other way to learn how a
translation unit is compiled. Without a compilation database it guesses, and on
this project it guesses wrong in three places that matter: `-std=c23`,
`render`'s vendored Vulkan include path (ADR-0040), and `platform`'s *generated*
Wayland protocol headers, which do not exist until the build has run
(ADR-0037). A contributor opening `platform/src/window_wayland.c` in a fresh
clone sees a wall of red that has nothing to do with the code. CLion, which the
principal uses, reads the same file.

**`check.cmake` step 7.** ADR-0042 made `clang --analyze` a step of the check
script, and an analyser invocation needs the same three things an editor needs,
for the same reason. That step is specified separately in ADR-0045; it is a
second consumer of this decision, not the reason for it.

The constraints already fixed: every folder configures standalone and is its own
CMake project (ADR-0001, ADR-0027); the root ties them together; Ninja on both
platforms (ADR-0027); the onboarding invariant means nothing here may add a
required tool (ADR-0021).

**Where CMake puts the file is not where a reader looks.** `clangd` searches a
source file's own directory, then each ancestor, and a `build/` subdirectory of
each. Our binary directory is `${sourceDir}/build/debug` — one level deeper than
that search reaches. So export alone does nothing; the file has to be moved.

## Options considered

### Option A — export at the root only, copy to the repository root
One `compile_commands.json`, from the root configure, sitting next to the root
`CMakeLists.txt`. Every source file in every folder finds it by walking up.
Simple, one file, one rule. It says nothing about a folder configured
standalone, which then has a build directory whose database never surfaces.

### Option B — one per project, beside that project's `CMakeLists.txt`
The rule is uniform rather than root-special: whatever project you configured,
its database lands beside its own `CMakeLists.txt`. Configure the root and the
root gets one; configure `render/` standalone and `render/` gets one. Because
`clangd` takes the *nearest* database walking upward, a contributor working
inside a standalone folder gets that folder's flags and everyone else gets the
root's, with no precedence rule written anywhere — it falls out of the search
order.

### Option C — export nothing; let contributors configure their editors
Status quo. Costs nothing to build and pushes the cost onto every contributor
forever, in a form ("why is my editor red") that reads as the project being
broken rather than unconfigured. It also leaves ADR-0042's step 7 with no way to
learn a flag.

## Decision

**Option B, decided by the principal.** `CMAKE_EXPORT_COMPILE_COMMANDS` is on,
and after generation the database is placed beside the `CMakeLists.txt` of the
project that produced it.

The deciding factor: it is the same sentence for the root and for a standalone
folder, so ADR-0001's standalone property costs nothing here instead of needing
a carve-out — the case Option A leaves undefined is the case Option B never has
to mention.

**Consumers of the library are explicitly out of scope.** The principal's
words: *"How that affects consumers of the library, I dont know and i dont think
we need to think about it yet."* Nothing here is a promise to anyone outside
this repository.

## Blast radius

**Reversibility: cheap.** One cache variable and one placement step. No source
file references the database, nothing links against it, and no build output
changes. The only thing that regresses on removal is editor comfort and
ADR-0045's step 7.

The direction that would be expensive is the one not taken: making any *rule*
depend on the database's contents — a naming check, a lint gate — would turn a
convenience into an obligation and give a generated file authority over the
tree. Step 7 reads it; nothing else may.

## Consequences

- **The database is generated and gitignored, in every project that emits one.**
  A committed compilation database is a machine's absolute paths in another
  machine's checkout.
- **The placement step needs a copy fallback on Windows.** The natural spelling,
  `file(CREATE_LINK … SYMBOLIC)`, requires either Developer Mode or elevation on
  Windows and fails silently into a broken link otherwise. This is a two-platform
  project and this is exactly the kind of thing that works on the Linux machine
  and does not on the other one.
- **The file is only as current as the last configure.** An editor reading a
  database from before a folder gained a dependency shows stale errors. This is
  a known and universally accepted property of the format; it is called out here
  only so it is not mistaken for a bug in the placement rule.
- **A contributor who configures both the root and a folder standalone has two
  databases**, and `clangd` uses the nearer one. That is the correct answer, not
  a conflict — but it is a surprise the first time, because the nearer one knows
  about fewer dependencies.
- **`clangd` becomes the de facto second reader of `cmake/voe.cmake`.** Anything
  that folder function does to flags now shows up in every contributor's editor
  the same day, which is a quiet reinforcement of ADR-0027's single-place rule.

## Rejected options and why

- **Root-only export (A)** — rejected not because it is wrong but because it is
  a special case pretending to be a rule. It answers the common path and is
  silent on the standalone path that ADR-0001 makes a first-class way to work.
- **No export (C)** — rejected on evidence: the three things `clangd` cannot
  guess here (`-std=c23`, vendored Vulkan headers, generated Wayland headers)
  are all decisions this project took deliberately, so the editor breakage is
  self-inflicted and fixable in one line. It also leaves ADR-0042 undeliverable.

## Questions this opens

- **D-048 (new)** — none open on the decision itself; the placement mechanism
  and its Windows fallback are implementation, settled by the card that writes
  it. Recorded so the register carries a row for the decision.
