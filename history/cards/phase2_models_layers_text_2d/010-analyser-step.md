# 010 — the analyser step the rulebook already promises

status: complete
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

`CLAUDE.md` rule 8 says `check.cmake` runs `clang --analyze` and fails on any
warning. **It doesn't.** The script ends at step 6b. Decided by ADR-0042 and never
built. The triangle card found it, ran the analyser by hand, and it was clean —
which is not the same as the script enforcing it.

## Goal

`check.cmake` runs the analyser over every folder's `src/` and `tests/`, zero
warnings, no baseline. A finding fails the script.

## What is already decided

- **Zero warnings, no baseline** (ADR-0042). Not `clang-tidy` — that is a separate
  install and would be a fifth required tool.
- **A false positive is suppressed at the site with a reason**, never globally, and
  never by changing correct code to quiet it.
- The step is numbered per the script's current ordering; the triangle card's run
  reported 13 steps, so pick the next number and say what it is.

## What this card decides

The register carries these as open, and they are yours to settle against real
code rather than in the abstract:

- **The exact invocation and checker set.** Start with the default checkers and no
  extra `-Xanalyzer` enables. Add one only if you can show a real finding in this
  tree that the default set misses, and say so if you do.
- **How a per-site suppression is spelled.** Whatever you choose, it must be
  greppable, and the reason must be next to it.

## Verify

- A deliberately leaky file fails the step; removing it passes. **Prove the step
  fires** — a check step that cannot fail is the failure mode card 002 existed to
  prevent.
- Clean over the whole tree as it stands today.

## Known cost, already accepted

This compiles the tree a second time and roughly doubles a check. The principal
accepted that explicitly: there is no CI, so this script is the whole safety net.
Do not add a fast path or a changed-folders-only mode — that was considered and
rejected.

## Notes — implementation

**It is step 7, with 7b beside it.** Rule 8 already called the analyser step 7,
so the number was chosen for it; 7b is its can't-fail proof, standing to 7
exactly as 6b stands to 6. The printed-step count the card mentions is a
different quantity — step 2 prints one line per folder, so the tally moves every
time a folder is added, while the section numbers do not.

**The exact invocation.** The flags are not written here at all. The compile
database step 3 already wrote is read back and each entry replayed, so the
include directories, the C standard, the warning set and render's `--embed-dir`
have exactly one definition — `cmake/voe.cmake` — and a folder added later is
covered with no edit to `check.cmake`. Default checker set, no `-Xanalyzer`
enables: nothing in this tree needed more than the default to be caught. Three
flags are added and each is forced, not preferred:

- `-Xanalyzer -analyzer-output=text` — an output format, not a checker. Without
  it clang drops a `.plist` beside every source analysed.
- `-Wno-unused-command-line-argument` — the replayed entry carries `-Werror`, and
  a codegen- or link-only flag becomes an unused argument once `-c` and `-o` are
  stripped. Verified: `-Wl,--gc-sections` under `-Werror` fails a clean file
  outright without this.
- `-o`/`-c`/`-MD`/`-MT`/`-MF` are dropped, since an analysis makes no object and
  no dependency file.

**`clang --analyze` exits zero with findings in hand, and `-Werror` does not
change that.** Measured, not assumed: an analyser finding is not a compile
warning. A step that read the exit code would have passed forever while
reporting nothing — the exact failure card 002 exists to prevent. So the step
judges the output, and reads the exit code only to catch clang failing to launch.
Step 7 also fails when it analysed *zero* files, so a filter that stopped
matching cannot pass as a clean tree.

**How a suppression is spelled:**

```c
        // <one line saying why the analyser is wrong here>
        #ifndef __clang_analyzer__
                ...
        #endif
```

Greppable by `grep -rn __clang_analyzer__`, reason on the line above, and
`__clang_analyzer__` is defined only while analysing so the real code is still
the code that compiles. **There are no suppressions in the engine today** — none
was needed, so none was added (rule 10). Step 7b proves the spelling works
anyway, so nobody has to discover otherwise mid-card.

## Notes — verified

