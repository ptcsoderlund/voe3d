# 038 — the sectioned format parser

status: todo
claimed-by: -
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
