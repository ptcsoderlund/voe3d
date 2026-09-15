# 028 — geometry that changes

status: review
claimed-by: claude-fable-5-1 (kanban-coder)
blocked-by: - (ADR-0085 accepted 2026-09-07; this card is claimable)

Written by the tech lead under the standing grant. **Not a spin-off**: it is the
card that follows 027, so it takes the next number rather than a letter.

The decisions behind this card are **ADR-0084**, **ADR-0085** and, since
2026-09-08, **ADR-0098**. Everything you need from all three is restated here and
you should not have to go and read them.

## This card is claimable

**ADR-0085 was accepted on 2026-09-07** and the `3d` section below is in scope.
It is the small decision that lets a new geometry id reach the mesh table;
without it the last third of this card would have had no mechanism.

## Amended 2026-09-08: who opens the frame

**The first coder on this card found a contradiction and stopped, correctly.**
The transient create below requires an open frame, but the only code that opened
one was `voe_3d_draw_system_run`, which called `voe_render_frame_begin`, walked
the tables and called `voe_render_frame_end` inside one call. So the dev program
had no moment at which to build its readout. That is resolved by decision
(ADR-0098), not by loosening the create:

- **The program's loop opens and closes the frame.** `voe_render_frame_begin` and
  `voe_render_frame_end` are called from the loop in `dev/src/main.c`, exactly as
  the example at the top of `render/include/render/device.h` has always shown.
- **The draw system draws into the frame that is open**, and asserts if none is —
  the same rule `voe_render_frame_draw` already applies. It no longer begins or
  ends anything and it loses its `size` parameter; the size goes to `_begin`.