Run on Linux/WSL. `cmake -P check.cmake` **could not be run end to end on this
machine**: `ninja`, `slangc` and `wayland-scanner` are absent, so step 1 fails on
`slangc` before anything else executes. That is an environment gap, not a result
— **step 7 has not been observed inside a full check run, and Human should run
`cmake -P check.cmake` on Windows, where the tree does build.**

What was run instead: the step 7 and 7b text was extracted from `check.cmake`
**verbatim** and executed by `cmake -P` against a real CMake-generated compile
database, substituting only the generator (`Unix Makefiles`, no ninja here) and
the check directory (drvfs cannot `chmod`, see below).

| | result |
|---|---|
| step 7, real database, `base` + `render/tests/loader.c` | `ok analyser (6 files)` |
| step 7b | `ok analyser reports a finding` |
| whole file parses | `cmake -P check.cmake` reaches step 1 and fails on `slangc`, so there is no syntax error anywhere in it |
| `render/tests/loader.c` after the fix | compiles clean under `-Werror`; analyser silent |
| everything this machine can analyse | clean: `base/src` ×4, `base/tests` ×1, `math/src` ×4, `render/src` device·frame·loader·swapchain·backend_wayland, `render/tests/loader.c`, `dev/src/main.c` |

**Proved it fires.** Four mutations, each required to fail and each doing so:

| mutation | expected | got |
|---|---|---|
| leak body replaced by clean code | 7b fails | `a deliberate leak was not reported as unix.Malloc` |
| `__clang_analyzer__` misspelled in the suppression | 7b fails | `the documented suppression did not silence the finding` |
| database naming the real pre-fix `render/tests/loader.c` | 7 fails | the `core.CallAndMessage` report, with its full trace |
| database holding no folder source | 7 fails | `the database holds no <folder>/src/ or <folder>/tests/ file of ours` |
| database absent | 7 fails | `step 3 left no compile database at …` |

**Not analysed anywhere, and a real gap in this card's evidence:**
`platform/src/window_wayland.c` and `library_wayland.c` need `wayland-client.h`
and wayland-scanner's generated header, neither present here; every `_win32.c` in
`platform` and `render` cannot be analysed on Linux at all. Those files are
covered by step 7 on Human's Windows machine and have never been through it. **A
first Windows run may well surface findings this card never saw.**

## Notes — the tree was not clean

The card's premise that the tree is clean did not hold. Step 7 found one real
finding in `render/tests/loader.c:48`: `VOE_TEST_CHECK` on the pointer records a
failure without returning, and the next line calls through the same pointer, so
`core.CallAndMessage` reports a null call. `loader.c:100` does guarantee the
pointer whenever `voe_render_loader_open()` returned true, but that invariant is
in another translation unit and the analyser cannot see it.

`render` is a sibling folder, so this was **not** made silently: it was put to
Human with the options, and Human chose to guard the call rather than suppress
the finding. The guard makes the test return early when the pointer is null, so
the analyser needs no suppression and the test reports instead of dereferencing
null — which is a latent defect fixed, not merely a diagnostic quieted.

`AUTHORISED CROSS-FOLDER EDIT: render/tests/loader.c, the analyser finding above,
Human's call over the two options offered.`

No `DEVIATION:` and no `BLOCKED:` markers were left in the code.

## Notes — reported, not repaired

Two pre-existing environment conditions found on the way past and deliberately
left alone:

- **The worktree is CRLF and the index is LF**, with no `.gitattributes` and no
  `core.autocrlf`, so `git status` shows all 79 files modified with 37552
  identical insertions and deletions. It will contaminate every commit made from
  this machine. A repo-wide call, and Human's.
- **Step 6b already fails on this machine** for a reason that has nothing to do
  with this card: `configure_file: Operation not permitted`, because the tree is
  a drvfs mount owned by `nobody` and CMake cannot `chmod` there. Confirmed by
  running the pre-existing 6b as a control before concluding anything about 7b.
  Git also refuses the repository outright without
  `safe.directory` set for it.

**Suggestion, not done (out of card):** step 7 re-analyses every file on every
run and the card rules out a fast path, which is right. But nothing yet proves
the *database* is the one step 3 wrote — if step 3 were ever reordered after 7,
the step would silently analyse a stale tree. A cheap guard would be to assert
the database is newer than the step-3 build. Worth a card only if step ordering
ever becomes something people edit.
