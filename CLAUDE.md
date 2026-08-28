# VOE3D

A general-purpose real-time 3D engine. This repository is for **executing and
writing**. It is self-contained — nothing here depends on a parent directory.

## Read first

| Document | Authority over |
|---|---|
| `guidelines.md` | Scope of work, folder structure, `<folder>.md` files, coupling, workflow, design preference. |
| `coding_convention.md` | Code style. Linux kernel style, small functions with asserts, one level of dereference. |

Both are authoritative. This file does not restate them.

`guidelines.md` uses C#, C++20 and F# only as examples. Every rule in it applies
to C. Read it with this mapping:

| In `guidelines.md` | In C |
|---|---|
| namespace | a **folder** — folder and namespace stay in sync means the folder *is* the namespace |
| module (a language construct) | a **header + source pair** — `foo.h` + `foo.c` |
| class owning its data | a struct plus the functions in its pair; fields are not touched from outside the pair |

## Givens

| | |
|---|---|
| Language | C23 (plain C — not C++) |
| Graphics API | Vulkan only |
| Platforms | Windows and Linux desktop only |
| Build | CMake, dependencies fetched at configure time |

**Onboarding invariant:** clone this repository and build with nothing installed
beyond a compiler and CMake. An option that breaks this is rejected on that
ground alone.

## Rules

1. **Every folder configures standalone.** A folder is the unit CMake links.
   It owns a `CMakeLists.txt` that works on its own, keeps its public headers in
   `include/<folder>/`, and never reaches into a sibling's source directory.
2. **Folder dependencies point one way.** No upward or sideways dependency, no
   cycles. A change that needs one is reported, not made.
3. **No CI.** Verification is local, run before a card moves to `complete/`.
4. **Do not push or pull.** Remote git operations are the human's.

## Work

`kanban/todo/` → `review/` → `complete/`. One card per file, `NNN-slug.md`,
shaped by `kanban/CARD-TEMPLATE.md`. Set `claimed-by:` before starting — there
is no in-progress bucket, so that field is the only claim signal.

Cards arrive in `todo/` already decided. Implement what the card says; a card
that is unclear is asked about, not guessed at.

## Status

**No engine source or build files yet.** The module layout and toolchain are not
settled, so code written now would encode guesses about both. Throwaway spikes
are fine; they do not live in this repository.
