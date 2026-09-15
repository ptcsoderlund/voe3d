# 0118. Async loading is opt-in per load, and game logic never reads an async asset

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal asked for asynchronous asset loading and, asked to choose a mechanism,
declined and described the product instead — which was the right call, because the
mechanism follows from the contract and not the other way round. In his words:

> Lets say we have a finished game. The game starts and a scene loads. All physics shapes
> and entities gets loaded, the game starts. The game runs smoothly even though 3d models
> with animations and textures are being loaded in async. We get no stuttering and the
> models starts popping up until everything is loaded. That is ok since its the ECS system
> and the physics shapes that decide game logic. So yeah, even cpu decoding and stuff
> should be included in the async loading. We should have it as opt-in, meaning sync
> loading is default and dev can pick which ones to load async. We get both options.

Five requirements are in that paragraph: loading splits into a tier that gates the game
starting and a tier that does not; the dividing line is whether game logic reads the
asset; popping in is acceptable rather than a degraded mode; CPU decoding is inside the
asynchronous boundary and not merely the GPU upload; and the facility is opt-in with
synchronous loading as the default.

What depends on this: the shape of every loader call `assets` will ever expose, whether a
component may hold an id whose data has not arrived, what the draw does when it meets one,
and whether the engine grows a second thread at all. **This ADR is the contract. ADR-0119
is the mechanism under it, deliberately separate, because the contract is load-bearing and
the mechanism is not.**

### Hard constraints already fixed

- **Nothing in the engine opens a file, by decision rather than by omission.** `platform`
  owns files and has no API for them (ADR-0022); a decoder takes a byte range and never a
  path, decided for the JSON reader (ADR-0054) and inherited by the authored text format
  (ADR-0073). Every asset `dev` uses arrives by `#embed` at build time, as shaders do
  (ADR-0046, ADR-0053) and as the default font does (ADR-0062, which rejected loading from
  disk for exactly this reason). Card 017 states the working rule verbatim and names its
  own trigger: *"The card that gives `platform` a file API is the card that makes this take
  a path, and nothing in `assets` changes when it does."* **There is therefore no read to
  make asynchronous yet, and that card is this decision's prerequisite.**
- **One thread until an ADR says otherwise** — ADR-0065 rule 6, whose wording is *"a card
  does not introduce a thread; a decision does"*, and whose reversibility is recorded as
  load-bearing for the single thread.
- **The intent queue is the sanctioned concurrency seam.** ADR-0013 answered *do we need to
  plan for multithreading now* with *the plan is the type*; ADR-0017 records that queued
  intents are safe to submit from several threads and drain in one place.
- **Arenas are not thread-safe and the sanctioned shape is one per thread or per job**
  (ADR-0032 rule 6), whose own revisit condition is *if and when there is a job system*.
  Allocation failure is fatal and never returns NULL (rule 5), with a carve-out for a
  large, externally-sized allocation.
- **A GPU resource is named by a generational id that *is* the shader's index,** with no
  table between a caller's id and the GPU's view of it (ADR-0018, ADR-0060). A stale id is
  refused at draw and logged. **Geometry behind an id is already replaceable** (ADR-0085).
- **The upload waits for the GPU to go idle** and its header calls itself a startup
  operation. A wait-for-idle stalls the *device*, not the calling thread, so no arrangement
  of threads hides it.
- **Loading was priced as a one-time startup cost** — *"loading happens once, off the
  frame; a slow glTF reader costs a second at startup"* (ADR-0025). **Hot reload is not
  pursued** (ADR-0055). **The engine caches nothing and building a cache is a decision**
  (ADR-0084 rule 4).
- **Recoverable failure is a value checked at the call site** (ADR-0041). There is no
  deferred-failure concept anywhere in the engine.
- **Opt-in is this project's standing preference** where a capability can be had either
  way.

## Options considered

### Option A — one loading path, and it is the asynchronous one