- **Inside the frame the order is fixed**: begin; build what changes this frame
  (this card's readout, later the GUI); the draw system walks the world; end,
  which presents. Building what changes is the only world write after the
  systems have run, and it is the direct `voe_3d_mesh_set_geometry` call this
  card adds.
- **A begin that comes back with `drawing == false` is the loop's case**: no
  transient build, no draw system, no `_end`. The header documents it; the loop
  follows it.
- **Do not** let `_create_transient` wait on a fence so it can run between
  frames. That was the other way out and it is rejected in the ADR for the reason
  this card already gives under *likely to go wrong*: a wait inside the create
  means the pools are not per-slot.

The `3d` and `dev` scopes below have been extended with the concrete edits.

## Goal

Geometry can be built during a frame, drawn in that frame, and forgotten — so
that **the frame statistics leave the console and go on the screen**, where card
020 said they would go once this existed.

Three things get this, and only one of them is text: the statistics readout,
card 023's GUI, and debug drawing, which has never had a path at all.

## The one thing to understand before starting

**There are now two lifetimes of geometry and the id type does not distinguish
them.** That is deliberate and it is what keeps the draw path single.

- **Static** — `voe_render_geometry_create`. Waits for the card to go idle,
  stages through a temporary buffer, appends to a device-local pool with no
  destroy. A startup operation. **Unchanged by this card.** A model, a fixed
  label, the dev scene's cubes all keep it and keep costing exactly one upload.
- **Transient** — new here. Host-visible, mapped, written directly, **valid until
  the end of the frame** and then gone. No idle wait, no staging, and nothing to
  exhaust because it is reset.

**An id from the second kind, used after its frame, is refused by the staleness
check that already exists** — `voe_render_geometry_at` compares the slot's
generation against the id's. You are not building a new refusal path. You are
making the existing one fire on a schedule.

**The engine caches nothing and this card must not add a cache.** The principal
asked directly whether geometry should be cached until the text changes, and the
answer was no, for the invalidation reason rather than a performance one: a cache
that misses an invalidation draws yesterday's text, which looks like the program
has frozen. Fixed text already has its cache — it is the static pool. If you find
yourself adding a dirty flag, a hash of the string, or a "has it changed" check
anywhere in `render` or `text`, **stop; that is this card failing.**

The cost you are not allowed to optimise away, with its arithmetic so you can see
it is small: two thousand glyphs is about eight thousand vertices, ~256 KB written
per frame, ~15 MB a second at sixty frames, into memory that takes gigabytes a
second. Roughly a tenth of a millisecond, against the 1–3 ms per thousand objects
that *submitting* the draws already costs (ADR-0081). Streaming is not the binding
cost.

## Scope — `render`

### The pool

- **Host-visible and mapped, one set per frame in flight**, vertices and indices
  both, exactly as `frame->objects_mapped` and `frame->uniforms_mapped` already
  are (`render/src/frame.c`). The fence at the top of the frame is what makes
  reusing a slot's memory safe, and it is the same reason those two are safe. Do
  not invent a second synchronisation story.
- `voe_render_capacities` gains **`transient_vertices`** and
  **`transient_indices`** — counts, not bytes, matching the two that exist. All
  capacity is fixed at device creation and nothing grows; the struct's own comment
  says so and stays true.

### The slots

- The `geometries` array grows by **`transient_geometries`**, a third new
  capacity. Static allocation keeps walking `[0, capacities.geometries)`;
  transient allocation walks the band above it. `voe_render_geometry_at`
  bounds-checks against the total and is otherwise **untouched** — that function
  working unchanged is the point.
- A slot gains a way to say **which pool it is in**. The draw needs it (below).
- **At the top of every frame, every transient slot that is live becomes not live
  and its generation is bumped.** That is the whole reset, and it is what makes
  every id from last frame stale. Bumping only the live ones keeps the counter
  slow; at sixty frames a second a `uint32_t` still has years in it, and a wrap
  would hand back a live-looking stale id, so **note the wrap in a comment rather
  than handling it.**

### The call

```c
[[nodiscard]] bool voe_render_geometry_create_transient(
        voe_render_device *device, const voe_render_vertex *vertices,
        uint32_t vertex_count, const uint32_t *indices, uint32_t index_count,
        voe_render_geometry *out, voe_base_error *error);
```

Deliberately the same shape as the static call: the caller builds an array and
this copies it. **Handing back mapped pointers for the caller to write into is
the other design and it is not this card's** — write-combined memory that must
never be read back is a footgun, and the copy here is from cache-hot memory.

- **It requires an open frame and asserts without one**, because the slot it
  writes into is not known until `voe_render_frame_begin` has picked one. That is
  the mirror image of the static call, which is a *startup* operation — say so in
  the doc comment, in those terms, because it is the thing a reader will get
  backwards.
- **It fails, returned and not fatal (ADR-0041), when either transient pool is
  full or there is no transient slot left.** Same shape as the static call's
  refusal, same `VOE_BASE_ERROR_REFUSED`, same stderr line naming the numbers.
  This is a new place a frame can fail where nothing could fail before; the
  message should make it obvious that a capacity was too small rather than that
  something broke.
- A count of zero is the caller's bug and asserts, as it is for the static call.

### The draw

**This is the part that is easy to get wrong.** `render/src/frame.c:366` binds
*one* vertex buffer and *one* index buffer, once, at the top of the frame. The
transient pool is a different buffer, so a draw out of it needs a rebind.

- Bind lazily and track what is bound, **exactly as `device->bound` already tracks
  the pipeline** in `voe_render_frame_draw`. Do not bind per draw; do not bind
  both at the top.
- `voe_render_frame_draw` and `voe_render_frame_draw_blended` are the only entry
  points and **no third one is added.** They read the slot, see which pool, rebind
  if it changed, and draw. Everything else about them — the object record, the
  push constant, the refusal when the id names nothing — is unchanged.
- `first_vertex` still goes in as `vertexOffset` so a mesh keeps its own index
  numbering. That reasoning is in `geometry.c`'s header and applies unchanged.

### What must not change in `render`

State in your report that you checked each of these:

- **The static pool.** Device-local, bump-allocated, no destroy, GPU idle wait.
  Nothing about it moves and no `voe_render_geometry_destroy` appears.
- **`voe_render_geometry_at`'s logic.** Bounds, live, generation. Only the bound
  changes.
- **The id type.** No `voe_render_geometry_transient`, no second draw entry point,
  no flag in the id itself.
- **Two frames in flight** and the existing fence discipline.

## Scope — `text`

- **`voe_text_block_create_transient`**, the same signature as
  `voe_text_block_create` and the same layout code underneath — the only
  difference is which create it calls. Do not fork the layout.
- **The header's paragraph beginning *A TEXT BLOCK IS BUILT ONCE AND DOES NOT
  CHANGE* is now half wrong and is the sentence a reader will trust.** Rewrite it,
  do not extend it: say there are two calls, say which is a startup operation and
  which lives inside a frame, and keep the reason the static one exists — a label
  that never changes should still cost one upload.
- The sentence *"it is why the frame statistics are still printed on the console"*
  becomes false with this card. Remove it.
- **Nothing else in `text` changes.** No caching of laid-out vertices — that is
  allowed by ADR-0084 but only when something has measured that it matters, and
  nothing has. No kerning, no shaping, no atlas changes.

## Scope — `3d`

One function, under ADR-0085:

```c
[[nodiscard]] bool voe_3d_mesh_set_geometry(voe_ecs_world *world,
                                            voe_ecs_entity entity,
                                            voe_render_geometry geometry);
```

- False when the entity is not alive or has no mesh.
- **Geometry is the only mutable field.** The layer is not writable and the row is
  not replaceable wholesale — a writable layer is a way to break the depth-clear
  ordering that card 024 spent a card getting right.
- **Rewrite the header's reasoning, do not extend it.** `voe_3d_mesh_add`'s
  comment currently says it is a direct call *because it is creation*, and that
  sentence no longer covers the table. The replacement has to say what the actual
  boundary is: this table has one writer per entity and nothing to coordinate
  between systems, so it is written directly; the day two systems both want to
  write a mesh row, that is an ADR and not a patch.

And the draw system, under ADR-0098:

- **`voe_3d_draw_system_run` no longer calls `voe_render_frame_begin` or
  `voe_render_frame_end`** (`3d/src/draw_system.c:248`, `:330`). It computes the
  view and the sun as it does now, hands them — see the next point — and draws.
- **The `size` parameter is removed.** `_begin` takes the size and it is the
  loop's to pass. The view and the sun stay where they are computed today; if
  `_begin` needs them before the draw system runs, expose what the draw system
  computes as one call the loop makes just before `_begin` — do not compute a
  camera twice and do not move the camera into `dev`.
- **Calling the draw system with no frame open is the caller's bug and
  asserts**, with `VOE_BASE_DEBUG_ASSERT`, the same way `render` treats a draw
  without a `_begin`.
- **The header's first sentence, *One frame, one call*, is now false. Rewrite it,
  do not extend it**: the draw system draws the world into the frame the loop has
  opened, and the loop is where the frame's phases are ordered
  (`3d/include/3d/draw_system.h:1`). Its paragraph about a window with no area
  drawing nothing and returning true moves to describing `_begin`'s `drawing`
  flag as the loop's concern.

## Scope — `dev`: the statistics on the screen

**This is the proof, and it is what the principal will look at.**

- The four timing blocks currently printed by `report()` (`dev/src/main.c:1393`)
  get drawn in the **overlay layer** as text, rebuilt every frame from the current
  numbers through the transient path.
- **Keep the console output.** It reports on a period with averages and worsts and
  that is a different, still-useful thing; this card adds a screen readout, it does
  not move one. If the two ever disagree, say so in your report.
- The overlay layer, the material and the placement all already work — the dev
  program's heads-up line does exactly this today with a static block
  (`dev/src/main.c:1150`, `:1175`). Follow it. **Unlit, blended, base colour
  texture declared a distance field**, the five things `text/font.h` names.
- **Keep it small.** Frames per second and the four millisecond numbers is enough.
  This is a demonstration of a mechanism, not a profiler UI, and a layout engine
  is not in scope.
- **The loop's shape, under ADR-0098** (`dev/src/main.c:1791`): after the
  `update` sample, call `voe_render_frame_begin`; if `drawing`, build the readout
  through `voe_text_block_create_transient`, put its id on the readout's entity
  with `voe_3d_mesh_set_geometry`, call `voe_3d_draw_system_run`, call
  `voe_render_frame_end`; if not `drawing`, do none of those. Break out of the
  loop on a false from `_begin`, the draw system or `_end`, as the loop does
  today on a false from the draw system.
- **The `draw` timing sample keeps its name and now spans `_begin` to `_end`
  inclusive of the transient build.** Rewrite the comment at `dev/src/main.c:1351`
  and the `say_what_is_measured()` line for `draw` so they describe what is inside
  the loop's draw phase rather than what is inside the draw system.

## Where this card is likely to go wrong

- **Forgetting the rebind, or binding per draw.** The first draws the wrong
  geometry out of the wrong buffer — usually visible as one object wearing another
  object's shape. The second is a per-draw cost nobody asked for. Track it like
  the pipeline is tracked.
- **Writing into a slot the card is still reading.** If you find yourself needing
  a barrier or a wait inside `_create_transient`, the pools are not per-frame-slot
  and that is the bug.
- **Reading back from mapped memory.** Host-visible memory here may be
  write-combined; writes stream, reads crawl. Build in your own array, copy once.
- **The reset bumping every slot rather than the live ones**, which is correct but
  wasteful at a large `transient_geometries`, and which makes the generation
  counter move much faster.
- **A capacity chosen by feel.** Pick numbers that fit the dev program's readout
  with obvious headroom, say in your report what you picked and what the readout
  actually consumes, and do not make them large "to be safe" — the pool is
  resident memory and D-119 wants a real number, which your measurement is the
  first input to.
- **Quietly adding a cache** because rebuilding the string every frame feels
  wasteful. See above. It is a tenth of a millisecond and the decision is written.
- **Building the readout before `_begin`**, because it reads naturally to prepare
  the text and then draw. The create asserts, and that assert is right: the slot
  is not known yet. The order is begin, build, draw, end.
- **Leaving `_begin`/`_end` inside the draw system and adding a callback** so the
  one-call shape survives. Rejected by ADR-0098; the loop owns the frame.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **A `render` test, headless**, that builds transient geometry, draws it, ends
  the frame, and then confirms the id is refused. Then a second frame with
  *different* contents at the same slot, confirming the picture changed. The
  headless device exists for exactly this (`voe_render_device_new_headless`) and
  `render/tests/pools.c` is the neighbour to follow.
- **A test that the static path still works alongside it in the same frame**, both
  pools drawn from, which is the rebind path exercised.
- **The existing `3d` and `dev` builds still pass with the draw system drawing
  into a frame the caller opened** — the headless device is how `3d`'s tests, if
  any open a frame, get one.
- **A test that overrunning a transient pool is refused and does not corrupt the
  frame** — the frame after it must draw correctly.
- **Screenshot the dev program with the statistics on screen**, and let it run long
  enough that the numbers visibly change. A still image of numbers proves the text
  drew; the numbers changing between two shots is what proves the card.
- **Screenshot the dev scene otherwise unchanged** — the cubes, the sprites, the
  sign, the see-through quads, the overlay. Nothing static may have moved or
  changed appearance.
- Let it run a few minutes and confirm memory is flat and the pool is not filling.
  That is the specific failure the reset exists to prevent, and it would look like
  a program that works for thirty seconds.
- Windows is the principal's.

## Report when this lands

- The three capacity numbers you chose and what the readout actually consumed.
- Whether the rebind ever fires more than twice a frame in the dev scene, and if
  so how often. That number is the first evidence for whether the two pools want
  grouping in the draw, and nobody has it yet.
- What the transient path costs per frame, if you can measure it at all against
  the existing `update` and `draw` numbers. If it is below the noise, say that —
  it is the answer ADR-0084 predicted and it is worth having confirmed.
- Anything in the comments of the files you touched that still argues for text
  being built once. `text/include/text/font.h` is the known one; there may be
  others in `render.md` and `text.md`.
- Whether keeping the console report alongside the screen one felt right or
  redundant.
- Whether handling `drawing == false` in the loop was awkward, and whether the
  view and sun had to be computed anywhere other than where they are today. Both
  are inputs to a later decision about what the `app` folder exposes.

## Notes (coder, 2026-09-08, Linux/WSL, claude-fable-5-1)

**Implemented in full, including the 2026-09-08 amendment, and verified:
`cmake -P check.cmake` exited zero on the human's Windows machine (clang 22,
cmake 4.3.3, slangc) — every folder standalone, root build, all guards, includes,
34 tests passed, analyser clean over 92 files. Written on Linux/WSL, where the
toolchain is absent (see the last paragraph), so Linux is the platform not
checked; whatever it turns up becomes a new card.**

