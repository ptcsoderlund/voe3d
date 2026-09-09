# 039 — the check script proves what it ran

status: review
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it comes out
of card 033's report and takes the next free number. It touches no module and can
be worked beside 031 and 032 by anyone.

There is no ADR behind this card and it does not need one. `check.cmake`'s own
header already states the property as a convention — *"A folder is discovered,
never listed: every top-level directory holding a `CMakeLists.txt` is a folder. New
folders are picked up with no edit here."* **The claim is false today.** This card
makes the script true to its own header, which is a repair rather than a decision.

## The hole, exactly as it was found

Card 033 added the `ui` folder. Reported by its coder:

> *"check.cmake needed nothing to see a new folder — step 2 globs. The root
> `CMakeLists.txt` did: it lists `add_subdirectory()` by hand, and until `ui` got
> its line every step said ok while the folder's test was never built or run. 36
> tests before, 37 after, and nothing in the output told the two apart."*

So: **the script discovers folders, the root build lists them, and nothing compares
the two.** A folder missing from the root's list still passes its standalone
configure — that step uses the folder's own `CMakeLists.txt` — and then the root
configure, build and `ctest` run happily over everything *except* it. Every step
prints ok.

**This is the worst shape a test failure can have**, and it is worth being explicit
about why: not a wrong answer, but a right answer to a question nobody asked. A
green run means *the tests that were built passed*, and the number that would have
told you otherwise is a count nobody reads and nothing asserts. Any future folder
lands with the same silence, and the first one to do it under real work will be a
folder whose tests were red for a week.

## Goal

**A run that says ok has run every folder's tests, and says which folders those
were.** After this card, a folder absent from the root build is a named failure
rather than a silent pass.

## Scope — `check.cmake`

### The membership assert

- The folder list already exists in the script, from the glob at *folder list*.
  **Read the root `CMakeLists.txt`, take the `add_subdirectory()` names out of it,
  and compare the two sets.**
- **A discovered folder missing from the root build is a failure**, naming the
  folder and saying what it means — *its tests are not being built or run* — in the
  `step_fail` voice the script already uses. That sentence is the whole point of
  the card: whoever hits it in two years should not have to work out what it
  implies.
- **A name in the root build with no such folder is also a failure**, and cheap to
  add while you are in there. It cannot happen today without breaking the root
  configure outright, so it costs one comparison and closes the direction nobody
  is watching.
- Where this goes in the run is yours: it needs the glob and it should come before
  anything expensive. Say where you put it and why.
- **Keep the parse honest.** A regex over `add_subdirectory(x)` is enough for the
  root file as it stands, and if you find yourself wanting a CMake parser, stop:
  the fix is then to make the root file simple again, not to make the check clever.
  Say in your report what your parse would do with a commented-out line or a
  variable, and make it fail loudly rather than pass quietly if it cannot tell.

### Saying what ran

- **Report the number of tests, per folder, in the step's own line.** The reason is
  the report above: 36 became 37 and the output could not be read either way. A
  reader should be able to see that a folder contributed nothing.
- **Do not assert that every folder has tests.** `dev` is a program, not a module,
  and an exemption list is the listed-not-discovered mistake in a second place. A
  visible zero is enough — a person reading the output can tell whether it is
  wrong, and the membership assert above is what catches the case they cannot see.
- If `ctest` cannot give you per-folder counts without a naming convention you
  would have to invent, **say so and print the total plus the per-folder breakdown
  you can get.** Do not invent a test-naming scheme on this card; report what would
  be needed and it becomes its own decision.

## Also, while you are here — two documentation gaps from the same report

Both pre-existing, both found by 033's coder, who correctly left them alone rather
than fixing them on the way past. They are the same class of defect as the one
above — the record disagreeing with the map — so they are folded in here rather
than made a card of their own.

- **`CLAUDE.md`'s folder table has no row for `text` or `sprite`.** Add both, in the
  voice the other rows use, saying what the folder is for in one line.
