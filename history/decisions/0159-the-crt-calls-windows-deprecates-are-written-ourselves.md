# 0159 The CRT calls Windows deprecates are written ourselves, and nothing is silenced tree-wide

Status: accepted
Date: 2026-09-15

Spec 003's editor gained a `--size <W>x<H>` argument parsed with `sscanf`. It built and ran on
Linux, where the feature was verified under ADR-0130, and did not compile on Windows at all: Clang
19 against the MSVC C runtime marks `sscanf` deprecated in favour of Annex K's `sscanf_s`, and
`-Werror` makes that an error. The same marking sits on `fopen`, `getenv`, `strcpy`, `strcat`,
`strtok`, `sprintf` and the time converters, so this is a class and not one call — and rule 8
forbids the two obvious ways out, because `_CRT_SECURE_NO_WARNINGS` loosens the check for the
whole tree and `sscanf_s` does not exist on Linux.

## Decision

**What the deprecated call did is written here instead.** A `--size` parser is a dozen lines and
refuses more precisely than a format string does; that is the usual shape of the replacement, and
rule 5 already says whose job it is.

**`_CRT_SECURE_NO_WARNINGS` is never defined** — not in `cmake/voe.cmake`, not in a source file,
not on a command line. Neither is `_CRT_NONSTDC_NO_WARNINGS`.

**No `#ifdef` splits the replacement.** One compiler builds both platforms (Givens), so a
replacement that needs a platform branch is the wrong replacement.

**A test may keep the C library as its oracle, and suppresses at the one call.** A test that reads
back what `platform` wrote must not read it with `platform` — a matching pair of bugs would pass —
and the C library is the only other reader there is. That call, and nothing wider, carries
`#pragma clang diagnostic ignored "-Wdeprecated-declarations"` with a comment naming this record
and saying the replacement is Annex K, which is not available on Linux. This is rule 8's
suppress-at-the-site, in the same shape and with the same limits. Never in `src/`.

**Each one is found by building on Windows.** There is no list to keep and nothing greps for the
class; the compiler already names the call and the line, and the sponsor builds there periodically.

## Rejected

- Define `_CRT_SECURE_NO_WARNINGS` in `cmake/voe.cmake` — one line, and it turns off a whole
  diagnostic class for every folder for ever. Rule 8.
- `sscanf_s` behind an `#ifdef _WIN32` — two parsers to keep in step, with the one nobody builds
  daily going wrong quietly, and Annex K is optional even where it exists.
- A `check.cmake` step grepping `src/` and `include/` for the deprecated names, so the class fails
  on Linux — **the sponsor declined it (2026-09-15): he test-builds and runs on Windows every now
  and then, and fixes what shows when he does.** A grep of a list is a second, weaker copy of what
  the compiler already knows, and it has to be maintained to stay true; the build is the authority
  and it is run often enough. This is ADR-0130 applied to a build break rather than to behaviour.
- A reader in `platform` so the tests need no `fopen` — rule 10: nothing loads a file yet, and a
  reader written to quiet a warning would be a public surface nothing calls.
- One shared file reader in `testing/` so there is a single suppression — it widens a surface every
  folder links, for two callers with different shapes. Revisit when there is a third.

## Consequences

- A call of this class written on Linux survives until someone builds on Windows, and is fixed
  then. That is the arrangement, not an oversight.
- Parsing in this engine is ours, which was already true of every format it reads.
- The two suppressions that exist are in `platform/tests/file.c` and `app/tests/capture.c`. A third
  appearing in `tests/` is a hint that the oracle should be something else.