Every load is a request that completes later. No synchronous call exists; a caller that
wants to block waits on its own request.

Fewest concepts and one code path per decoder to test. But it makes every caller
asynchronous to serve the few that need it: `check.cmake`'s tests, the headless device's
tests, a future editor importing one file on a menu click, and the sync tier the principal
explicitly wants for physics shapes all become request-and-wait ceremony. It also forces
the thread, the completion machinery and the pending-id question to exist before anything
can load anything, which puts the prerequisite file-API card behind three decisions
instead of none.

### Option B — two tiers, and the engine decides which by asset kind

The engine classifies: textures and render meshes are always asynchronous, collision
shapes and scene structure always synchronous.

Needs no new API surface at all — the existing calls keep their signatures and change
behaviour underneath. But it takes the choice away from the programmer, and the
classification is wrong as often as it is right: a 32×32 icon needed on the first frame is
a texture, and a 200 MB collision mesh is a shape. It also means an existing call silently
changes meaning, which is the worst way for this to arrive.

### Option C — two tiers, and the caller decides per load call

Synchronous loading keeps the calls and the behaviour it has today. An asynchronous load is
a *different call* that hands back its id immediately and fills it in later. The
programmer picks per asset.

The engine has done exactly this once before and it worked: `voe_render_geometry_create`
and `voe_render_geometry_create_transient` are two calls with two lifetimes returning the
same id type, and `render/include/render/device.h` records the property that makes it safe
— *"what holds one never has to know which kind it holds"*. Two calls rather than a
boolean, for the same reason: a flag at a call site reads as a tuning knob, and this is a
different contract.

## Decision

**Option C.** The deciding factor is that the tier is a property of the *call site's
needs*, not of the asset's type — the same picture is blocking for a loading screen and
deferred for scenery — and only the caller knows which it is.

The contract, in rules:

1. **Loading has two tiers and the line between them is whether game logic reads the
   asset.** The synchronous tier gates the game starting: entities, physics shapes, and
   anything the simulation reads. The asynchronous tier does not gate anything.
2. **Synchronous is the default and asynchronous is asked for, by a different call.** Not a
   flag, not a mode, not a setting. Existing loader calls keep their behaviour, so nothing
   already written changes meaning.
3. **An asynchronous load hands back its id at request time.** The id is valid to store in
   a component immediately. This is affordable only because ADR-0018 made the id the
   shader's own index with nothing in between, and ADR-0085 already made what sits behind
   one replaceable.
4. **An entity whose visual asset has not arrived is simulated and not drawn.** Popping in
   is the specified behaviour and not a failure. There is no placeholder mesh and no
   placeholder texture — the engine draws nothing rather than something wrong, and a
   program that wants a stand-in loads one synchronously and swaps it.
5. **Game logic must never read an asynchronous asset.** This is the invariant the whole
   contract rests on: it is what makes the simulation deterministic and identical whether
   an asset arrived in frame 3 or frame 300, and it is why rule 4 is acceptable rather than
   a bug. A system that needs a mesh's vertices to decide anything is reading the wrong
   tier and the asset belongs in the synchronous one.
6. **The wait for the GPU to go idle goes.** It is not a consequence of this decision so
   much as the price of it: it is the largest single stutter in the current path and it
   cannot be threaded away. *How* it goes is undecided and is its own question.
7. **The engine still caches nothing** (ADR-0084 rule 4). An asynchronous load is not a
   cache, does not deduplicate, and asking for the same file twice loads it twice. A
   caller that wants otherwise keeps its own table, exactly as rule 5 of that ADR allows.

## Blast radius

**Reversibility: load-bearing**, and specifically rule 5. Every system written after this
is written assuming it may not read an asynchronous asset; reversing that means auditing
every one of them for a dependency the compiler cannot see. Rules 1–4 are moderate — they
shape the loader API and the draw's skip test, both of which are small and local. Rule 6 is
structural inside `render` and costs nothing outside it.