- `render`: three `transient_` capacities (all-nought allowed, meaning no
  transient room; a create then refuses with a message). Host-visible mapped
  vertex and index pool per frame slot, built in `geometry.c` beside the static
  pools. Slot table is two bands; `voe_render_geometry_at` only had its bound
  widened. `voe_render_geometry_frame_reset` runs in `_begin` right after the
  fence wait: live transient slots go not-live and bump generation; the slot's
  pools go back to empty. `voe_render_geometry_create_transient` asserts on no
  open frame, refuses on a full pool or no slot with `REFUSED` and a stderr line
  naming the numbers, and copies straight into the mapped pool. The draw tracks
  `device->bound_transient` like `bound` and rebinds the vertex+index pair only
  when a range's pool differs. The transient band starts at generation 1 so
  generation 0 stays unissued while the reset, not the create, moves it on.
  One addition the card did not name: `voe_render_frame_is_open`, so the draw
  system can make the assert the amendment asks for without seeing render's
  internals.
- `text`: `voe_text_block_create_transient`; both creates are one `build_block`
  with a bool deciding the last call. Header paragraph rewritten; the console
  sentence removed.
- `3d`: `voe_3d_mesh_set_geometry`; mesh header reasoning rewritten to "one
  writer per row, nothing to coordinate"; layer not writable. Under ADR-0098:
  `voe_3d_draw_system_frame(world, size)` computes the camera and the sun once
  into a `voe_3d_frame`; the loop passes it to `_begin` and back to
  `voe_3d_draw_system_run(world, device, arena, frame)`, which no longer begins
  or ends anything, has no `size`, and debug-asserts on no open frame. A size
  with no area gets aspect 1 in `_frame`, since `_begin` is about to say
  `drawing == false`. **Deviation from the card's letter:** `_run` returns
  `void`, not `bool`. Nothing left in it can fail in a way the loop should stop
  for — a refused draw stops its group and says so, as before — and a `bool`
  that is always true would be surface nothing uses (rule 10). The loop breaks
  on a false from `_begin` or `_end`, which are the two calls that can fail.
  Header rewritten; `3d/tests/import.c` drives begin/run/end itself.
