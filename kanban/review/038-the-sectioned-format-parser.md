# 038 — the sectioned format parser

status: review
claimed-by: claude-fable-5-1
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it follows
030 and takes the next number. It is **not part of the GUI stack** — it sits in
`assets`, depends on nothing above it, and can be worked at any time by anyone.

The decisions behind this card are **ADR-0073** and **ADR-0096**.

## Goal

`assets` can read the engine's own authored text format: `[Section]` headers and
flat `key = value` lines.

**It returns text. It interprets nothing.** That is the whole shape of this card
and it is why it is small.

## The one thing to understand before starting

**You are building layer one of three, and the card exists because somebody
realised the other two were not blocking it.**

1. **The parser** — sections, keys, values as text. **This card.** It does not know
   what a value means and must not try.
2. **A consumer with a known schema** — a theme reader knows `contrast_strength` is
   a number because a person wrote the code that asks for it. Converts the text it
   gets. Not this card.
3. **A generic consumer** — reading arbitrary data back without knowing it in
   advance. **Blocked, and staying blocked**, on a separate open question about how
   a value carries its own type.

The example file carries type sigils — `400.12345d`, `400_000_000_000`,
`bool=1` — and **those exist for layer three.** You are not implementing them. A
value's text is handed back as it was written, minus quoting, and somebody else
decides what it is.

**If you find yourself writing a number parser, stop.** That is layer two and it
belongs to whoever knows the schema.

## The format, from the principal's own sketch

The example lives in the planning root at `source_assets/Custom_Text_Base_Syntax.txt`
and you do not need it — everything it shows is here:

```
[Player1]
type="Node"
id=1
hp=20.0

[Player1.Stats]
max_hp=200.0
some_array=[0,2,3,4,5]

//This is a comment

[Theme1]
accent="#2B2B2B"
contrast_strength=1.25
error_light="#B836BA"
```

What that pins:

- **Sections are `[Name]`**, and nesting is a **dotted name** — `[Player1.Stats]` —
  not nested brackets. Two levels in the sketch.
- **Lines are `key=value`.** One per line.
- **Comments are `//`, and this is deliberate, not a stylistic leftover.** `#` is
  what INI and TOML use, and **an unquoted hex colour starts with `#`**. A theme
  file is exactly the file most likely to spring that trap. Put the reason in the
  file header so nobody "fixes" it later.
- **Quoted text uses `"`.** Stripping quotes is lexical and is yours; deciding that
  the result is a colour is not.
- **Arrays are `[a,b,c]`** on one line. Hand back the text between the brackets, or
  hand back the elements as text — **your call, and say which and why.** Do not
  convert them.

## Questions this card decides, and it must decide them out loud

These are genuinely open and the card is asking you to settle them and write the
reasoning down, not to guess quietly:

- **Whether more than two levels of dotting is accepted or refused**, and if
  refused, with what error. Two is all the sketch shows and all anything needs.
- **Whitespace** around `=` and around section names. Tolerated or rejected.
- **Escapes inside quoted text.** The honest minimum is `\"` and `\\`; the honest
  alternative is none at all, with a stated reason.
- **Duplicate keys in one section**, and duplicate sections. Last wins, first wins,
  or refused. Refused is probably right for a format whose whole selling point is
  that a bad edit stays local.
- **What a malformed line does.** Rule 13 says a failure the world caused is
  returned — a file is the world. So it is a returned failure with enough
  information to name the line.

## Scope

- `assets/include/assets/` gains one header. `assets/src/` gains the parser.
- **The caller supplies the bytes.** File reading is `platform`'s and this card does
  not do it — hand the parser a buffer, the same way the image decoders work.
- **Working memory is an arena passed in** (rule 11). Rewind and keep nothing.
- **No recursion** (rule 14). A two-level dotted name cannot nest, so this should
  fall out for free rather than needing a depth counter — **say in your report
  whether it did.**

## What must not change

- **No type conversion anywhere.** No floats, no integers, no booleans, no colours.
- **The JSON reader** and everything else in `assets` is untouched. This is a second
  reader beside it, not a generalisation of it.
- **Nothing above `assets` learns about this**, and no consumer is written here.

## Where this card is likely to go wrong