Rule 2 is the cheap half: because synchronous keeps today's calls, this ADR can be recorded
now and nothing in the tree becomes wrong.

## Consequences

- **There is nothing to build until `platform` can open a file.** The first card out of
  this ADR is the file API, not the loader. That is not a delay this decision caused — it
  is a hole three earlier decisions deliberately left, now load-bearing for the first time.
- **A pending id and a dead id become indistinguishable, and that is a new way to hide a
  bug.** Today the draw refuses both and logs. Under rule 4 "not ready" becomes normal and
  silent, so a genuinely stale handle in a component — a destroyed asset, a load that
  failed — disappears behind *it is probably still loading*. The draw needs three states
  where it has two. Opened as D-224 and it is not optional.
- **The physics tier may not be as cheap as the vision assumes.** A collision mesh is
  often geometry inside the same `.glb` as the thing you look at. If rule 1 requires the
  shape before the game starts and the shape shares a file with the visual mesh, that file
  is read and decoded synchronously on the main thread and only its GPU upload was
  deferred — which puts the stutter back at scene load. The likely answer is that shapes
  are authored as primitives or as their own asset, which is what shipping games do anyway
  and what a render mesh makes a bad collider argues for. It is an authoring question, there
  is no physics in the engine yet, and it is opened as D-225 rather than guessed at here.
- **Popping in is visible to the player.** The principal accepted this explicitly. It is
  worth writing down that the acceptance is of *this* engine's behaviour and not of any
  particular game's art direction; a game that cannot tolerate a hole in the world loads
  that asset synchronously, which rule 2 makes a one-word change.
- **Two loading paths mean two paths to test per decoder**, and the asynchronous one is the
  harder to test because it is timing-dependent. The headless device exists and is the
  right place for it.
- **A background loader that exhausts memory takes the process down**, because ADR-0032
  rule 5 makes allocation failure fatal. Acceptable for embedded startup assets; it is
  exactly the case that rule's own carve-out for a large externally-sized allocation was
  written for, and a streaming loader will need that carve-out rather than the abort.
- **Animation is not actually supported.** The vision says "3d models with animations", and
  `assets/include/assets/model.h` ignores animation rather than refusing it. Nothing here
  is blocked by that, but the vision is describing a capability that does not exist yet and
  this ADR does not create it.
- **Nothing here is hot reload** (ADR-0055) and nothing here makes it closer. A load that
  completes later is not a file watched for changes.

## Rejected options and why

- **Option A, everything asynchronous.** Rejected because it taxes every caller to serve a
  few, and because the principal asked for both options by name. It also inverts the
  dependency order: it cannot be built until the thread, the completion machinery and the
  pending-id question are all decided, where Option C can be recorded today against a tree
  that stays correct.
- **Option B, the engine classifies by asset kind.** Rejected on two counts. The
  classification is wrong often enough to matter — a small icon needed immediately is a
  texture and a huge collision mesh is a shape — and it removes the choice the principal
  specifically asked to keep. Worse, it changes what an existing call does without changing
  how it is spelled, which is the failure mode ADR-0085's generation check exists to make
  loud rather than silent.

## Questions this opens

- **D-224** — what the draw does with an id that is pending rather than dead, given rule 4
  makes the first normal and the second a bug, and they are the same thing today.
- **D-225** — whether physics shapes are authored separately from render meshes, or the
  loader must be able to take a shape out of a file and defer the rest of it.
- **D-226** — the shape of `platform`'s file-read call, which everything here waits on.
- **D-227** — how the upload stops waiting for the GPU to go idle: a dedicated transfer
  queue with an ownership transfer, or a fence on the queue the device already has.
- **D-228** — what a failure that arrives after the call site has returned does, given
  ADR-0041 has no row for one.
- **D-229** — the order several in-flight asynchronous loads complete in, and whether a
  caller may ask for one before another.
