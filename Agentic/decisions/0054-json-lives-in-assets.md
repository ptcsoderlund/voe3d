# 0054. The JSON parser lives in `assets`, over a byte range

**Rule:** The in-house JSON parser is part of **`assets`**, not `base`. It
operates on a range of bytes handed to it and does no file I/O of its own.

**Status:** Accepted · 2026-09-01
**Deciders:** Human, Tech Lead

**Why:** `base` owns allocation, containers, strings and assert, and depends on
nothing — which means every other folder depends on it. A JSON parser there is
linked by `render` and `platform`, neither of which will ever call it, and
`base` was deliberately shrunk by ADR-0034 to the assert path and the arena.

`assets` is the folder whose whole job is reading files into CPU data
(ADR-0022), and every JSON consumer in sight is its business: glTF is JSON
(ADR-0010), the shader graph is JSON (ADR-0053), and the text scene format
arrives with the editor.

**Byte-range, not file-reading**, for a specific reason: it preserves the
property ADR-0022 bought by separating `assets` from `render` — a model can be
loaded, and now parsed, on a machine with no GPU. Keeping the parser off the
filesystem too makes it testable from a string literal, which is what makes a
parser's test suite worth having.

**Not new work, and not now.** ADR-0023 already committed to writing a JSON
parser and sized it at about a day for the subset glTF needs. Under ADR-0034 it
is written when the first glTF card calls it. **Only the placement is decided
today** — placement changes what later code looks like, which is the test
ADR-0034 sets for deciding early.

**Cost accepted:** the editor's own non-asset JSON — window layout, project
files — reaches for a parser through `assets`, which is a slightly odd edge for
a folder named for assets. Accepted because the parser is self-contained: if
that edge becomes wrong, moving it down to `base` is a file move, not a redesign.

**Rejected:** `base`. It is where a generic utility instinctively goes, and that
instinct is the same premature generality this pre-study has rejected under
other names — placing code by what it resembles rather than by who calls it.