- **Building layer two by accident**, because returning a `float` feels more useful
  than returning `"1.25"`. It is the one way this card fails.
- **A comment inside a quoted string.** `name="http://example.com"` contains `//`
  and is not a comment. Get the order of lexing right and test it.
- **An empty section, a section with no keys, a key with an empty value, a file
  that is only comments.** All legal, all easy to crash on.
- **Over-building for the sigils.** They are layer three and they are not yours.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **Tests are plain C, one per module** (rule 12), and this wants a real set: the
  sketch above parsed whole; each decided question above with a test that pins the
  answer you chose; the malformed cases named above; and `//` inside quotes.
- **Parse the principal's actual example file content** as a test fixture, including
  the `[Theme1]` section. If anything in it does not parse, that is a finding and it
  goes in your report rather than being worked around.
- No screenshot — nothing draws.
- Windows is the principal's, though this card has no platform-specific code and
  should be the rare one that simply works.

## Report when this lands

- **Each of the decided questions above, with the answer and the reasoning.** That
  list is the real output of this card; the code is the easy half.
- Whether no-recursion fell out of the format's shape or needed a counter.
- Whether anything in the principal's example file failed to parse.
- What you deliberately did not build, so the next person knows the parser is layer
  one and looks complete without being finished.

## Notes (coder, 2026-09-08, Linux/WSL, claude-fable-5-1)

**Implemented in full. `cmake -P check.cmake` did NOT run on this machine** —
the WSL checkout has no `ninja`, no `slangc` and no `wayland-scanner`, so the
script fails at step 1 before it reaches anything. What did run, all green:
`assets` configured standalone (CMake 4.2.3, Clang 21.1.8, Unix Makefiles
generator since Ninja is absent); the library and all six assets test
executables built with the engine's own flag set (`-std=c23 -Wall -Wextra
-Wpedantic -Werror`) through the CMake targets; `ctest -R assets` 6/6 passed,
`assets/sectioned` among them; `clang --analyze -Xanalyzer -analyzer-output=text`
over `src/sectioned.c` and `tests/sectioned.c` reported nothing; the includes
guard (step 5) applied by hand — the new files name only `base/` and `assets/`,
and `testing/` appears only under `tests/`. The one thing the build tripped on
was the `compile_commands.json` copy into the source tree, which the 9p mount
refuses; that is this machine, not the code. **Windows is unchecked**, though
nothing here is platform-specific. The full `check.cmake` run is the human's,
on a machine that has the tools.

**What was added — `assets` only, nothing above it touched:**
`include/assets/sectioned.h` (the format, every decision, three functions),
`src/sectioned.c` (the reader), `tests/sectioned.c` (the fixture and one test
per decision), three lines in `assets.md`. The JSON reader and everything else
in the folder are untouched.

**The decided questions — the real output of this card:**

- **Dotting deeper than two parts is refused**, as malformed, with the line
  named: `[a.b.c]` fails, `[a.b]` passes, and so do `[.a]`, `[a.]`, `[a..b]`
  fail (an empty part). Reason: two is all the sketch shows and all anything
  needs, and a limit is easier to lift than to impose after files exist. A
  dotted name is **flat text** to this reader — `[Player1.Stats]` does not need
  a `[Player1]`, builds no tree, and does not see its parent's keys.
- **Whitespace is tolerated** — spaces and tabs around `=`, inside the section
  brackets, at either end of a line — and **refused inside a key or section
  name** (`max hp=1` and `[Pl ayer]` are malformed). Reason: a hand-authored
  format that refused `hp = 20` would be hostile for no protection gained; a
  blank inside a name is a typo, not an intent. Blanks inside quotes belong to
  the value; an unquoted value loses only its end blanks.
- **No escapes.** A quoted value runs from `"` to the next `"`, comes back byte
  for byte, and only blanks may follow the closing quote. `"C:\Assets\x"` is
  `C:\Assets\x`; `"say \"hi\""` is refused (text after the closing quote).
  Reason: the one backslash an authored file is likely to hold is a Windows
  path, and `\"`/`\\` would refuse that line to make room for a quote inside a
  string, which nothing wants yet. So a quoted value cannot contain `"`; the
  card that needs one adds the escape (rule 10).