- **The tree comment above that table says the planned folders are *"each on
  `render`"*, and that is wrong for `sprite`**, which depends on `3d` — decided by
  card 022 and recorded in `cmake/voe.cmake`'s own map comment. Correct the
  sentence to match the map. **Do not change the map**: the map is right and the
  comment is stale.
- Nothing else in `CLAUDE.md` is in scope. If you spot a third thing, report it
  rather than fixing it.

## What must not change

State in your report that you checked each of these:

- **No module code, no headers, no tests inside a folder.** This card touches the
  check script and two documents.
- **The root `CMakeLists.txt` keeps its explicit list.** It is deliberate: the
  root's list says what the engine *is*, and a glob there would let a scratch
  folder join the build silently — the same class of bug as this card's, pointed
  the other way. The script is what reconciles the two, and that is the decision
  this card implements rather than a detail to improve.
- **No step is removed or reordered** beyond inserting the new assert.
- **A skip stays a pass.** The script's contract about absent tools is unchanged.

## Verify

- `cmake -P check.cmake` exits zero, all steps, analyser clean — the ordinary bar.
- **Prove the new assert fires.** Comment out one `add_subdirectory()` line in the
  root file, run the script, watch it fail with the folder named, put the line
  back. Say in your report which folder you used and what the failure said, quoted.
  A guard nobody has seen fail is a guard nobody knows works — 033's coder proved
  his Y guard by breaking it in a scratch copy, and that is the standard here.
- **Prove the other direction fires** the same way, with a name that has no folder.
- Paste the step lines with the per-folder counts in them, so the report shows what
  a reader will now see.
- Windows is the principal's. The script runs there too, so say whether anything
  you did is path-shaped in a way that could differ.

## Report when this lands

- Where the assert went and what its failure text says, quoted.
- What the parse does with a line it cannot understand.
- What per-folder counts you could get out of `ctest`, and what a naming convention
  would have bought if you had been allowed to invent one — that is the input to
  deciding whether it is worth having.
- The two `CLAUDE.md` corrections, and anything else you found and left alone.

## Notes — claude-opus-5 (kanban-coder), 2026-09-09

### Where the assert went

**A new step `1b folders in the root build`, between the folder-list glob and step
2.** It needs the glob and nothing else, reads one file and compares two lists, so
it costs milliseconds and lands before the thirteen configures of step 2 — a
folder that is not in the build is named in a second rather than after a whole
build has succeeded around it.

**It is `1b` and not `2` on purpose: nothing was renumbered.** `CLAUDE.md`'s rule
8 names step 6 and step 7 by number, and renumbering would have made that rule
wrong — which is this card's own class of defect. The script already uses `4a`,
`4b`, `4c` and `6b`, so a letter was the existing way to insert one.

The failure text, quoted from a real run with `add_subdirectory(ui)` commented
out:

```
FAIL  folders in the root build
      ui/ holds a CMakeLists.txt but the root CMakeLists.txt never adds it.
      Its tests are not being built and not being run, and every step below this one would have said ok without them.
      Add `add_subdirectory(ui)` to the root CMakeLists.txt.
```

And the other direction, with `add_subdirectory(physics)` added:

```
FAIL  folders in the root build
      the root CMakeLists.txt adds physics, and there is no physics/ holding a CMakeLists.txt.
      Either the folder was removed and its line was not, or the name is misspelt.
```

### What the parse does with a line it cannot understand

It refuses, loudly, in four ways — and each was proved by breaking the root file
and watching it fail, not by reading the code:

| The root file holds | What happens |
|---|---|
| `# add_subdirectory(ui)` | **Not listed.** A commented-out line is skipped, which is exactly the case the card is about — proved above. |
| `add_subdirectory(${extra})` | *"`add_subdirectory(${extra})` is not a plain folder name, so this step cannot say which folder it adds."* |
| `#[[ … ]]` anywhere | *"the root CMakeLists.txt holds a bracket comment, and this step cannot tell what one hides."* |
| `if()`, `foreach()`, `while()`, `macro()`, `function()` | *"a folder inside it is not unconditionally in the build."* |

