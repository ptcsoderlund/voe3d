# 0028. The local check script is a CMake script, run with `cmake -P`

- **Status:** Accepted
- **Date:** 2026-08-28
- **Deciders:** Tech Lead (delegated by Human)
- **Amended by:** ADR-0029 — the hook is `VOE_CHECK_FAKE_CLANG_VERSION`; nine folders per ADR-0030.
- **Supersedes:** —
- **Superseded by:** —

## Context

D-013. With no CI (ADR-0004), one local script is the only enforcement of the
standalone-folder property (ADR-0001), the module map (ADR-0022, made
checkable by ADR-0027) and the three configure-time guards (ADR-0005,
ADR-0026). It must run identically on Windows and Linux from a clean clone.

Constraint that kills most options: the required tools are Clang, CMake and
`slangc` (ADR-0021). Anything else the script needs is a fourth install.

## Options considered

### Option A — `cmake -P check.cmake`
One script, in a language every contributor already has, on both platforms.
CMake's scripting is ugly, but the job — run configures, inspect output,
assert — is what `execute_process` and `string(FIND)` do well.

### Option B — `check.sh` and `check.ps1`
Idiomatic on each platform; two copies to keep in step, which is the drift
ADR-0027 just spent a decision removing.

### Option C — Python
Pleasant to write; a fourth required tool for a script that runs once per card.

## Decision

**Option A.** `check.cmake` at the engine root, run as `cmake -P check.cmake`.
Deciding factor: zero new installs and one file, not two.

**What it runs**, in order, stopping at the first failure:

1. **Tools present and above floor:** `clang --version` ≥ 18 with the GNU
   driver, `cmake` ≥ 3.28, `slangc` present (floor is D-032).
2. **Every folder configures standalone** into its own scratch build directory,
   with the Ninja generator and the same flags the root preset uses.
3. **The root configures and builds** `debug`.
4. **Negative tests — each guard fires:**
   - configure with `-DCMAKE_C_COMPILER=gcc` (Linux) or `clang-cl` (Windows)
     fails naming ADR-0005 or ADR-0026;
   - configure with the version guard forced (a `-DVOE3D_CHECK_FAKE_CLANG_VERSION=17`
     hook that `voe3d_module()` honours only when the check script sets it)
     fails naming ADR-0005;
   - a scratch folder declaring a forbidden `DEPENDS` fails naming ADR-0022.
5. **Include discipline:** no file outside `platform/` includes an OS header
   (`<windows.h>`, `<unistd.h>`, X11, …); no folder includes a header from a
   folder its `DEPENDS` does not name.
6. **Tests pass** (mechanism is D-011; the step exists now and is a no-op until
   then).

Exit code non-zero on any failure, with the failing step named. That is the
whole contract: a card does not move to `review/` unless `cmake -P check.cmake`
exits zero on the coder's machine, and does not move to `complete/` unless it
exits zero on the principal's.

## Blast radius

**Reversibility: cheap.** The script has no consumers but people. Rewriting it
in another language changes nothing else.

## Consequences

- **One more thing to read in CMake's scripting dialect.** Accepted; it is the
  one file after `cmake/voe3d.cmake` an agent should not need to open. Its
  contract — "exit zero before `review/`" — is one sentence in the engine's
  `CLAUDE.md`.
- **Negative tests need a hook in the build.** `VOE3D_CHECK_FAKE_CLANG_VERSION`
  is test scaffolding inside `voe3d_module()`. Small, and the alternative —
  keeping an old Clang around to test the guard — is worse.
- **Step 5 is a grep, not a compiler.** It catches includes, not link-time
  reach. Combined with `voe3d_module()`'s map check, that closes the gap:
  the map check stops a folder *linking* what it should not; the grep stops it
  *including* what it should not.
- **Runtime grows with the folder count** — eight standalone configures plus
  the root. Configure-only steps are seconds each under Ninja; acceptable for a
  once-per-card script. If it becomes a minute, the standalone configures can
  be made a `--full` option, with the default running only the root build and
  the negative tests.

## Rejected options and why

- **Option B** — two scripts, one rule; exactly the duplication this project
  keeps eliminating.
- **Option C** — a fourth required tool for the least-run script in the repo.
- **Making it a CMake *target* (`cmake --build --target check`)** — rejected
  because the script's first job is to test configures that must *fail*, which
  a target inside a configured tree cannot do cleanly.

## Questions this opens

- **Closes D-013.** Exit criterion 7 is now *defined*; *passing* awaits the
  scaffolding card.
- **D-011** owns step 6.
- **D-032** owns the `slangc` floor in step 1.
