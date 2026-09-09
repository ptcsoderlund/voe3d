# 039 — the check script proves what it ran

status: todo
claimed-by: -
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
