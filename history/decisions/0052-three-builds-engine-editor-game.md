# 0052. Three builds: the engine, the editor, and the cooked game

- **Status:** Accepted
- **Date:** 2026-09-01
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal stated the product shape: **engine → editor → cooked game, and
cooking means building again.** Pressing Play in the editor makes a debug build
and runs it.

Until now the pipeline had no written shape. ADR-0009 put the editor on the
*later* list and ADR-0010 established that authoring is text and shipping goes
through a build step, but neither said what the build products are or who runs
them. That gap is why the shader question (ADR-0046) looked like a contradiction
for one session: a rule written about "the shipped product" cannot be applied
when nobody has said what the shipped products are.

Three earlier decisions constrain the space before options are weighed:

- **ADR-0010** — authoring is text, shipping is binary via a build step. A cook
  step is therefore already required; the only question is what it emits.
- **ADR-0021 and ADR-0040** — tools that transform source are installed by the
  programmer, and *end users install nothing*. Anything that puts a compiler or
  a runtime beside the shipped game fights this.
- **ADR-0008** — folders link statically, no runtime code loading. Any option
  where a game's code is discovered at runtime reopens it.

## Options considered

### Option A — three build products, the game compiled
The engine is a static library. The editor is a program built against it. A game
is C code built against the engine, with cooked data compiled or embedded into
it. Play builds the game in debug configuration and runs it.

Costs: shipping a game requires the engine's toolchain — Clang 19, CMake,
`slangc`. Every Play pays a compile and a link, which is only tolerable because
of ADR-0055.

### Option B — the editor hosts the game, no game build
The editor loads the game's code into itself and runs it in-process, the way
Unity, Godot and Unreal's Play-In-Editor all work. Play is instant.

Costs: the game's code becomes a shared library discovered at runtime, which is
exactly the boundary ADR-0008 rejected, plus an ABI obligation between editor
and game.

### Option C — a prebuilt player binary, the game is data
One `voe_player` executable ships, and a game is data plus scripts loaded by it.
No compiler is needed to ship. This is Godot's actual model.

Costs: gameplay must live in something the player can load without compiling —
i.e. a scripting runtime, which ADR-0009 lists as a non-goal. It also means the
player binary carries every feature any game might use.

## Decision

**Option A.** The deciding factor is that it is the only option that requires no
runtime code loading, so ADR-0008 stands untouched and the shipped game keeps
the single-binary property ADR-0040 bought.

The three products, and who runs each build:

| Product | Built from | Built by | Contains |
|---|---|---|---|
| Engine | this source tree | a programmer, or the editor's build | a static library, per folder |
| Editor | the engine plus editor code | a programmer | an engine program; sideloads shaders (ADR-0053) |
| Game | the engine plus game code plus cooked data | **the editor**, on Play or on ship | one binary, shaders embedded (ADR-0046) |

**Cooking is a compile and a link, not a data-only transform.** It converts text
assets to binary (ADR-0010), generates Slang from shader graphs (ADR-0053),
compiles it, and embeds the result. The output is a program.

**Play and ship are the same build in different configurations** — debug and
release. Play is not a separate mechanism, which is what keeps "it worked in the
editor" from being a category of bug.

## Blast radius

**Moderate, and asymmetric.** Adding a build product later is cheap. What is
expensive is the reverse: if a game ever ships without the toolchain present,
that is Option C, and it needs a player binary and a scripting runtime — a
different product, not a flag.

## Consequences

- **Whoever ships a VOE3D game is a programmer, or employs one.** This is
  already implied by ADR-0010's "making things happen requires the engine's
  API", but the pipeline makes it concrete and it should be said out loud.
- **An artist's shader graph cannot reach a running game without a compiler
  run.** The editor's preview (ADR-0053) hides this during authoring; Play does
  not. If an artist-only path is ever wanted, this is the decision it breaks
  against.
- **Play is only usable if rebuilding is fast.** ADR-0055 makes that a budget
  with a number rather than a hope. These two ADRs are load-bearing on each
  other and were taken together.
- **The editor has no place in the module map.** ADR-0022 names eight folders
  and none of them is an editor; ADR-0030 added `3d`, `text`, `sprite`, `ui`
  under `render`. Whether the editor is a folder, a separate executable beside
  `app`, or its own repository is a new question — and not one for today.
- **Whether Play runs in-process or as a child process is deliberately not
  decided here.** Option B's mechanism is rejected; the *process model* for a
  game built under Option A is a separate question, parked with a trigger: it is
  decided the first time Play exists, on the measured rebuild time.
- **The scene view and the play view are different things.** ADR-0024 mentions
  "an editor viewport in a panel", which reads like in-process play but is not:
  the scene view is in-process by definition because the editor *is* the engine.
  Only the play view is at issue.

## Rejected options and why

- **Option B** was rejected on ADR-0008. Instant Play is a real benefit and this
  is the option every commercial engine took — but all of them pay for it with a
  code-loading boundary and an ABI contract, and ADR-0008 rejected that
  mechanism as premature generality when nothing asked for it. Under ADR-0055,
  what B buys is measured in seconds saved, and the budget says those seconds are
  five.
- **Option C** was rejected because it requires a scripting runtime in the
  shipped product, which ADR-0009 lists as a non-goal, and a runtime beside the
  binary, which ADR-0040 spent a decision to avoid. **This rejection is now
  live rather than settled**: the principal has since stated a direction toward
  .NET scripting in the editor. If scripting reaches *shipped game code*, Option
  C becomes available and this ADR is the one it supersedes.

## Questions this opens

- **Where the editor sits relative to the module map** (ADR-0022). Not today.
- **The Play process model** — in-process or a child process. Parked, with the
  trigger above.
- **Whether scripting reaches shipped game code, or stops at the editor.** The
  principal's stated direction is .NET scripting for the editor, with the
  language left open. Which side of the ship boundary that lands on decides
  whether Option C above is dead or merely deferred, and it collides with
  ADR-0009's non-goal, ADR-0023's dependency default, and ADR-0040's
  install-nothing rule. Needs its own ADR before any editor card is written.
