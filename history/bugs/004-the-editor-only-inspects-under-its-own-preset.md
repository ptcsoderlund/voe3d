# bug 004 — The editor expands nothing unless it was built with the `editor` preset

status: card 063 — decided 2026-09-12 by ADR-0145, *this repository is a development tree
  and field descriptions are on throughout it*. **Remedy 2, "on everywhere", taken with its
  objection removed**: the objection was that every shipped binary would carry the field
  names, and ADR-0144 settles that no shipped binary is built from this root at all. The
  report's own third statement is what did it, exactly as written below.
  **An interim fix went in first**, on the principal's explicit instruction while the
  decision was being written — *"turn on descriptions in debug preset so i can review 059.
  There are no numeric inputs showing. I review in debug."* That is remedy 1, it is in the
  working tree, it is described under *What was changed* at the foot, and **card 063
  supersedes it**: the switch moves out of the presets and into `voe_target_settings`, for
  the reason the same section names — `check.cmake` never reads a preset, so with the flag
  living in one the gate builds a configuration nobody runs. Archived under ADR-0110; the
  move reverses if card 063 is abandoned.
found-by: the principal, 2026-09-12, reviewing card 059 on the Linux dev machine
reported-by: claude-opus-5 (kanban coder), from the principal's words
folder: the build — `CMakePresets.json`. **Probably not a folder at all**: the
  switch being off by default is written down in `base/include/base/describe.h` as
  a decision, so the fix changes something settled and wants an ADR before a card.
severity: the editor's only feature today is invisible in two of the three builds,
  and the sole warning is one line of startup text a person has usually scrolled
  past. A reviewer's first run shows two headings and nothing under them, which
  reads as the inspector being broken. **Nothing is wrong with the picture, the
  data or the engine**: the inspector is correct in every build, it is being told
  there are no fields and saying so. `dev`, `render`, the tests and `check.cmake`
  are all unaffected, and so is every other program in the tree — the editor is the
  only one that reads a description.

## The report, in the reporter's words

> The properties dont show. If i select cube i can only se transform but not its
> properties.

and, on being told which binary that was:

> Editor should work the same way in debug mode as in release. Or else we cant
> debug.

and, on being shown the three remedies:

> 059 says kind and control for fields. We see no fields. editor is supposed to
> output C code and more for clang to build. Meaning the editor project should not
> be baked in the finished game.

## What happens

Three presets, three behaviours, and only one of them inspects anything.

| built with | `CMAKE_BUILD_TYPE` | `-DVOE_BASE_DESCRIPTIONS=1` | selecting `Cube` shows |
|---|---|---|---|
| `--preset debug` | Debug | no | two headings, nothing under them |
| `--preset release` | Release | no | two headings, nothing under them |
| `--preset editor` | Debug | yes | the transform's nine number boxes and the identity's two labels |

## What should happen

The reporter's sentence is the requirement: one program, one behaviour, whichever
preset built it. Today the editor is a different program depending on how it was
configured, and the difference is the whole of what it is for.

## Evidence the code actually ran, and is not a stale build

Not a rebuild anyone forgot. The two binaries differ by **exactly one compiler
flag** and nothing else — from each build's own `compile_commands.json`, the same
source file:

    debug  : -g
    editor : -DVOE_BASE_DESCRIPTIONS=1 -g

Both say which they are on their first line of output, which is
`say_whether_descriptions_are_in` in `editor/src/main.c`:

    build/debug/editor/voe_editor   descriptions  off — nothing will be expandable; …
    build/editor/editor/voe_editor  descriptions  compiled in

And the inspector's own count, measured while card 059 was being verified: with the
same entity selected it records **nine controls** under the `editor` preset — three
position, three rotation, three scale — and **none** under `debug`. Both runs drew
both component headings, so the walk over the world's types found the same two
components either way.

## Where it isolates to

`voe_ecs_component_description` hands back `NULL`, and the inspector draws the
heading and nothing else — which is exactly what card 059 specifies for that case.
The `NULL` is put there at registration, four lines in `scene`:

    // scene/src/transform_system.c:77
    static const voe_base_struct_description *transform_description(void)
    {
    #if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
    	return voe_scene_transform_description();
    #else
    	return NULL;
    #endif
    }

`scene/src/identity_system.c` has the same shape. So nothing in `editor` or in `ecs`
is involved: by the time the inspector asks, the answer was decided when `scene` was
compiled.