The last two are the ones that matter: a bracket comment can hide an
`add_subdirectory` on a line that does not itself begin with `#`, and a folder
inside an `if()` is not unconditionally in the build. Either would let the parse
report a folder as listed when it is not — a false green, which is the bug this
card exists to remove. **Both fail rather than guess**, and the message says the
fix is to make the root file simple again rather than to make the check cleverer.

### Per-folder counts, and the naming convention I did not have to invent

**None was needed: it already exists.** `cmake/voe.cmake` registers every test as
`add_test(NAME ${folder}/${test_name} …)` so that `ctest -R math` runs one
folder's. The step reads those names back out of ctest's own progress lines from
the run it just did — no second `ctest -N`, nothing invented, and no exemption
list.

What a reader now sees:

```
ok    folders in the root build (12: 3d assets base dev ecs math platform render scene sprite text ui)
ok    tests (38 passed — 3d 6, assets 6, base 2, dev 0, ecs 3, math 5, platform 2, render 6, scene 3, sprite 1, text 3, ui 1)
```

`dev 0` is the visible nought the card asked for and is not a failure. **The
breakdown is required to add up to the total ctest reported** — otherwise the
step fails saying so, because a breakdown that quietly covers fewer tests than
ran would be this card's own bug wearing a different hat. It can only diverge if
`voe.cmake` stops naming tests that way or ctest's progress lines change shape.

### The two `CLAUDE.md` corrections

- **Rows added for `text` and `sprite`.** I drafted both from memory first and
  they were wrong; `cmake/voe.cmake`'s `voe_allowed_deps` is the authority and
  they were corrected against it before anything was run — `text` is
  `render, math, base` (not `assets`) and `sprite` is `3d, render, math, base`
  (not `scene`, not `ecs`).
- **The tree comment.** *"text · sprite · ui  each on render, low level"* became
  two lines: `text · ui` on render, and `sprite` **on `3d`, not beside `text`,
  because it hands back a material** — which is what card 022 decided and what
  the map already says. **The map was not touched.**

### Found and left alone

- **`CLAUDE.md`'s folder table still has no row for `dev`,** though the map has a
  deliberate, commented one for it. That is the third thing and the card says to
  report it rather than fix it, so it is reported.
- **`testing/` is a directory with no `CMakeLists.txt`,** so the discovery rule
  correctly does not call it a folder and the new step's `12` does not count it.
  It is an INTERFACE target defined inside `voe.cmake` and needs no row in the
  map — not a defect, written down because a reader seeing `12` may wonder where
  it went.
- **`app` has a row in both the map and the table and no folder yet.** Planned,
  not stale.

### What must not change — checked

- **No module code, no headers, no tests inside a folder.** The diff is
  `check.cmake` and `CLAUDE.md`, and nothing else.
- **The root `CMakeLists.txt` keeps its explicit list**, unchanged. It was edited
  four times to prove the guards and restored from a copy each time; `diff`
  against that copy is clean.
- **No step removed or reordered.** One inserted, as `1b`, and every existing
  number is where it was.
- **A skip is still a pass.** The new step has no tool to be absent and cannot
  skip; nothing about step 4a changed.

### Verified on

`cmake -P check.cmake` exits zero on **Linux** (Fedora 44, clang 22.1.8, CMake
4.3.0, Ninja 1.13.2) — all steps, 38 tests, analyser clean.

**Windows: one thing is line-ending-shaped and it was tested rather than
assumed.** The parse reads the root file and splits on `\n`, so a CRLF checkout
leaves a `\r` on each line. `string(STRIP)` removes it, the captured folder name
comes back as `[ui]` with no `\r`, and a commented-out line is still recognised as
a comment — checked with a three-line CMake script rather than reasoned about.
Nothing else in the step touches a path: it reads one file by `${root}` the way
every other step already does, and the test-name parse excludes `\r` explicitly.
