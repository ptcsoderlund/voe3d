# 0149. A scene file is entity sections by number, component sections beneath, and one spelling per value

- **Status:** Accepted
- **Date:** 2026-09-13
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (completes ADR-0073, whose grammar it deliberately left unwritten)
- **Superseded by:** in part, by ADR-0154 — the value table's *Fixed arrays* row: an array item is any value, to 8 levels

## Context

**Closes D-097**: *the authored text format's specification*, the grammar ADR-0073 chose the
direction for and left unwritten with six findings as the agenda. It is the second decision
of the arc the principal chose on 2026-09-13 (ADR-0146): nothing can be saved without it.

**Most of it was already decided by the time it was asked.** Card 038 built the sectioned
parser for the theme (ADR-0096) and pinned, with tests, the lexical half: `[Section]` headers,
`key = value` lines, whole-line `//` comments, blanks tolerated around `=`, at most two dotted
parts in a section name, a repeated key or section refused, `\r\n` tolerated, values returned
as text (`assets/include/assets/sectioned.h`). And the component decisions answered three of
ADR-0073's six findings by removing their premise:

- **Type sigils (finding 1).** ADR-0122 and ADR-0127: a component's fields are described in code
  with their kinds, so the reader knows the type before reading the value.
- **Arrays of complex things (finding 4).** ADR-0123 and ADR-0124: a component is flat; one-to-many
  is a fixed array of references, an inversion, or an entity.
- **Whitespace and comments (finding 6).** Card 038.

What was left: how an entity and a component are told apart in a file (finding 5), one spelling
per value (finding 2), string escaping (finding 3), and what a writer emits so a save diffs
cleanly.

Constraints already fixed. ADR-0073: an edit touches one line and a bad edit stays local.
ADR-0125: identity is one component with a per-file numeric id and a name; ids are unique in
their file; references are ids; presence means authored. ADR-0123: the field vocabulary.
ADR-0130: the principal's Windows machine reads the same files.

## Options considered

### Option A — an entity's section is its number
`[1]` holds the entity's name as an ordinary key; `[1.voe_scene_transform]` is a component of
it. The header is the one thing about an entity that never changes. A rename is one line. A
reference `target = 1` is found by searching for `[1`. Names are free text.

### Option B — an entity's section is its name
`[Main_Camera]` with `id = 1` inside; `[Main_Camera.voe_scene_transform]`. Reads better. A
rename rewrites every header the entity has, which in git conflicts with anyone editing any of
its components; a reference means hunting for `id = 1`; names may hold no blank and no dot
and must be unique in the file.

## Decision

**Option A**, the tech lead's recommendation and the principal's call — together with the
principal's own refinement that **a scene holds entities and components and nothing else**:

> *"What if we decide that everything is an entity or a component? There is no difference for
> scene."* — and, to the four points as restated, *"yes"*.

**Sections.**

1. **A scene file has exactly two shapes of section.** `[N]` is an entity, `N` its authored id,
   decimal, at least 1. `[N.<key>]` is one of its components, `<key>` the component's
   `voe_ecs_key` name exactly — `voe_scene_transform`. **Any other section name in a scene file
   refuses the file.** There is no `[scene]` section and no header block: scene-wide settings —
   ambient light, sky, gravity — are components on an entity, which is Godot's
   WorldEnvironment shape.
2. **`[N]` is where the identity component is written.** Its id is the header and its other
   fields are keys beneath it — today `name = "Main Camera"`. `[N.voe_scene_identity]` never
   appears.
3. **A component section needs its entity section somewhere in the file**, not necessarily
   adjacent; one without refuses the file.
4. **A new entity takes the highest id in the file plus one.** Nothing records a next id. The
   editor clears in-file references to an entity it deletes; a reference from another file is
   D-237's.

**Values** — the field's described kind says how its text is read; there is one spelling each.