## The suspect, as a hypothesis

**Hypothesis: there is nothing to fix in any folder, and what is wrong is that the
switch is a build-wide `-D` at all.** `base/include/base/describe.h` says it plainly
— *"VOE_BASE_DESCRIPTIONS IS THE SWITCH, AND IT IS OFF UNLESS A BUILD ASKS"* — and
gives the price of having it on: every translation unit carries its own copy of each
table, and the field names reach the binary as strings. That is a real cost for a
shipped program and a non-cost for a tool. Which is presumably why the `editor`
preset exists.

**The switch has no idea the inspector exists, and that is the whole shape of this.**
The field list is written down in the component's own folder because that is where
the fields are, so the table is compiled into `voe_scene` — which ships inside a
game. The flag is therefore a property of the build and not of a program, and the
editor inherits it by linking the same archive everything else links. Nobody
decided to turn the inspector's fields off; the inspector was not in the room.

If that holds, the question is not *why is it off in debug* but *why is a program's
own required feature a flag somebody has to remember*, and the answers are a
decision and not a patch:

1. **On for `debug`, off for `release`.** Smallest change. Does not give one program
   one behaviour — `release` still shows nothing.
2. **On everywhere.** One behaviour always, and every shipped binary carries the
   field names as strings and a copy of each table per translation unit, for good.
3. ~~**Make the description an argument to registration**~~ —
   `voe_scene_transform_register(world, capacity, description)`, so the caller and
   not the build decides. **Struck by the principal's third statement above**: it
   existed only so that a shipped game would not carry the editor's tables, and a
   game that does not build the editor does not carry them. It solves a problem that
   does not exist.

**AND THE THIRD STATEMENT IS THE ONE THAT SETTLES THIS.** If the editor emits C code
for clang to build, it is a tool that is never in the finished game — so a game's
build tree and the editor's build tree are different trees, and descriptions being
on in the tree that holds the editor costs a shipped binary nothing. **The cost this
switch exists to avoid was never the editor's to pay.** What is left is one: the
fields are on wherever the editor is built, which makes the `editor` preset
redundant rather than necessary — it buys back a feature that should not have been
off. Which of `debug`, `release` and a future game build that is, is the decision.

It is still not mine to pick.

**A fourth was considered and is dead: a compile definition on the `voe_editor`
target.** It cannot work and the reason is worth writing down so nobody proposes it
twice. `target_compile_definitions(voe_editor ...)` reaches `editor/src/*.c` and
nothing else, but what the world is told is decided at
`scene/src/transform_system.c:212`, inside `voe_scene`, which is compiled once for
the whole build. The define would arrive long after the `NULL` was chosen. I put
this option to the principal before checking it and it was wrong.

## What confirms or kills it

One line, no source edit:

    cmake -B /tmp/probe -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_C_FLAGS=-DVOE_BASE_DESCRIPTIONS=1 && \
    cmake --build /tmp/probe --target voe_editor

If that binary inspects, the flag is the whole of it and no code is at fault. If it
does not, this report is wrong and the fault is somewhere in `scene`'s registration.

**Run, 2026-09-12: it inspects.** A plain `Debug` build with nothing but that flag
added builds `voe_editor` and it says `descriptions  compiled in`; the flag reached
`scene/src/transform_system.c` and `scene/src/identity_system.c` in the same build,
which is the link that matters, since those two are where the `NULL` is decided. So
**no code is at fault in any folder** and the whole of this is which builds pass one
`-D`. The probe build directory was removed; nothing in the tree was edited.

## Things that look like the fault and are not

- **Debugging is already possible today, and the reported reason is met.** The
  `editor` preset is `CMAKE_BUILD_TYPE: Debug` with `-g`, differing from the `debug`
  preset by that one `-D` and by nothing else — same optimisation level, same
  asserts, same symbols. A debugger attached to `build/editor/editor/voe_editor` is
  a debugger on a debug build. **This does not make the complaint wrong**: a program
  that behaves differently under a preset named `debug` than under one named
  `editor` is a trap whatever the symbols say, and the trap is the report.
- **`release` is not the working one either.** Taken literally, "the same in debug as
  in release" is already true — both are blank — so whoever writes the fix should
  read the requirement as *the editor inspects in every build*, which is what the
  second sentence means.
