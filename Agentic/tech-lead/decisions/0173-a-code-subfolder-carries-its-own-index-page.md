# 0173 — A code subfolder carries its own index page, and the module page keeps what the reader came for

date: 2026-09-18
by: planner

## Decision

**Every folder that directly holds code files carries `<folder>.md`**, named for the folder it sits in:
`base/src/src.md`, `base/tests/tests.md`, `base/include/base/base.md`. This is `checks.sh`, adopted whole with
ADR-0168, and it is unconditional — crowding is no longer the trigger, so ADR-0133 point 4 ("a page under its
ceiling delegates nothing") does not hold for a code subfolder.

**The shape of an index page**, exactly as `checks.sh` enforces it:

- First line `# <folder>`, the bare name of the folder the file sits in, and no other line beginning `#`.
- No code blocks and no tables; a line beginning with a backtick fence or a pipe is a finding.
- One entry per code file directly in the folder and one per subfolder below it that holds code, written
  `- ` + the bare name in backticks + ` — ` + one sentence saying what the file is for.
- Nothing listed that is not there.
- The sentence is a pointer, not documentation: what the file owns, in a line. Everything else belongs in the
  file's own header comment, which is where a reader who has chosen the file goes next.

**What sits where.** The module page — `platform/platform.md`, the one `Agentic/tech-lead/system.md` names —
keeps its preamble and its public-surface entries, because ADR-0133 point 2 says a folder page answers on its own
what the folder is for and what its public surface is. It also keeps one line per subfolder.

- `src/src.md` and `tests/tests.md` **take** the per-file entries: they move down out of the module page, they
  are not copied. `render/src/src.md` and `render/tests/tests.md` already work this way and are the model.
- `include/<folder>/<folder>.md` is a **thin index**: one line per public header, a short sentence each. The
  module page keeps its own fuller `include/...` entries, because they are its duty under ADR-0133 point 2. This
  one duplication is the price of the rule and it is the smallest one available.

## Reasoning

- **Copy every entry down and leave the module page untouched** — rejected. It doubles `src/` and `tests/` as
  well as `include/`, so three of four pages have two places to edit when a file changes, and the module pages
  that are already over ADR-0114's 120 lines (`3d`, `dev`, `editor`, `platform`) stay over it for no gain.
- **Move everything down, module page becomes a table of contents** — rejected by ADR-0133 point 2: a reader who
  opens `platform/` to find out what it is and what it exposes must land on an answer, not an index.
- **Ask the sponsor to exempt the subfolders in `CLAUDE.md`** — rejected. The exemption list is for code nobody
  edits (`render/vulkan`, `history`); exempting `src/` everywhere would turn the workflow's one mechanical
  documentation rule off for the whole engine to avoid writing forty short files once.

## Replaces

ADR-0133 point 4, for folders that hold code: delegation to a code subfolder is now required rather than
triggered by crowding. The rest of ADR-0120 and ADR-0133 — present tense, and a module page that answers what the
folder is and what its public surface is — stands untouched.
