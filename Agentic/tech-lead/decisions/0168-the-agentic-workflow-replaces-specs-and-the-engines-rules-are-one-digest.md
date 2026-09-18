# 0168 — The Agentic workflow replaces specs, and the engine's rules are one digest
date: 2026-09-18
by: tech-lead

The engine has been built under two workflows already: the kanban of records 0001–0154 (a tech lead in a
separate planning root, cards, a bug inbox, a secretary), then the spec-driven one of ADR-0155 (`Agentic/specs/`
holding `spec.md`, `plan.md`, `tasks.md`; a 160-line `CLAUDE.md` carrying the engine's rules whole). On
2026-09-18 the sponsor moved every product to the Agentic framework — `/tech-lead`, `/drive`, `/secretary`, a
planner and a coder spawned by the driver, `checks.sh` as the mechanical rules — and asked for this repository to
be converted so work can go on in compliance. The framework fixes where files live and what shape they take:
`Agentic/tech-lead/`, `work-orders/`, `kanban/<feature>/`, `completed/`; a decision is `# NNNN — title`,
`## Decision`, `## Reasoning`; `CLAUDE.md` is under forty lines and has three headings. The old records and the
old `CLAUDE.md` fit none of that, and the engine's rules cannot be lost, because code cites them by number.

## Decision

**Work goes work order → cards → review, in `Agentic/`, as the framework says.** The tech-lead writes decisions
and work orders; `/drive` takes the lowest work order, cuts `feature/NNN-slug` from `dev`, and spawns the planner
and coders; `/secretary accept NNN` merges into `dev`. `cmake -P check.cmake` exiting zero on Linux is still what
done means (ADR-0004, ADR-0028, ADR-0042, ADR-0130); `checks.sh` runs it as the whole-suite command and runs a
folder's build and tests per card.

**The specs became features.** 001–004, accepted, are `Agentic/completed/NNN-slug/feature.md`. 008, in progress
on `feature/008-typing-what-you-press`, is the one feature in `kanban/`, its done task a card in `review/` and its
remaining task in `todo/`. 006, in progress on `feature/006-themes` with four tasks done, is a work order on
`dev` and a kanban folder on its branch, the undone tasks a blocked card the planner re-cuts; the sponsor's order
of work — 008, then 006, then the editor font — is kept by number, so spec 005 became work order 009 and
ADR-0167's "spec 005" reads as it. 007, a draft never interviewed, is lines in `ideas.md`. A spec's acceptance
criteria are its `## How to test`; nothing else in a spec was reworded. "Spec NNN" in a code comment means the
feature of that number in `completed/` or `kanban/`.

**Records 0001–0167 moved, unchanged, to `history/decisions/` with their index.** They keep their format and
their accepted status; `ADR-NNNN` means the record of that number wherever it lives. Numbering continues here
from 0168; the records written on the two branches under the old workflow are renumbered 0169 (008) and
0170–0172 (006) and brought to this shape, because two branches had each written a 0168 and the framework's
`decisions.md` must list every file in this folder. This is the one addition `history/` takes; the folder is
closed again behind it.

**`CLAUDE.md` holds product facts only, and the rules it held are restated below, numbered as code cites them.**
`rule N` in a code comment means rule N of this record. Where the framework's `c.md` convention and a rule or a
record here disagree, the rule wins: returned failure takes the shape of rule 13 (ADR-0041), not a negative code;
recursion over data the engine built itself is allowed and rule 14 binds only data read from a file; a struct
may be a `typedef` (ADR-0014); the floor of two asserts per function is not adopted, an assert marks a caller's
bug where one exists (rule 13). Everything else in `c.md` applies.

### Givens

Language C23, plain C. Compiler **Clang 19+ only, as the GNU-driver `clang` on both platforms** — not `clang-cl`,
not MSVC, not GCC; configuration fails for anything else; 19 because a compiled shader reaches the binary through
`#embed` (ADR-0005, ADR-0026, ADR-0047). Graphics Vulkan 1.3 only. Shaders in Slang, compiled to SPIR-V by
`slangc` at build time and `#embed`ded; nothing is read from disk at run time (ADR-0015, ADR-0046). Platforms
Windows and Linux desktop only. Build CMake 3.28+ with Ninja on both. Required tools, installed by the programmer:
Clang 19+, CMake 3.28+, Ninja, `slangc`; on Windows the Windows SDK and MSVC C runtime; on Linux the Wayland
development package (`wayland-scanner` and the client library). Optional, never required: the Vulkan SDK (the
easy way to get `slangc`; its validation layers are used when present) and `clang-tidy`. **Tools that transform
source are installed by the programmer; anything the engine links against or ships is fetched by the build**
(ADR-0021). A programmer clones and builds without assembling an environment; an option that breaks this is
rejected on that ground alone.

### Rules

1. **Every folder configures standalone.** A folder is the unit CMake links. It owns a four-line
   `CMakeLists.txt`, keeps public headers in `include/<folder>/` and sources in `src/`, and never reaches into a
   sibling's source directory (ADR-0001, ADR-0027).
2. **Folder dependencies point one way.** No upward or sideways dependency, no cycles. The allowed edges are data
   in `cmake/voe.cmake`; a `DEPENDS` outside them fails configuration. A change that needs a new edge is reported
   as needing a decision, not made (ADR-0022).
3. **Data is read by anyone, written by one.** Any code may read any component, read-only or by copy. A component
   is written only by its own system. The exception is creation: a folder's typed creation call and `authoring`'s
   scene reader add rows to entities they have just made, and neither edits one (ADR-0011, ADR-0152).
4. **To change another module's data, submit an intent.** An intent is a datatype, handed to the ECS, drained by
   the owning system. Never call another system; no `system -> system` dependencies (ADR-0013, ADR-0017).
5. **Write it ourselves.** No third-party dependency is added without asking the sponsor first; the default answer
   is to write it (ADR-0023).
6. **No `**`.** One level of dereference, not worked around with a typedef (ADR-0043).
7. **The prefix is `voe_`, everywhere.** Functions, types, CMake targets and projects, the namespace spelled out:
   `voe_math_float3_add`, `voe_render_…` (ADR-0029, ADR-0014).
8. **No CI. `cmake -P check.cmake` is the verification, and it exits zero before a task is done.** That includes
   step 6 (tests) and step 7 (`clang --analyze`): an analyser finding fails the script exactly as a warning does;
   no baseline file, no tolerated count. Where the analyser is genuinely wrong, suppress at that site with a
   comment saying why, in the form step 7's header gives — never globally, never by changing correct code
   (ADR-0004, ADR-0042, ADR-0045).
9. **Do not push or pull.** Remote git operations are the human's.
10. **Implement on demand. No "in case".** A function is written when something calls it, a type when something
    stores it; a set is not completed for symmetry. A card specifying surface nothing uses is over-specified:
    report it (ADR-0034).
11. **Memory is an arena you are handed.** Working memory comes from a `voe_base_arena` passed as a parameter and
    is freed by rewinding or destroying the arena, never one allocation at a time; there is no global or default
    arena. Long-lived or growable data is the exception and uses `new`/`destroy`. Failing to allocate is fatal,
    not a returned `NULL`; no null check after an allocation (ADR-0032).
12. **Tests are plain C programs, one per module**, at `<folder>/tests/<module>.c`, an ordinary `main()`, zero for
    pass. The build finds them; nothing is registered. Check macros come from `voe::testing`, linked from tests
    only, never from `src/`. A failure message names the expression, the expected value and the actual one
    (ADR-0031).
13. **A failure the world caused is returned. A failure the program caused is an assert.** A bad parameter is the
    caller's bug: `VOE_BASE_ASSERT`. A file that will not open, a device that will not create, a compositor that
    is not there: returned to the caller, who decides. One way to fail → return `NULL` from a `_new` or `false`,
    no error parameter with one value in it. Several distinguishable ways → still `NULL`, plus one
    `voe_base_error *error` out-parameter, which may be `NULL` when the caller does not care; where there is
    nothing to return the enum is the return value and `VOE_BASE_OK` is `0`. `[[nodiscard]]` on every function
    that can fail. The codes live in one enum in `base`, added when code needs one, categories not incidents
    (ADR-0041, ADR-0140).
14. **Do not recurse over data read from a file.** Recursive descent on a glTF or JSON file is a stack overflow on
    deep nesting that neither `-Werror` nor the analyser finds. Explicit stack, a named nesting limit, refuse the
    file past it, reason in the file header. This binds `assets` and `authoring`; recursion over data the engine
    built itself is fine.

### Scope of a card

A card that changes its folder's public surface also updates the call sites the change breaks in folders
downstream of it (ADR-0113); downstream is the edges in `cmake/voe.cmake`. It covers what the change broke and
nothing else — would the tree build without this edit? — and names the call sites touched in its report. No new
edge, no reversed arrow, no reaching into a sibling's source: those are blocked. One platform is enough, Linux
first (ADR-0130): `check.cmake` green on Linux finishes a card; Windows code is still written and `platform`
keeps both backends as equals; what the other platform turns up is reported, not reopened.

### Conventions the world is built on

Silent when wrong; where a reference disagrees, the reference is wrong for this engine (ADR-0033, ADR-0035).
Right-handed, +Y up, −Z forward; a camera looks along its own −Z; this is glTF's convention, so the importer
converts no coordinates and may not be given a conversion later. Vectors are columns: `M · v`, composition right
to left, translation the last column. Matrices are stored row-major, `m[row][column]`, because that is what the
same expression means in Slang; `slangc` runs with `-matrix-layout-row-major`; glTF stores column-major, so the
importer transposes — layout only, never coordinates. `math` is spelled the way Slang spells it —
`voe_math_float3`, `voe_math_float4x4` — and `_mul` is component-wise, `_scale` is by a scalar. Depth runs
backwards, deliberately: near 1.0, far 0.0, float depth buffer, `GREATER`, cleared to 0, orthographic included.
Exactly one Y flip exists, in the viewport, as a negative height in `render`, never by negating a projection row;
the front-face constant matches and a test proves it. `math` knows no graphics API and builds no projection
matrices. Angles are radians; units are metres and seconds.

### Build

Every folder's `CMakeLists.txt` is four lines: `cmake_minimum_required(VERSION 3.28)`, `project(voe_<folder> C)`,
`include(${CMAKE_CURRENT_LIST_DIR}/../cmake/voe.cmake)`, `voe_module(<folder> DEPENDS …)` — a program folder says
`voe_executable()`. `voe_module()` does the rest: compiler guards, the one flag set, dependencies, the target
`voe_<folder>` and its alias `voe::<folder>`, globbing `src/` and `tests/`; adding a file needs no CMake edit. The
`DEPENDS` line is the folder's row in `cmake/voe.cmake` and is checked. `render` is the GPU layer only and the
only folder that names Vulkan; the headers are vendored in `render/vulkan/`, declarations only, the loader is
opened by name at startup and every entry point resolved through `vkGetInstanceProcAddr` into one table resolved
once, never reassigned, never passed or stored; a missing loader is a returned failure (ADR-0040). Nothing
renders to the screen directly: the frame is drawn to a target and presenting it is a separate last step
(ADR-0024, ADR-0051). Throwaway spikes do not live in this repository. `render/vulkan/` is exempt from every
convention and never edited; `history/` is read-only.

## Reasoning

- **Keep the old records in `Agentic/tech-lead/decisions/`** — rejected: `checks.sh` requires every file there to
  open `# NNNN — title` and carry `## Reasoning`, and 0001–0154 are never edited (ADR-0155 said so again). Moving
  them whole to `history/` beside the cards they came with keeps them citable and keeps the rule.
- **Rewrite them into this shape** — rejected for the same reason, and the content is unchanged by the format.
- **Keep the rules in `CLAUDE.md`** — rejected: the framework caps it at forty lines and three headings, and a
  rule that is loaded into every agent's context whether or not its card needs it is what the framework was
  written to stop. A decision the planner names on every card reaches the coder that needs it and no one else.
- **Renumber the rules to fit `c.md`, or drop the ones it covers** — rejected: about eighty code comments say
  `rule 10`, `rule 13`, `rule 14`, and their meaning must not move.
- **Renumber 006 to sit below the font work order instead of renumbering 005** — rejected: 006 has a branch,
  three records and code comments naming it; 005 had a parked plan whose independent half was already built as
  006's card 02.
- **Merge the two branches' 0168 into one, or remove one (ADR-0056)** — rejected: they are different decisions
  on different features; renumbering is the change that loses nothing.
- **Leave the two in-flight branches to be replanned from scratch** — rejected: four cards of 006 and one of 008
  are done, committed and tested; the framework has no reader for a lost commit.

## Replaces

ADR-0155's workflow — spec → plan → tasks in `Agentic/specs/`, `guidelines.md` and `coding_convention.md`
replaced by the framework's guidelines skill — is retired; the stack 0155 recorded stands as written. The
statement in ADR-0155 and `history/history.md` that nothing is added to `history/` is amended for this one
addition. ADR-0113 and ADR-0130 keep standing over the framework as rules above say.