- **The inspector is not at fault and card 059 is not the cause.** The behaviour
  predates it: card 058b put the preset and the startup line in, and
  `editor/editor.md` has documented it since. 059 is what made it visible, by giving
  the panel something to fail to show.
- **The tests do not stand in the way.** `base/tests/describe.c` turns the switch on
  for itself whatever the build said, and `scene/tests/transform.c` and
  `identity.c` follow the build through a `BUILD_DESCRIBES` constant and check both
  answers. Turning it on globally breaks none of them.

## Two things the repository does not say, and one of them is load-bearing

- **That the editor emits C code for clang to build.** It is in none of
  `editor/editor.md`, no card in `complete/`, and no file header in `editor/`. The
  root says the repository is the only memory; this is a fact about what the editor
  *is*, it decides the whole of the paragraph above, and today it exists only in the
  principal's head and in this report. **Writing it down is worth more than the fix.**
- **The build does not express the separation.** `CMakeLists.txt:17` is a bare
  `add_subdirectory(editor)`, so `cmake --preset release` builds `voe_editor`
  alongside the engine exactly as `debug` does. "The editor is not baked into the
  finished game" is an intent today and not something configuration enforces — which
  is also why the first two remedies have to name which builds, rather than being
  able to say "the game's build" and have that mean something.

## Why it was not caught before

Card 058b introduced the preset, and card 059's Verify asks for both builds to be
run and describes the headings-only result as the expected `debug` behaviour. So it
was seen, written down, and read as correct — by me, on this card, in the report I
handed in. What nobody wrote down is that a reviewer opening the program for the
first time reaches for `debug`, and that one line of startup text is all that stands
between him and concluding the feature does not work. **That is what happened here.**

## Platforms

- **Linux** (Fedora, Wayland, NVIDIA RTX 4070 Laptop, clang 22) — reproduces every
  time, on both the `debug` and the `release` preset.
- **Windows** — not tried. It is a compile-time flag with nothing platform-specific
  anywhere near it, so it will reproduce there identically; that is a prediction and
  not a measurement.

## Cards this does not impeach

**059** (in `review/`), **058a**, **058b** and **060**. 059 does what it says under
the preset its own Verify names, and the two screenshots taken for it show both
behaviours. Nothing here is a defect in any of the four — the fault, if it is one,
is that the preset split exists at all, which is older than all of them.

## Not fixed, deliberately

No edit was made. The switch's default is written down as a decision in
`base/include/base/describe.h`, the remedy changes `CMakePresets.json` and possibly
`cmake/voe.cmake`, and choosing between the three remedies above is an architecture
call. A coder stops here (ADR-0108).

## What was changed, on instruction

The principal reviewed in `debug`, saw no fields, and directed the change rather
than waiting for a card. Made, and said here so the inbox is not carrying a report
that describes a tree that has moved on:

- **`CMakePresets.json`** — the `debug` preset gains
  `"CMAKE_C_FLAGS": "-DVOE_BASE_DESCRIPTIONS=1"`. Two lines. `release` is untouched
  and still has them off. `debug` and `editor` are now byte-identical
  configurations, confirmed from both trees' `compile_commands.json` against
  `scene/src/transform_system.c`: `-DVOE_BASE_DESCRIPTIONS=1 -g` either way.
- **`editor/editor.md`** — the paragraph that said `debug` keeps descriptions off
  and cannot expand anything was false the moment the preset changed, so it was
  corrected in the same edit.

`cmake -P check.cmake` exits zero afterwards. **Nothing it gates changed**: it
configures its own build directories and never reads a preset, so its runs still
build with descriptions off — which is what keeps the `BUILD_DESCRIBES false` branch
of `scene/tests/transform.c` and `identity.c` covered. That branch would otherwise
have been silently lost, which is the one thing this edit could have broken.

**What is now redundant and was deliberately not touched:** the `editor` preset,
which is a duplicate of `debug`. Removing it is a decision — `editor/editor.md` and
card 058b both name it — and a coder does not take one.

**This is not card 059's change.** 059 is in `review/` and its diff is
`editor/src/inspector.{h,c}`, `dock.c`, `scene.h`, `interface.c` and the two file
lines in `editor.md`. The preset line and the preset paragraph in `editor.md` are
this report's, made afterwards and on instruction. Review them as two things.