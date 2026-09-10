# 045 — the check names the shader compiler's version

status: todo
claimed-by: -
blocked-by: -
folder: root — `check.cmake` only. **No engine code changes in this card.**
decided-by: **ADR-0112**

Written by the tech lead under the standing grant. **Not a spin-off** — it is the
next number.

**This card does not fix bug 002 and must not try to.** It is motivated by 002 and
it is right whether 002 turns out to be a compiler question or not, which is why it
is a card of its own and why report 002 stays in the inbox rather than being
archived against this one.

## Why

Every `.slang` file is compiled by `slangc` **on the machine doing the build**, and
the SPIR-V is `#embed`ded into the binary. **The Windows binary and the Linux binary
therefore do not contain the same shader bytes** — they contain the output of two
different installations of a tool whose version this project has never recorded,
floored or compared.

`check.cmake` step 1 proves clang (parsed, floored at 19, printed) and cmake
(parsed, floored at 3.28, printed). For `slangc` it runs `slangc -v`, **throws the
output away**, and prints the word `slangc` with no number. `wayland-scanner` is the
same. And neither of those two checks its exit code properly — the test is
`if(NOT code MATCHES "^[0-9]+$")`, which passes for any numeric exit code including
a failure.

The cost came due today: bug 002 contains a disassembly of what `slangc` emits for
the element vertex shader, offered as evidence that the shader is clean. It was
produced on Linux by the Linux `slangc`, about a fault that exists only on Windows.
Nothing in the tree made that visible to anybody.

## What to do

All of this is in `check.cmake`, step 1 (`set(step "tools")`), and nowhere else.

1. **Fix the exit-code test for `slangc` and for `wayland-scanner`.** Both currently
   accept any numeric code. Match what clang and cmake do a few lines above:
   `NOT code MATCHES "^[0-9]+$"` for *could not run*, then `NOT code EQUAL 0` for
   *ran and failed*, with the two messages saying different things.

2. **Parse `slangc`'s version out of what it actually prints, and print it in the
   tools line** beside clang's and cmake's, so the line reads
   `clang 22, cmake 3.31, slangc <version>` instead of ending in a bare word.

   **Do not assume the format.** Run `slangc -v` and look at it — the clang and
   cmake parses above were each written against that tool's real output and this one
   is no different. Note that some builds of `slangc` print their version banner on
   **stderr**; `run_capture` already concatenates `out` and `err`, so that is handled,
   but it is the reason to look rather than guess.

   If it cannot be parsed, `step_fail` with the text it did print — the same shape
   as `"could not parse a version out of: ${text}"` above. A tool that will not say
   what it is fails the check.

3. **Floor it at `2026.13.1`.** The number is measured and it is in — see below —
   so there is nothing for you to collect. Write it as a named variable next to the
   parse, the way `clang_major` and `cmake_version` are used a few lines above, and
   `step_fail` beneath it with a message that names both numbers.

   **The two machines are two years apart and that is the whole reason this card
   exists:**

   | | `slangc` |
   |---|---|
   | This Linux workstation | **2026.13.1**-1-g84792eb15 — recorded in card 009's notes and nowhere since |
   | The principal's Windows machine | **2024.17**-1-g839bc9aa, measured 2026-09-10 |

   **The floor is the newer one, which is unusual and deliberate.** ADR-0112 said *the
   older of the two, where both are known good*. **2024.17 is not known good** — it is
   the compiler that built the binary bug 002 is about, and no shader in this engine
   has ever been verified under it. 2026.13.1 is the only version anything here has
   been checked on. Flooring at the suspect would write the fault into the build as a
   supported configuration.

   **This will fail step 1 on the principal's machine until he updates**, and that is
   the correct behaviour and not a regression: it is the check telling him something
   true that nothing told him for two years. **Say so in the commit message**, because
   a check that starts failing on somebody else's machine without warning is how a
   check gets ignored.

   **Compare the versions as versions, not as strings** — `VERSION_LESS`, the way the
   cmake floor a few lines above does it. These two particular numbers happen to
   compare correctly either way, which is exactly why a string compare would survive
   review here and be wrong later: `2026.9` is *greater* than `2026.13` as a string
   and *less* as a version, and that is a comparison this floor will meet.

4. **Check the `slangc` the build actually uses, not the one on `PATH`.** This is a
   hole in the card as first written and it matters more than it looks.

   `cmake/voe.cmake:331` does `find_program(VOE_SLANGC slangc)` and compiles every
   shader with **that** result, which is a cache variable a programmer can override
   with `-DVOE_SLANGC=...` — and has good reason to, because the Vulkan SDK's bundled
   `slangc` lags the standalone Slang releases and pointing the build at a newer one
   without disturbing the SDK is the obvious move. Step 1 meanwhile runs bare
   `slangc` off `PATH`. **Normally the same file; exactly not the same file on the
   machine this card is about.**

   A check that proves a different compiler from the one that produced the SPIR-V
   proves nothing, and it would report *2026.13.1, all good* about a binary built by
   2024.17. So step 1 resolves it the same way the build does — `find_program` on the
   same variable name, honouring an override — runs *that* path for the version, and
   **prints the path beside the version** when it is not the one `PATH` would have
   found. Say in a comment that this is the point of the step.

5. **Do the same for `wayland-scanner`'s version in the tools line** — parsed and
   printed, Linux only, no floor. It is a source-transforming tool on the same
   footing and it is in the same line for the same reason. If its output has no
   version in it, print what it does say rather than failing: **unlike `slangc` it
   produces no bytes that reach the binary**, so the number is documentation and not
   a dependency. Say that in a comment.

## What is out of scope, stated so you do not do it

- **No change to any `.slang` file, to `cmake/voe.cmake`, or to anything under
  `render/`.** Whatever bug 002 turns out to be, its fix is a different card.
- **No fetching or vendoring of `slangc`.** ADR-0112 rejects it, on the onboarding
  invariant: tools that transform source are installed by the programmer.
- **No checking compiled SPIR-V into the tree.** Also rejected in ADR-0112, with the
  reasons.

## Verify

`cmake -P check.cmake` exits zero on your machine, and **step 1's line now carries a
version for every tool it names**. Paste that line into the card's notes — it is the
deliverable a human can read in one glance.

Then, deliberately, **prove the check can fail**: point `slangc` at something that
is not a compiler, or temporarily set the floor above your own version, and confirm
step 1 fails with the named message rather than sailing past. Revert it. This is the
same rule card 044 was held to — a check that has never been seen to fail has not
been tested.

## The principal's half

**Already done** — he supplied `2024.17-1-g839bc9aa` on 2026-09-10, which is what set
the floor. What remains is his ordinary review, plus the one thing this card will tell
him: step 1 refusing to pass on Windows until his `slangc` is current.
