// The engine's own authored text format: `[Section]` headers and flat
// `key=value` lines. Written here (ADR-0023), nothing fetched, and it is a
// second reader beside the JSON one rather than a generalisation of it.
//
//     [Player1]
//     type="Node"
//     hp=20.0
//
//     [Player1.Stats]
//     some_array=[0,2,3,4,5]
//
//     //This is a comment
//
//     [Theme1]
//     accent="#2B2B2B"
//     contrast_strength=1.25
//
// IT RETURNS TEXT AND INTERPRETS NOTHING, AND THAT IS THE WHOLE SHAPE OF IT.
// This is layer one of three. A consumer that knows its schema — a theme reader
// that knows `contrast_strength` is a number because a person wrote the code
// that asks for it — is layer two, converts the text it gets, and is not here.
// Reading a value back without knowing it in advance is layer three, blocked on
// how a value carries its own type, and the sigils the format sketches for that
// — `400.12345d`, `400_000_000_000`, `bool=1` — exist for it and are not read
// here: they come back as the text they are. So `hp=20.0` hands back the five
// characters `20.0`, `accent="#2B2B2B"` hands back `#2B2B2B`, and
// `some_array=[0,2,3,4,5]` hands back `[0,2,3,4,5]`, brackets included: the
// brackets are the array's own type sigil and splitting the elements would need
// a separator rule, which is a type rule and layer three's. Anyone writing a
// number parser in this module has started layer two and should stop.
//
// COMMENTS ARE `//` AND NOT `#`, DELIBERATELY. `#` is what INI and TOML use,
// and an unquoted hex colour starts with `#` — a theme file is exactly the file
// most likely to spring that trap. A comment is a line whose first non-blank
// characters are `//`, and that is the only place `//` means anything: it is
// not a comment after a value, so `url=http://example.com` is a value that says
// `http://example.com` whether it is quoted or not. That is the order of lexing
// that keeps a comment out of a string, and it is also why the sketch's
// whole-line comments are the only kind.
//
// QUOTES ARE STRIPPED AND A QUOTED VALUE HAS TWO ESCAPES, `\"` AND `\\`
// (ADR-0149). A value that starts with `"` runs to the next `"` that is not
// escaped, and only blanks may follow it on the line. Inside it `\"` comes back
// as `"` and does not close the value, `\\` comes back as `\`, and a backslash
// followed by anything else — the end of the line included — refuses the line,
// so `name="say \"hi\""` is `say "hi"` and `path="C:\Assets"` is malformed. An
// authored path is written with `/` on every platform, so a Windows path never
// needs a backslash. Outside quotes a backslash is a byte like any other:
// `url=http://a\b` is `http://a\b`. Whether a value was quoted is not recorded,
// quoted or not: the consumer gets the text, and a flag nothing reads is
// surface.
//
// THE DECISIONS THE FORMAT LEFT OPEN, EACH PINNED BY A TEST:
//
//   - A dotted section name has at most two parts: `[Player1.Stats]` is
//     accepted, `[a.b.c]` is refused. Two is all the sketch shows and all
//     anything needs, and a limit is easier to lift than to impose after files
//     exist. The name is flat text to this reader — `[Player1.Stats]` does not
//     require a `[Player1]` and builds no tree — which is why there is no
//     recursion here for rule 14 to worry about: nothing nests.
//   - Blanks — spaces and tabs — are tolerated around `=`, inside the brackets
//     of a section header and at either end of a line, so `hp = 20` reads the
//     same as `hp=20`. A hand-authored format that refused a space would be
//     hostile for no protection gained. Inside a key or a section name a blank
//     is refused, because `max hp=1` is a typo and not an intent.
//   - A key twice in one section is refused, and so is a section twice in one
//     file. The format's whole selling point is that a bad edit stays local,
//     and "last wins" is how a stray paste silently changes a value three
//     screens up. The same key in two sections is two keys.
//   - A key before any section header is refused. Every key belongs to a
//     section, and a key with no section has no address a consumer can ask for.
//   - An empty section, a section with no keys, a key with an empty value (`a=`
//     and `a=""` both), a file that is only comments and a file that is empty
//     are all legal and all come back as exactly what they are.
//   - Line endings are `\n`, with a `\r` before it tolerated, so a file saved
//     on Windows reads the same as one saved on Linux. The last line need not
//     end in one.
//
// THERE IS ONE WAY TO FAIL — THE FILE IS MALFORMED — SO THIS RETURNS FALSE AND
// TAKES NO ERROR PARAMETER (rule 13). A file is the world, so a bad line is
// returned and not asserted, and the line on stderr names the line number and
// what was wrong with it, which is all a person fixing the file needs. A
// malformed line is any line that is not blank, a comment, a section header or
// a key: a line with no `=`, a `=` with nothing before it, a `[` with no `]`,
// text after a `]` or after a closing quote, a quote never closed on its line,
// a backslash in a quoted value that is not `\"` or `\\`, a NUL byte anywhere.
//
// THE BYTES ARE THE CALLER'S AND ARE NOT KEPT. Every name and value is copied
// into `arena` as a NUL-terminated C string, so a layer-two reader can hand one
// to strtod without copying it out first — which is the copy the JSON reader's
// span shape forces on every number — and the text buffer may be freed the
// moment this returns. The caller supplies the bytes: nothing in this folder
// opens a file, and `platform` owns files.
#pragma once

#include <base/arena.h>

#include <stddef.h>
#include <stdint.h>

// What a section index holds when there is no such section.
#define VOE_ASSETS_SECTIONED_NONE UINT32_MAX

// One `key=value` line. Both are NUL-terminated text in the arena the reader
// was handed; `value` is never NULL and is "" for a key that had nothing after
// its `=`.
typedef struct {
	const char *name;
	const char *value;
} voe_assets_sectioned_key;

// One `[Section]`, and the run of keys that followed it. `name` is the text
// between the brackets, dots and all, and nothing here relates one dotted name
// to another.
typedef struct {
	const char *name;
	// Into voe_assets_sectioned.keys. The keys of one section are
	// contiguous and in file order.
	uint32_t first_key;
	uint32_t key_count;
} voe_assets_sectioned_section;

// Everything the file held, in file order, in the arena the reader was handed.
// Freed by rewinding or destroying that arena and never one allocation at a
// time (rule 11). A file with nothing in it is a count of nought and NULL
// arrays.
typedef struct {
	const voe_assets_sectioned_section *sections;
	uint32_t section_count;

	const voe_assets_sectioned_key *keys;
	uint32_t key_count;
} voe_assets_sectioned;

// Read the text. False on failure, `out` untouched when it fails, and a line on
// stderr naming the line number and what was wrong. `text` is a span of `size`
// bytes and need not be NUL-terminated; what is pushed on `arena` before a
// failure is the caller's to rewind, as with every reader in this folder.
[[nodiscard]] bool voe_assets_sectioned_parse(const char *text, size_t size,
					      voe_base_arena *arena,
					      voe_assets_sectioned *out);

// The section with exactly this name — `Player1.Stats`, dots included — or
// VOE_ASSETS_SECTIONED_NONE.
uint32_t voe_assets_sectioned_find(const voe_assets_sectioned *doc,
				   const char *name);

// The value of the named key in that section, or NULL when the section has no
// such key. Asserts on a section index the document does not have — asking for
// a key of a section that was not found is the caller's bug.
const char *voe_assets_sectioned_value(const voe_assets_sectioned *doc,
				       uint32_t section, const char *name);