- `dev`: the loop owns the frame: `_frame`, `_begin`, build the readout,
  `_run`, `_end`; `drawing == false` skips the middle three. The readout is a
  fourth text entity, unlit blended distance-field material, overlay layer,
  snapped into the top-left corner of the view — the corner computed from the
  camera's field of view and the window's aspect ratio, so it stays there on a
  resize — and left-aligned, so its changing width does not move it. Smaller
  than the heads-up line (`READOUT_EM 0.040`). Five lines: fps and the four millisecond
  numbers as the current period's running averages. Console block kept. A
  failed readout build is reported and stops the program after the frame has
  been ended. Prints once what the readout consumed. Capacities chosen:
  `MAX_TRANSIENT_GLYPHS 128` (512 vertices, 768 indices), 2 ranges; the
  readout is about 50 glyphs (the program prints the exact count). The `draw`
  sample now spans `_begin` to `_end` and its two comments say so. The three
  camera-locked placements share one `basis_of()` instead of three copies.
- Test: `render/tests/transient.c` — stale id refused next frame and the same
  slot draws different pixels; static + transient in one frame with two rebinds;
  overrun refused, the range that fitted still draws, the next frame is fine;
  device with no transient room refuses.

**Report items that need the program run** (rebind count per frame, transient
cost against `update`/`draw`, screenshots, the memory-flat soak) are the
reviewer's to take on the Windows machine; the coder's machine cannot run it. Expected rebinds: two per frame — the readout is one
transient range inside the overlay's blended group, so the pair is rebound once
into it and once out unless it sorts last.

**Verification on the coder's machine was not possible.** WSL, `/mnt/dev` is a
9p mount of a Windows folder. `ninja`, `slangc` and `wayland-scanner` are not
installed, and `chmod` is refused on the mount, which makes CMake's
`configure_file` fail even before the missing tools. What was run there instead:
`clang -std=c23 -fsyntax-only -Wall -Wextra -Wpedantic -Werror` and
`clang --analyze` on every translation unit touched — `geometry.c`, `frame.c`,
`descriptors.c`, `tests/transient.c`, `mesh_component.c`, `draw_system.c`,
`tests/import.c`, `font.c`, `dev/src/main.c` — all clean. The full
`check.cmake` run above is the human's, on Windows.

Files touched were normalised to LF (the index is LF; the working tree on this
mount was CRLF), so `git diff --ignore-cr-at-eol` shows exactly the change.
