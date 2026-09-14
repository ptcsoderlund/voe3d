# 0140. A recoverable problem is reported through one call in `base`, and where the lines go is a later step

- **Status:** Accepted
- **Date:** 2026-09-12
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-247**, opened by ADR-0138: whether `base` gains a reporting facility with levels,
where its lines go — stderr, a file, the editor's own console — and whether a shipped game
prints at all.

What is on disk, counted the same day. **There is no facility: 108 calls to
`fprintf(stderr, ...)` are written by hand at the site of the problem** — 82 in `render`,
12 in `dev`, 9 in `assets`, 3 in `ui`, 2 in `3d`. They already share a shape,
`"<module>: <detail>\n"`, arrived at by convention and held to without exception.
`base/src/assert.c` also prints, but an assert is the fatal path and not this question;
`testing/include/testing/test.h` prints a failed check, which is the test framework's own
business. `base/include/base/error.h` fixes the division this ADR sits under — a code is a
category, and **the message is written at the site where the detail still exists** — and its
own worked example is a hand-written `fprintf`.

**What forces the question now.** ADR-0138 point 4 has a drain print `warning:` when it
corrected a value and `error:` when it kept the last valid row, and says that line is *"the
contract a later reporting facility must replace rather than sit beside."* Cards 053, 056
and 060 write those drains. The point of the line is to tell **the person who dragged the
number**, and by cards 058 and 059 that person is in the editor's window and has no terminal.

**Constraints that kill most answers.** A third-party logging library is the principal's
call under ADR-0023 and has not been asked for, so anything here is ours. `base` is the
lowest folder and may not learn what an editor is. Rule 10 — surface with no caller is not
built. A cooked game never drains (ADR-0128), so drain reporting is an authoring-time
concern; every other line here is not.

## Options considered

### Option A — write the convention down, change no code
The prefix, the module name, ADR-0138's two words, recorded in `base/error.h`. 108
hand-written prints stay. Free today; the editor's console can never show these lines
without touching all 108 sites at that point.

### Option B — one call in `base`, printing to stderr
`base` gains a reporting call taking a level, a module and a message. It writes to stderr.
The output changes only by gaining the level word. One place to change when the destination
changes.

### Option C — the full facility now: levels, and a destination the program installs
The editor installs a console sink; a game installs a file or nothing. This is the shape the
engine ends at. Today it has no caller: the editor's console panel is not a card, is not
decided and has not been asked for, and a sink callable from anywhere raises threading
questions nothing has answered.

## Decision

**Option B, taken explicitly as the first step of C.** The principal: *"We are aiming for C
right? … can we somehow break it up and progress slowly over time?"* The deciding factor:
**the expensive part of C is not the destination-swapping machinery, it is the 108 call
sites** — so B is the difference between that later change being one file and being 108,
and it buys the indirection without the filters, categories and sinks that this kind of
system grows before anyone needs them.

1. **`base/include/base/report.h` holds one call.** A level enum with exactly two values,
   warning and error; a function taking the level, the module's name, the site's file and
   line, and a printf-style message; and two macros that supply the level and the site.
   The function is public because the macros expand to it and is not meant to be called
   directly, exactly as `voe_base_assert_fail` is.

2. **The module is its own argument, not text inside the message.** `("render", "…")`, not
   `"render: …"`. This is what lets a later console filter by module without parsing
   strings back apart.

3. **The site is captured and not printed.** The macros carry `__FILE__` and `__LINE__`;
   today's line does not show them. A console panel that can point at the source is an
   obvious thing to want at step 2, and capturing it later would mean touching every call
   site a second time — which is the cost this ADR exists to pay once.

4. **The line is composed into a fixed buffer on the stack and written once**, then
   truncated if it does not fit. A reporting path must not allocate and must not fail. One
   write also keeps a line from interleaving with another thread's. Composing the finished
   line — rather than printing it in pieces — is what makes step 2 a change inside this one
   function.

5. **The composed line is `<level>: <module>: <detail>`**, which is ADR-0138 point 4's
   shape applied to every report rather than only to a drain's. **This changes today's
   output**: 108 lines gain a leading `error: `. Accepted, and preferred to two shapes on
   one stream — the level is then greppable at column nought and a drain's line is not a
   special case.

6. **The arguments stay checked against the format string.** `[[gnu::format(printf, …)]]`,
   in the C23 attribute spelling the codebase already uses for `[[noreturn]]` and
   `[[nodiscard]]`. The compiler's checking is the one real thing wrapping `fprintf` would
   otherwise cost.

7. **Every existing site moves, in one card.** A half-migrated engine is worse than an
   unmigrated one: at step 2 the console would show some of the engine's problems and be
   believed about all of them. `base/src/assert.c` and `testing/test.h` are not reports and
   do not move; `base/error.h`'s worked example is rewritten to the new call.

8. **A shipped game keeps printing.** It costs nothing and a game that prints is a game
   whose bug reports contain something. What may change later is where the lines go, not
   whether there are any.

9. **A level below warning is added when a caller exists.** Not before (rule 10).

**The remaining steps, and what each waits for.** Step 2 — the program installs where lines
go, and the editor installs its console — waits on the editor's console being a card
(D-248). Step 3 — what a shipped game does with a destination — waits on a game somebody
ships. Neither is surfaced until then (ADR-0129).

## Blast radius

**Moderate, and deliberately front-loaded.** After the migration card there is one call, so
the destination, the format and the levels are each a change in one file that no call site
sees. Reversing to A means unpicking 108 sites again; going on to C means editing
`report.c`. Point 5's line shape is the part that is public — anything reading the engine's
stderr sees it.

## Consequences

- **Two cards ahead of nothing on the board**, and the second touches six folders.
- **`dev/src/main.c` is one of the six and card 052 has it open**, so the migration card is
  blocked behind 052 rather than merely later than it.
- **Output changes on the day the migration lands** — every line gains a level word. Anything
  a person greps for by hand changes with it.
- **The site is captured and shown to nobody**, which is data with no consumer until step 2.
  Accepted under point 3's reasoning and worth naming as the one place this ADR spends
  ahead of a caller.
- **`base` grows a fifth public header.** It is the right folder — assert already prints from
  there — and the call knows nothing above it.
- **A drain's line stops being special**, so ADR-0138 point 4 is satisfied by the ordinary
  call rather than by a rule a drain has to remember.
- **A fixed buffer truncates a long message.** No line in the engine today comes near it, and
  the alternative is allocating on the failure path.
- **Nothing gets quieter.** ADR-0138 point 6's edge-triggering is the drain's own doing and
  this call does not know about runs; a caller that would print every frame still does.

## Rejected options and why

**A — convention only.** It is free today and charges the whole 108-site bill at the moment
the editor's console is wanted, which is the moment we will least want to pay it. The
convention it would record is already held without exception, so it buys nothing that is
not already true.

**C — the full facility now.** A destination with no second destination, and a sink with no
subscriber. The editor's console is two cards away at the earliest and undecided in shape,
and building the mechanism first would fix its shape by guess. Taken as step 2, not
rejected — this ADR is the first half of it.

## Questions this opens

- **D-248** — where a reported line goes: whether the program installs a destination, what
  the editor's console is, what a shipped game does with it, and what threading a destination
  called from anywhere implies. Parked under *the first editor card … and the editor's own
  later cards*.
