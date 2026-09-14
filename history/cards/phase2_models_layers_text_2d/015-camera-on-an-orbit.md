# 015 — a camera, on a hardcoded orbit

status: complete
claimed-by: claude-code (kanban-coder)
blocked-by: 014

## Goal

The camera orbits the scene on its own, with no input. Proves the whole matrix
chain end to end.

## The scene, and it is the point

**Two cubes. One rotating on its own axis, one completely still. The camera orbits
both.**

The principal's design, and it is the only arrangement that is unambiguous: with
one cube you cannot tell an orbiting camera from a rotating cube. Three independent
motions means each one is identifiable, and a mistake in any of view, model or
projection shows up as a specific wrong thing rather than "something looks off".

**This is also where the front-face and Y-flip check gets its human counterpart.**
Card 013 proves it with a pixel readback; a cube you can see all sides of is what a
person can judge.

## Scope

- A view matrix from a position and a target. A projection matrix — and note that
  `math` deliberately builds no projection matrices (ADR-0035), so this belongs to
  whichever folder owns the camera.
- **Per-object model matrices**, since there are now two objects. Push constants or
  a per-draw uniform; say which and why.
- **The rotation type** the principal asked for in the original maths card arrives
  here, in `math`, with tests. This is the card where something finally rotates, so
  implement-on-demand is satisfied.

## Deliberately not the world-as-tables folders

`ecs` and `scene` are **not** written by this card. Two objects is not many
objects, and the tech lead was wrong to say earlier that a camera brings the ECS
with it. Tables arrive when there is something to tabulate — model loading or
later. If this card feels like it needs them, that is a finding to report.

## Verify

- All three motions distinguishable by eye, and the still cube genuinely still.
- Nothing clipped at the near plane when the camera passes close.
- `check.cmake` zero. Windows is the principal's.

---

## Decided during the card

Nothing here was asked, because nothing here changed what the card asks for.
Each of these is reversible and each is written down at the site as well.

- **The camera stays in `render`.** The card says it belongs to whichever folder
  owns the camera, and it rules `ecs` and `scene` out; `3d` does not exist. So
  `render/src/cube.c` keeps the look-at and the projection it already had, and
  grew the orbit and the two model matrices. No new folder, no new edge, and the
  file's header says what replaces it.
- **The rotation type is `voe_math_quat`, and it has one function.**
  `_from_axis_angle`, plus `voe_math_float4x4_from_quat` beside `from_translation`
  and `from_scale`. No `_identity`, `_mul`, `_normalize` or `_slerp`: nothing
  composes or interpolates a rotation yet and rule 10 says a function is written
  when something calls it. `quat` and not a Slang spelling because **Slang has no
  quaternion**, so there is nothing to keep in step; the components are `x, y, z,
  w` with the scalar last, which is the order glTF stores a rotation in.
- **Push constants for the model matrix, a uniform buffer for the camera** — the
  card asks which and why. The split is by how often each changes: the camera is
  written once per frame and read by every draw in it; the model matrix differs
  between two draws in one command buffer, which one uniform buffer cannot say
  without a second buffer, a dynamic offset or a descriptor per object. A push
  constant is none of those — 64 bytes into the command buffer, no allocation, no
  descriptor, nothing per-object to tear down. Vulkan's floor for
  `maxPushConstantsSize` is 128, so one matrix fits everywhere and nothing is
  queried. It is also the option that runs out first, and running out is the
  question the card that brings many objects has to answer.
- **`struct voe_render_uniforms` lost its `model` member**, which is what made
  room for the above. `matrix_probe.slang` read that member; it now reads `view`,
  the first member, and `render/tests/matrix.c` writes there. What that test
  claims is unchanged — it is about the layout of sixteen floats, and any member
  would do.
- **The still cube is at the origin and the turning one stands aside**, and that
  order is deliberate: the camera looks at the origin, so
  `render/tests/offscreen.c`'s centre pixel still lands on the same cube's `+Z`
  face and its claim needed no weakening. Straddling the origin with two cubes
  would have put background in the centre of that test.
- **The spin axis is tilted, not `+Y`.** The camera orbits about `+Y`, so a cube
  spinning about `+Y` too would read as the same motion at another speed — which
  is the one ambiguity the card's arrangement exists to remove.

## The one placeholder, and it is named as one

**The clock counts frames; it does not measure them.** `render` has no time
source — `platform` will own one and **card 020** is the card that brings it — so
`voe_render_device_frame` adds `1.0f / 60.0f` seconds per recorded frame. Every
number downstream of that line is already in seconds, so card 020 replaces one
`#define` and nothing else. What it costs meanwhile: the orbit's speed follows
the display's refresh rate. `NOMINAL_FRAME_SECONDS` in `render/src/frame.c` says
all of this at the site.

A side effect worth having: a device nobody has asked for a frame is at zero
seconds, so `voe_render_frame_draw` is deterministic for the two tests that call
it directly.

## What changed

    math/include/math/quat.h        new    the rotation type
    math/src/quat.c                 new    one way to build one
    math/tests/quat.c               new    which way it turns, and by how much
    math/include/math/float4x4.h    +      _from_quat
    math/src/float4x4.c             +      the nine elements, and a unit assert
    math/math.md                    +      three lines

    render/src/cube.c               the orbit, the two model matrices, the header
    render/src/frame.c              the counted clock, a push and a draw per cube
    render/src/device.c             the push constant range on the pipeline layout
    render/src/device_internal.h    uniforms without model, voe_render_push, seconds
    render/src/loader.h/.c          vkCmdPushConstants
    render/shaders/cube.slang       a push constant block; model comes from it
    render/shaders/matrix_probe.slang   reads view rather than model
    render/tests/matrix.c           a third claim: the scene, on the CPU
    render/include/render/device.h   what it draws, and that it moves on its own
    render/render.md                four lines

    dev/src/main.c                  the header: three motions, and how each fails
    dev/dev.md                      one line

