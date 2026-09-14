# 0073. Authored data is a sectioned line format of our own; JSON is for what we import

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

ADR-0010 requires authoring to be text **so that AI agents can take part in
developing games**, with a build step to binary for shipping. It left the
authoring *syntax* open, saying only that the text scene format "arrives with the
editor". Three later decisions quietly filled part of the space:

- **glTF is the import format, in its text form** (ADR-0010) — and glTF is JSON.
- **The shader graph is JSON**, committed as source of truth (ADR-0053).
- **The JSON parser is written in-house and lives in `assets`**, over a byte
  range, sized at about a day (ADR-0054, ADR-0023).

So a JSON parser arrives regardless of what is decided here. The question is not
*JSON or something else* — it is whether **authored scene and component data
shares that syntax or gets a second one**.

The principal raised it and sketched a candidate syntax
(`source_assets/Custom_Text_Base_Syntax.txt`): `[Section]` headers with dotted
names for nesting, flat `key=value` lines beneath, `//` comments, arrays of
simple values, digit separators in numbers.

Nothing on the board consumes this. Cards 022 and 023 are sprites and GUI, and
neither touches a scene file. **The direction is decided today and the format
specification is explicitly not**, because its input — the component type
description (D-024) — does not exist yet, and the execution-order rule at the top
of the register says a decision taken before its input exists is taken on taste.

## Options considered

### Option A — JSON for everything
One grammar, one parser, nothing new to specify or own.

Costs: no comments. A trailing comma is a syntax error, so appending an entity
dirties the previous line too. Entity → component → field is three levels of
braces. Most importantly, **an agent adding an entity must balance braces and
place a comma at a distance**, and a wrong edit is not local — it invalidates the
whole document rather than one line.

### Option B — a sectioned line format for authored data, JSON only where an external standard forces it
Section header per object, flat `key = value` lines, dotted names for one level
of nesting. Every fact is one self-contained line; an append touches no other
byte; a bad edit stays local. Comments are free.

Costs: two syntaxes in the codebase, and we own the second one's specification
forever — including escaping, which is where homegrown formats usually break.

### Option C — TOML
Option B, but already specified, with a conformance suite behind it.

Costs: either a third-party dependency, which ADR-0023 makes the principal's
decision and defaults to no; or writing a TOML parser, which is *harder* than
the JSON one already committed to — dotted keys, array-of-tables, multiline
strings, date and time types.

## Decision

**Option B.** The deciding factor is the requirement that started all of this in
ADR-0010: **an agent editing the file**. In a line format, adding a component is
appending one line and a botched edit is visibly local. In JSON it is brace
balancing at a distance, and a botched edit corrupts the document. Line-oriented
files also merge, which is not theoretical — the principal works from several
machines.

The split is by **who writes the file**:

| File | Syntax | Why |
|---|---|---|
| Imported model (glTF) | JSON | An external standard; we do not choose it |
| Shader graph | JSON | Machine-authored by the node editor; nobody hand-edits a node soup |
| Scenes, prefabs, components, material instances, project and editor files | **The sectioned line format** | Written and edited by hand, by a human or by an agent |

**The recognition that this format is TOML.** The principal's sketch is TOML in
all but four details: `//` for `#`, `bool=1` alongside `true`, a `d` suffix for a
second float type, and no array-of-tables. Arriving independently at TOML's shape
is evidence the shape is right, and it reframes the eventual specification as
*TOML minus what we do not need*, not as invention from nothing. It also caps the
cost: **the parser is cheaper than the JSON one**, because a two-level sectioned
format has no recursion in it.

**Not specified today**, and each carries a finding from the review of the
sketch, recorded so the specification session does not start cold:

1. **Type sigils should go.** The sketch carries the type in the syntax — `200.0`
   is a double, `400.12345d` a decimal. There is no decimal type in this engine,
   and there is currently no way to write a 32-bit float, which is what nearly
   all engine data is. The component's struct already declares the type, so the
   parser knows it before reading the value; a sigil only creates a way for the
   file and the struct to disagree. **This is why the format cannot be specified
   before D-024** — answer the type description and the sigils disappear.
2. **One spelling per value.** `bool=1` and `true` both being legal is a tax
   every reader, test and agent pays forever, and `1` is ambiguous against an
   integer.
3. **String escaping needs a rule** — an embedded quote, a newline, a Windows
   path. The most common way a homegrown format breaks, and the principal is the
   Windows tester.
4. **Arrays of complex things are missing and will be needed.** A list of
   children, a list of materials. The sketch has arrays of simple values and one
   named sub-object per name. TOML's second bracket form exists for this.
5. **The sketch uses a top-level section two ways** — once as an entity carrying
   `type="Node"`, once as a component (`type="Transform"`). Whether a section is
   an entity or a component is structural, not cosmetic.
6. **Whitespace and comment placement** are unstated: spaces around `=`,
   indentation, end-of-line comments and what stops them inside a string.

## Blast radius

**Moderate, and asymmetric.** The *direction* is load-bearing in one narrow way:
it fixes the target syntax that the component type description (D-024) is
designed against, and that design is hard to redo. The *format details* are
cheap — nothing is written, and a sketch is not a parser.

Reversing to Option A after the editor exists means a converter and a re-export
of every authored file, which is the same cost ADR-0010 already accepted for
changing the authoring format.

## Consequences

- **Two parsers in `assets`, not one.** JSON for what we import, the sectioned
  format for what we author. Accepted because the second is smaller than the
  first and neither is optional.
- **The sectioned parser lives where the JSON parser lives** — `assets`, over a
  byte range, no file I/O of its own, for the reasons ADR-0054 gives. It inherits
  that shape rather than deciding it again.
- **Agents get one authoring syntax to learn, and it is the forgiving one.** The
  JSON they meet is glTF, which they read rather than write.
- **The format specification is now a named piece of deferred work**, triggered by
  the first card that reads or writes an authored file, and blocked behind D-024.
- **`source_assets/Custom_Text_Base_Syntax.txt` is a sketch, not a specification.**
  It informed this ADR; it does not bind the eventual format, and nothing should
  be written against it.
- **Entity identity and cross-references are untouched by this.** A scene needs
  stable ids so one entity can point at another and survive a rename and two
  machines editing at once. That problem is identical in every candidate syntax,
  which is exactly why choosing the syntax did not solve it.

## Rejected options and why

- **Option A — JSON everywhere.** The genuinely tempting one: it costs nothing
  new, and the parser is already committed to. Rejected on the founding reason
  for text authoring. JSON's failure mode under agent editing is global — a
  misplaced brace invalidates the file — where the line format's is local.
  Comments and clean diffs are secondary and would not have carried it alone.
- **Option C — TOML.** The better-specified format and the likely reference for
  our own. Rejected because taking it as-is means a dependency ADR-0023 defaults
  to refusing, and writing it means implementing dates, multiline strings and
  dotted keys that this engine has no use for. We take its shape and leave its
  surface area.

## Questions this opens

- **D-097** — the format specification itself: the six findings above, settled as
  one grammar. Blocked behind D-024, triggered by the first card that reads or
  writes an authored file.
- **D-098** — entity identity and cross-file references in authored data: stable
  ids, renames, and what happens when two files define the same name.
