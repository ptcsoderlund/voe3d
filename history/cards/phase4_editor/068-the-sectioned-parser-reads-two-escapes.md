# 068 — the sectioned parser reads two escapes

claimed-by: kanban-coder (claude-opus-5, session 1ad66825)
blocked-by: -
decision: *A scene file is entity sections by number, component sections beneath, and one spelling per value* (ADR-0149) — its value table's text row and "Where this lands in code": the parser's only change is `\"` and `\\`.

## Goal

A quoted value in the engine's authored text format may contain a quote and a backslash,
written `\"` and `\\`. Any other backslash inside a quoted value is a malformed line. Nothing
else about the parser changes.

## Scope

**1. `assets/src/sectioned.c`.** Inside a quoted value only:
- `\"` is one `"` in the returned text and does not close the value.
- `\\` is one `\`.
- A backslash followed by anything else — including end of line — is a malformed line, reported
  with its line number like every other malformed line, and the parse returns false.
- The returned text is the unescaped bytes, NUL-terminated in the arena, as today.

**Unquoted values are untouched**: a backslash there is a byte, as it is today. The rule is
about quoted text because that is the only place a quote could otherwise end a value early.

**2. `assets/include/assets/sectioned.h`.** Replace the paragraph *QUOTES ARE STRIPPED AND NOTHING
INSIDE THEM IS ESCAPED* with what is now true: the two escapes, that any other backslash in a
quoted value refuses the line, that authored paths are written with `/` on every platform so
a Windows path never needs one, and that a value quoted or not is still not recorded. Add the
backslash case to the list of malformed lines. **Name ADR-0149 where the old paragraph named
rule 10.**

**3. `assets/tests/sectioned.c`.** `quotes_are_stripped_and_nothing_is_escaped` is replaced — its
premise is withdrawn by the decision, so rewriting it is this card's job and not a broken test.
New tests:
- `name = "say \"hi\""` returns `say "hi"`.
- `path = "a\\b"` returns `a\b`.
- `path = "C:\Assets"` is refused, and the report names the line.
- `x = "ends in \\"` returns `ends in \`.
- `x = "open \"` (escaped quote, then end of line) is refused as an unclosed quote.
- `url = http://a\b` (unquoted) returns `http://a\b` unchanged.
- The file header comment's list of what is pinned by a test is updated.

**4. `assets/assets.md`** — the entry for `sectioned.h` if it mentions escaping.

## What must not change

- Every other test in `assets/tests/sectioned.c` passes unedited.
- Comments stay whole-line `//`; section names, the two-part limit, duplicates refused, blanks
  around `=`, `\r\n`, empty values, the arena contract.
- The parser still interprets nothing: no numbers, no arrays split, no booleans. Those are the
  scene reader's, which has no card yet.
- No folder but `assets`. No theme file needs editing: none in the tree contains a backslash —
  confirm with `grep -rn '\\\\' --include=*.theme --include=*.txt .` or equivalent, and say what
  you ran in Notes.

## Verify

- Linux: `cmake -P check.cmake` green; `ctest -R assets` passes.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A quoted value can hold a quote and a backslash, and a Windows-style path in quotes is refused
with the line number rather than read as something else.

## Notes

**Done 2026-09-13, verified on Linux (Fedora 44, clang, lavapipe for the GPU tests).** Not
checked on Windows; nothing here is platform-specific but a `\r\n` file with a quoted escape
was not run there.

- `assets/src/sectioned.c`: `read_key()` scans a quoted value byte by byte, stepping over `\"`
  and `\\` and refusing any other backslash (end of line included) as a malformed line on both
  passes. `struct line` carries `quoted`, and `intern()` unescapes a quoted value into the pool;
  unescaping only drops bytes, so the pool sizing and its assert are unchanged. File header says so.
- `assets/include/assets/sectioned.h`: the quoting paragraph now states the two escapes, the
  refusal, `/` for authored paths, backslash as a byte outside quotes, quoted-ness not recorded,
  and names ADR-0149. The backslash case is in the malformed-line list.
- `assets/tests/sectioned.c`: `quotes_are_stripped_and_nothing_is_escaped` replaced by
  `a_quoted_value_has_two_escapes`, holding the six cases the card lists plus the two existing
  unquoted/quoted-equivalence checks it carried. File header's list of pinned decisions updated.
  Every other test unedited.
- `assets/assets.md`: its `sectioned.h` entry does not mention escaping, so it is unchanged.

**One reading to flag.** "Refused, and the report names the line": the test checks the refusal;
no test in the tree captures stderr and `base/report.h` has no hook to, so the line number was
checked by running the test binary, which prints
`error: assets: sectioned: line 3: a backslash in a quoted value that is not \" or \\` for the
`C:\Assets` case (placed on line 3 on purpose). A test that asserts the number needs a way to
capture a report, which is not this card's folder.

**Ran:**
- `cmake -P check.cmake` → exit 0 (45 tests, analyser 124 files, all standalone configures ok).
- `ctest -R assets --output-on-failure` → 6/6 passed.
- `grep -rn '\\' --include=*.theme --include=*.txt --include=*.scene . --exclude-dir=build
  --exclude-dir=cmake-build-debug` → no matches; there are no `.theme` files in the tree at all,
  and `voe_assets_sectioned_parse` has no caller outside `assets/tests/sectioned.c`.
- `bash tools/hot.sh` at the planning root → all hot files under their ceilings.