- **Duplicates are refused**, both a key twice in one section and a section
  twice in one file, as the card guessed. Same key in two sections is fine;
  case is not folded (`x` and `X` are two keys). Reason: the format's point is
  that a bad edit stays local, and "last wins" is how a stray paste silently
  changes a value three screens up.
- **A malformed line is a returned `false`** with one stderr line:
  `assets: sectioned: line N: <what>`. The messages name the shape: no `=`, a
  `=` with nothing before it, `[` with no `]`, text after `]`, a quote never
  closed on its line, text after a closing quote, a NUL byte, a key before any
  section. **A key before any section header is refused** — every key belongs
  to a section; a sectionless key has no address a consumer can ask for.
- **Arrays come back verbatim, brackets included**: `some_array=[0,2,3,4,5]`
  hands back `[0,2,3,4,5]`. The card offered "text between the brackets" or
  "elements as text"; this is neither and here is why: the brackets are the
  array's own type sigil, and splitting elements needs a separator rule, which
  is a type rule and layer three's. Least interpretation wins.
- **Comments are `//` at the start of a line and nowhere else.** No trailing
  comments: `hp=20 // health` is the value `20 // health`, and
  `url=http://example.com` is whole whether quoted or not. Reason in the header:
  `#` is a hex colour, and a trailing-comment rule is exactly the one that eats
  an unquoted URL. Tested with `//` inside and outside quotes.
- **Line endings:** `\n`, with one `\r` before it tolerated (the human authors on
  Windows); the last line need not end. A `\r` mid-line is text and makes the
  line malformed.
- **Empty things are legal and tested:** empty file (size 0), only blanks, only
  comments, a section with no keys between two that have some, `a=` and `a=""`
  both giving `""` (never NULL).

**Shape decisions the card left to me, stated:**

- **No `voe_base_error` parameter.** Every failure here is the file being
  malformed — one category — and rule 13 says one way to fail is a bare
  `false`, no error parameter with one value in it. The stderr line carries
  the line number.
- **Names and values are copied into the arena as NUL-terminated C strings**,
  one text pool pushed once, so a layer-two reader can hand a value to `strtod`
  without the copy-out the JSON reader's span shape forces, and the caller's
  buffer may be freed on return (tested: the buffer is overwritten after parse
  and the strings survive). The output is bounded by the input — a line is at
  most one record, every string a disjoint substring — so **no caller-stated
  capacity** unlike the JSON reader; two passes over one line lexer size the
  arrays exactly. Refused up front: a text over `UINT32_MAX` bytes.
- **Two lookups** — `_find(doc, "Player1.Stats")` → index or NONE, and
  `_value(doc, section, "hp")` → `const char *` or NULL — following the JSON
  reader's `_member`. Without them every layer-two consumer writes the same
  strcmp loop. Linear; files are small and hand-authored.
- **Whether a value was quoted is not recorded.** The card asked for text
  minus quoting and nothing reads a flag. *Suggestion, not done:* if the
  layer-three answer to "how a value carries its own type" turns out to be
  "quoting means string", a `bool quoted` on `voe_assets_sectioned_key` is the
  one thing this reader would need to add; it is a one-line change.

**No-recursion fell out of the format's shape**, no counter needed: nothing
nests, a dotted name is a name, and the parser is a loop over lines.

**The principal's example file was not read** — the card says the coder does
not need it and the planning root is off-limits to this role. The fixture is
the card's restatement of the sketch, all three sections including `[Theme1]`,
plus a fourth section holding the three sigil forms the card quotes
(`400.12345d`, `400_000_000_000`, `bool=1`). All of it parses, and every value
comes back as the text it was written as, hex colours included. If the real
file has a line the card did not restate — a trailing comment, say — the
human's run of the real file is what finds it, and it would be a finding
against a decision above rather than a bug.

**Deliberately not built**, so the parser looks complete without being
finished: no number, boolean, colour or array-element conversion (layer two);
no reading of the type sigils (layer three); no escapes; no trailing comments;
no file reading (`platform` owns files, the caller supplies bytes); no
consumer anywhere above `assets`; no quoted flag.

**Markers left: none.** No `DEVIATION:`, no `BLOCKED:`.
