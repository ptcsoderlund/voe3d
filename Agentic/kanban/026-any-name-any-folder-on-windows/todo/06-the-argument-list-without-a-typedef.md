# 06 — The argument list is spelt without a typedef
folder: platform
decisions: 0168, 0247, 0248

## Change
Bug 01: card 05 cleared the `**` finding in `platform/src/arguments_win32.c` with
`typedef const char *utf8_argument;`. The C convention allows `typedef` only for opaque handles
and function pointers a library hands back, so that typedef goes. The `**` check
(`checks.sh`) matches a `*` followed by optional whitespace and a second `*`; the rule 6
deviation for this list is already recorded in `platform/include/platform/arguments.h` and
stays. Windows code is written and verified on Linux only (ADR-0130); this file is not
compiled on Linux, so write valid C23 by reading it.

- `platform/src/arguments_win32.c`:
  - Delete the `utf8_argument` typedef.
  - `values` becomes a pointer to an array of unknown size of UTF-8 strings,
    `const char *(*values)[]`: the arena block it points at is that array. Push
    `(count + 1) * sizeof((*values)[0])` bytes, write each entry and the NULL through
    `(*values)[i]`, and return `.values = *values` (it decays to what the struct holds). No VLA.
  - The header's RULE 6 DEVIATION paragraph keeps its first point (the list is argv's shape, a
    pointer to pointers) and replaces the `utf8_argument` sentence: the list is spelt as a
    pointer to the array of arguments, and no typedef stands in for a string.
- `platform/src/src.md`: the `arguments_win32.c` entry names no typedef today; leave it unless it
  becomes untrue.

## Done when
1. `! grep -n 'typedef\|utf8_argument' platform/src/arguments_win32.c` exits 0.
2. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`.
3. In `p=$(mktemp -d)/'Åsa 李 värld'` with `mkdir -p "$p" && cp -r examples/capsule/. "$p"`:
   `build/debug/editor/voe_editor --capture "$p/shot.png" "$p" 2>"$p/err"` exits 0, `$p/err` is
   empty, and `$p/shot.png` exists.
4. `bash ~/.claude/skills/checks/scripts/checks.sh --all` prints `FINDINGS: 0`.
5. The human's, on Windows: build the editor and follow steps 1–6 of `## How to test` in
   `feature.md`. Step 5 needs a second Windows user. If a step fails inside CMake, Ninja or
   clang, the build log says so; report which tool it was (0248).