| Kind | Written | Refused |
|---|---|---|
| Integers | decimal, optional leading `-`: `-12` | `_` separators, hex, `+`, a value out of the type's range |
| Floats | decimal, with or without fraction or exponent; the writer emits the shortest text that reads back to the same bits | `inf`, `nan`, hex floats |
| Booleans | `true`, `false` | `1`, `0`, any other spelling |
| FLOAT2/3/4, QUAT, FLOAT4X4 | `[x, y, z]`; a quaternion is `[x, y, z, w]` and a matrix sixteen numbers, both in memory order | a count other than the kind's |
| Fixed arrays | `[a, b, c]`; an array of vectors nests once: `[[0, 0, 0], [1, 1, 1]]` | a count other than the field's — never zero-filled |
| Text (`CHAR` arrays) | `"..."`, UTF-8 bytes as they are; escapes `\"` and `\\` and nothing else | any other backslash, a newline, more bytes than the field holds less its terminator — never truncated |
| Entity references | the target's authored id; `0` is no entity | — |
| Enumerations | the value's name, bare | **writable only once D-246 gives a described enumeration its names** |

**Paths in any authored file use `/`**, which both platforms accept; that is what lets the
escape rule stay two characters.

**What a writer emits** — so a save of an unchanged world is byte-identical and a change diffs
as the lines that changed:

5. Entities in ascending id; each entity's components in ascending byte order of key; fields in
   declaration order; **every described field written**, default or not.
6. `key = value`, one space either side; `, ` between array items; a blank line before each
   `[N]` and none elsewhere; `\n` line endings on every platform; a final newline.

**When the file and the code disagree** — a struct changed under a file, or the file names
something this program was not built with:

7. **A described field missing from its section** loads as zero and is reported as a warning.
8. **A key naming no field** is reported as a warning and is gone at the next save.
9. **A component section whose key this program never registered** is kept as its text and
   written back unchanged, in its sorted place. An editor built without a game's components
   opens that game's scene and does not destroy what it cannot read (D-254).
10. **Everything else wrong with a value refuses the whole file**, with the line reported — a
    value that does not read as its kind, a wrong count, a bad escape. A scene is never half
    loaded.

**Where this lands in code.** Card 038's parser changes in one respect: the two escapes, and a
backslash followed by anything else becomes a malformed line. Points 1–10 are the scene reader
and writer — ADR-0096's layer three — which reads the parser's text through the world's
descriptions and does not exist yet. Where that code lives is D-260.

## Blast radius

**Load-bearing for points 1, 2 and the value table**: every scene anyone saves is written this
way, and changing a spelling later is a converter over other people's files. **Cheap for points
5 and 6**, a writer's formatting, which a later version may change at the cost of one noisy diff
per file. Points 7–9 are policy a later decision can tighten.
Reversibility: **load-bearing.**

## Consequences

- **A section header is a number, so a scene is less pleasant to skim.** The name is the next
  line. That is the price of renames and references being local, and it was paid knowingly.
- **An agent editing a scene** appends an entity by appending `[N]` and its sections at the
  highest id plus one, and edits a field by changing one line. Both stay local, which is
  ADR-0073's whole claim.
- **`source_assets/Custom_Text_Base_Syntax.txt` is superseded as a reference.** The theme
  file already follows its section shape and is unaffected, save that a backslash in a theme
  value now needs escaping.
- **What is saved is decided elsewhere**: an entity by its identity (ADR-0125), a component by
  its description (ADR-0150).
- **Every field written means a struct gaining a field adds a line to every entity** that has
  that component at the next save. Accepted: it is one honest diff, and a writer that omitted
  defaults would make the file disagree silently with a changed default.
- **The theme is not a scene** and keeps its own schema; the value spellings in the table apply
  to every authored file, its section shapes only to scenes.

## Rejected options and why

**B — an entity's section is its name.** Every rename rewrites every header of the entity and
conflicts in git with any concurrent edit to its components — the global edit ADR-0073 chose
this format to avoid — and it constrains names to what a section name may hold.

**A `[scene]` section holding a next id.** The tech lead's first draft, withdrawn at the
principal's question: it is a third kind of section in a file that otherwise needs two, and
highest-plus-one costs nothing a single file can observe.

**Zero-filling a short array, truncating a long string, `last wins` on a duplicate.** Each turns
a bad edit into a silent one.

## Questions this opens

- **D-259** — how a component names an asset — a model, a texture — so that a mesh survives a
  save. `3d`'s mesh component holds a GPU geometry id, which is runtime state (ADR-0150), so a
  saved scene reloads its cubes without a shape until a component carries a path and something
  resolves it. Blocked by D-226, the file call.
- **D-260** — where the scene reader and writer live. It needs the world, the descriptions and
  the parser's text: `scene` depends on `ecs`, `math` and `base` and not on `assets`, and `3d`
  depends on both but is a renderer. Hot, and the arc's next placement question.