## Verified

**`cmake -P check.cmake` exits zero.** All fifteen steps `ok`, nothing skipped:
tools, five standalone folders, root configure and build, the three guards,
includes, **tests (9 passed)**, the harness's can't-fail proof, **analyser (30
files)** and its can't-fail proof. Linux only; Windows is the principal's.

Run on a mirror of the tree in `/tmp`, and that is not a shortcut — it is the
only place it can run. See *For the reviewer* below.

**The two render tests that draw really ran**, on Mesa's `llvmpipe` (Vulkan
1.4.335) — they are not skipping. `render/offscreen` still passes unchanged,
which is the culling and Y-flip claim surviving the geometry doubling.

**The new checks were made to fail on purpose**, because a test that has never
failed has not been tested:

| Mutation, in the mirror only | Result |
|---|---|
| `mul(rotation, translation)` instead of `mul(translation, rotation)` | `render/matrix` fails: `turning.m[0..2][3] == centre` — the spin became an orbit |
| `look_at` forward reversed to `eye - target` | `render/matrix` fails, 98 checks |
| Orbit shrunk to radius 1.7 at height 0.2 | `render/matrix` fails: `closest > near_plane * 2.0f` |
| Nothing (control) | 9 passed |

`math/quat`'s own checks are the same shape: a reversed sign or a forgotten
half-angle fails the three cyclic axis cases, and a reflection fails the
determinant.

**Looked at, not only asserted on.** There is no compositor here to judge it in a
window, so the scene was drawn headless at seven times through the engine's own
`voe_render_frame_draw`, read back and converted to images. All three motions are
there and each is identifiable:

- At 0 s the still cube is dead centre, the other 1.6 m to its right; top faces
  visible, both solid, near faces hiding far ones.
- At 1 s the right cube is visibly tilted about its own centre and has not moved;
  the left one is unchanged in position and size.
- At 3 s — a quarter of the orbit — the camera is on `+X`, the turning cube is
  **in front of** the still one and occludes it. That is the depth test and the
  parallax in one picture.
- At 6 s — half the orbit — the turning cube is on the **left**. The still cube is
  still in the middle, showing its `-Z` face.

**`voe_dev` runs.** It opens a real window under WSLg and presents frames for six
seconds without a failure or a crash, which exercises the push constants on the
presented path and not only the offscreen one. It could not be looked at from
here.

Markers left in the code: **none**. No `DEVIATION:`, no `BLOCKED:`.

## For the reviewer

- **`check.cmake` cannot run in the working checkout on this machine, and that is
  the checkout's location and not the script.** The repository is on a 9p Windows
  mount (`/mnt/dev/...`), where CMake's `configure_file` fails with `Operation not
  permitted` — step 2 dies inside `CMakeDetermineSystem.cmake` before any of this
  engine's code is reached. Proved by configuring an empty two-line project in the
  same directory: same failure. The same tree on tmpfs configures and builds
  clean. So every run above is a byte-for-byte copy of the tree in `/tmp`,
  refreshed before each run. Worth knowing before the next agent concludes the
  script is broken.
- **The machine had no `ninja`, no `slangc`, no `wayland-scanner` and no
  `pkg-config`**, so `check.cmake` had nothing to run. They were fetched into the
  session's scratch directory — Ninja 1.12.1, Slang 2026.16.1, and Ubuntu's
  `libwayland-bin`/`libwayland-dev`/`pkgconf` unpacked with `dpkg -x` — and put
  on `PATH` for the runs. **Nothing was installed on the machine, nothing needed
  root, and nothing outside the scratch directory changed.** The tools are the
  programmer's to install per `CLAUDE.md`; this is one session's stand-in for
  that, not a change to the onboarding story.
- **`git config --global --add safe.directory` was needed** before any git command
  worked in the submodule ("dubious ownership"). One line in the user's global git
  config; nothing else was touched and nothing was committed.
- **The whole tree shows as modified in `git status` and it is not this card.**
  Seventy-seven files, 37,417 insertions and exactly 37,417 deletions: the
  worktree is CRLF and the index is LF, with no `.gitattributes` and
  `core.autocrlf` unset. It predates this card — vendored Vulkan headers are in
  it — and it is a git-remote matter, so it was left alone. New files here are LF,
  matching the index and the tree's most recent files; edits to CRLF files keep
  CRLF, so no file was left with mixed endings.
- **Card 014 left `dev/src/main.c`'s header and `dev/dev.md` describing a
  triangle** after the cube replaced it. This card rewrites those same lines, so
  they were brought up to date rather than left half wrong beside new text.
  Flagging it because it is a correction to finished work, not this card's own.

## Suggestions, not done here

- `voe_render_cube_model` is an `if (index == 0)` over two cases. That is honest
  for two objects and it is the shape that has to go when there are many — a
  table, and then something that owns the table. Not this card's call.
- The projection's aspect ratio is recomputed per frame from the target's extent,
  which is right, but the camera's field of view, near plane, orbit and periods
  are six `#define`s in `render`. The first thing that wants to change one of them
  from outside is the card that decides where a camera lives for real.
- `render/tests/matrix.c` is now three claims in one file and its name only covers
  two of them. Splitting the scene half into its own test file is a rename
  question, and renames of test files are cheap — say the word.
